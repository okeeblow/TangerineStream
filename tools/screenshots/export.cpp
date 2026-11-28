#include "demo.hpp"
#include "migrations.hpp"
#include <algorithm>
#include <cctype>
#include <set>

namespace demo {
namespace {
constexpr char origin[] = "https://music.cooltrainer.org";
constexpr int visibleFavoriteCovers = 18; // six columns and three visible rows in the Olive layout
constexpr int listRows = 23; // visible rows including the partial bottom row of the Classic Modern library

struct Document {
    simdjson::dom::parser parser;
    Element root;
    explicit Document(const std::string& json) : root(parser.parse(json).value()) {}
};

std::vector<Element> rows(Element parent, const char* key) {
    simdjson::dom::array array;
    auto error = parent[key].get_array().get(array);
    if (error == simdjson::NO_SUCH_FIELD) return {};
    if (error) throw Error("Expected an array in the library response.");
    return {array.begin(), array.end()};
}

class Source {
public:
    Source(const std::string& user, const std::string& password) : user_(user), password_(password) {
        auto body = "{\"username\":" + jsonString(user) + ",\"password\":" + jsonString(password) + "}";
        tangerine::Response login(client_, std::string(origin) + "/auth/login", L"POST", L"Content-Type: application/json\r\n", body);
        if (login.status != 200) throw Error("Navidrome refused the login.");
        Document doc(login.text());
        auto token = field(doc.root, "token");
        if (token.empty() || token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.") != token.npos)
            throw Error("Invalid Navidrome login response.");
        authorization_ = L"x-nd-authorization: Bearer " + tangerine::wide(token) + L"\r\n";
    }
    std::string list(const std::string& endpoint, int limit, tangerine::Parameters params, int& total,
                     const std::string& sort = "", bool descending = false, int start = 0) {
        params.insert(params.end(), {{"_start",std::to_string(start)}, {"_end",std::to_string(start+limit)},
            {"_sort",sort.empty() ? (endpoint == "song" ? "title" : "name") : sort}, {"_order",descending?"DESC":"ASC"}});
        tangerine::Response response(client_, std::string(origin) + "/api/" + endpoint + tangerine::queryString(params),
                                    L"GET", authorization_);
        if (response.status != 200) throw Error("Could not read the Navidrome " + endpoint + " list.");
        auto count = response.header(L"x-total-count");
        if (count.empty() || count.find_first_not_of("0123456789") != count.npos)
            throw Error("Navidrome omitted the total for " + endpoint + ".");
        total = std::stoi(count);
        return response.text();
    }
    std::string sub(const std::string& endpoint, tangerine::Parameters params = {}) {
        auto salt = tangerine::randomHex();
        params.insert(params.end(), {{"u",user_},{"s",salt},{"t",tangerine::md5(password_ + salt)},
            {"v","1.16.1"},{"f","json"},{"c","tangerine-demo-export"}});
        tangerine::Response response(client_, std::string(origin) + "/rest/" + endpoint + ".view" + tangerine::queryString(params),
                                    L"GET", L"");
        if (response.status != 200) throw Error("Could not read " + endpoint + ".");
        if (endpoint == "getCoverArt") return response.text(2 * tangerine::MiB);
        Document doc(response.text());
        auto envelope = doc.root["subsonic-response"].value();
        if (field(envelope, "status") != "ok") throw Error("Navidrome rejected " + endpoint + ".");
        return simdjson::minify(envelope);
    }
private:
    tangerine::HttpClient client_;
    std::string user_, password_;
    std::wstring authorization_;
};

struct LyricLine {std::string text; int64_t startMs;};
struct SongLyrics {
    std::string lang, offset;
    bool synced=false;
    std::vector<LyricLine> lines;
    size_t distinctTimedLines=0;
};
SongLyrics songLyrics(Source& source,const Entry& song) {
    Document doc(source.sub("getLyricsBySongId",{{"id",song.id},{"enhanced","true"}}));
    SongLyrics best;
    for(auto variant:rows(doc.root["lyricsList"].value(),"structuredLyrics")) {
        auto kind=field(variant,"kind");
        if(!kind.empty() && kind!="main") continue;
        SongLyrics result;
        result.lang=field(variant,"lang");
        Element offset;
        if(!variant["offset"].get(offset) && offset.is_number()) result.offset=simdjson::minify(offset);
        if(variant["synced"].get_bool().get(result.synced)) result.synced=false;
        std::set<std::string> distinct;
        for(auto value:rows(variant,"line")) {
            auto text=field(value,"value");
            int64_t ms=-1;
            if(value["start"].get_int64().get(ms)) ms=-1;
            if(result.synced && ms>=0 && text.size()>=5) distinct.insert(text);
            result.lines.push_back({std::move(text),ms});
        }
        result.distinctTimedLines=distinct.size();
        if(best.lines.empty() || result.distinctTimedLines>best.distinctTimedLines) best=std::move(result);
    }
    return best;
}
std::string lyricExcerpt(const SongLyrics& lyrics,size_t first,size_t count) {
    if(lyrics.lines.empty()) return "[]";
    std::string json="[{\"lang\":"+jsonString(lyrics.lang)+",\"synced\":"+(lyrics.synced?"true":"false");
    if(!lyrics.offset.empty()) json+=",\"offset\":"+lyrics.offset;
    json+=",\"line\":[";
    for(size_t i=first;i<std::min(first+count,lyrics.lines.size());++i) {
        const auto& cue=lyrics.lines[i];
        if(i!=first) json+=',';
        json+="{\"value\":"+jsonString(cue.text);
        if(lyrics.synced && cue.startMs>=0) json+=",\"start\":"+std::to_string(cue.startMs);
        json+="}";
    }
    return json+"]}]";
}

Entry project(const std::string& kind, Element node) {
    Entry e{kind, field(node,"id"), field(node,"name"), field(node,"coverArt"), "{"};
    if (kind == "genre") e.id = e.name = field(node,"value");
    if (e.id.empty() && kind == "folder") e.id = simdjson::minify(node["id"].value());
    if (e.id.empty()) throw Error("Library object without an ID.");
    if (e.name.empty()) e.name = field(node,"title");
    if (e.cover.empty() && kind == "album") e.cover = "al-" + e.id;
    if (e.cover.empty() && kind == "song" && !field(node,"albumId").empty()) e.cover = "al-" + field(node,"albumId");
    e.data += "\"id\":" + jsonString(e.id);
    if (kind == "genre") e.data += ",\"value\":" + jsonString(e.name);
    else e.data += (kind == "song" || kind == "directory" ? ",\"title\":" : ",\"name\":") + jsonString(e.name);
    if (!e.cover.empty()) e.data += ",\"coverArt\":" + jsonString(e.cover);
    for (const char* key : {"artist","albumArtist","album","albumId","artistId","genre","year","maxYear",
                           "songCount","albumCount","duration","discNumber","track","trackNumber","isDir"}) {
        Element value;
        if (!node[key].get(value) && !value.is_null()) e.data += "," + jsonString(key) + ":" + simdjson::minify(value);
    }
    if (kind=="album") {
        for (const char* key : {"playCount","createdAt"}) {
            Element value;
            if (!node[key].get(value)) e.data+=","+jsonString(key)+":"+simdjson::minify(value);
        }
    }
    if (kind == "song") e.data += ",\"suffix\":\"wav\",\"contentType\":\"audio/wav\"";
    e.data += "}";
    return e;
}

class Export {
public:
    Export(Database& db, Source& source) : db_(db), source_(source) {}
    void run() {
        db_.exec("PRAGMA foreign_keys=ON");
        tangerine::migrateLibrary(db_.get());
        db_.exec("BEGIN IMMEDIATE");
        // The migrated library tables require a server key. Demo credentials exist only at runtime.
        db_.exec("INSERT INTO servers(id,name,url,user,password,native,cache_favorites,configured) "
                 "VALUES(1,'Demo Server','https://tangerine.stream/','','',1,0,0)");
        nativeWindow("artist","artist");
        nativeWindow("albumartist","artist",{{"role","albumartist"}});
        nativeWindow("song","song");
        playlists();
        Document starred(source_.sub("getStarred2"));
        auto favorites = starred.root["starred2"].value();
        std::set<std::string> starredAlbums;
        for (auto entry : rows(favorites,"album")) starredAlbums.insert(field(entry,"id"));
        albumShowcase(starredAlbums);
        std::vector<Entry> allFavorites;
        for (const char* kind : {"album","artist","song"}) {
            const auto values = rows(favorites,kind);
            auto list = std::string("starred/") + kind;
            collection(list, static_cast<int>(values.size()));
            for (auto value : values) {
                auto entry = project(kind,value);
                add(list,entry);
                add(kind,entry);
                allFavorites.push_back(entry);
            }
        }
        auto featured = std::find_if(allFavorites.begin(),allFavorites.end(),[](const Entry& entry) {
            return entry.kind == "album" && entry.name == "Speaking In Tongues";
        });
        if (featured == allFavorites.end()) featured = std::find_if(allFavorites.begin(),allFavorites.end(),
            [](const Entry& entry) { return entry.kind == "album"; });
        auto album = featured != allFavorites.end() ? *featured : firstAlbum_;
        if (album.id.empty()) throw Error("The library has no album to pose.");
        const auto openingTrack=detail(album);
        std::sort(allFavorites.begin(),allFavorites.end(),byName);
        playbackQueue(allFavorites,openingTrack);
        filterExamples();
        albumSearch();
        // The Favorites poses use the browser's descending song-count sort.
        auto favoriteAlbums = allFavorites;
        favoriteAlbums.erase(std::remove_if(favoriteAlbums.begin(), favoriteAlbums.end(),
            [](const Entry& entry) { return entry.kind != "album"; }), favoriteAlbums.end());
        auto songCount = [](const Entry& album) {
            Document doc(album.data);
            int64_t count = 0;
            if (doc.root["songCount"].get_int64().get(count)) return int64_t(0);
            return count;
        };
        std::stable_sort(favoriteAlbums.begin(), favoriteAlbums.end(), [&](const Entry& a, const Entry& b) {
            return songCount(a) > songCount(b);
        });
        for (size_t i = 0; i < std::min<size_t>(visibleFavoriteCovers,favoriteAlbums.size()); ++i) wantCover(favoriteAlbums[i]);
        Document genreDoc(source_.sub("getGenres"));
        auto genres = rows(genreDoc.root["genres"].value(),"genre");
        std::vector<Entry> sortedGenres;
        for (auto value : genres) sortedGenres.push_back(project("genre",value));
        std::sort(sortedGenres.begin(), sortedGenres.end(), byName);
        const int genreTotal=static_cast<int>(sortedGenres.size());
        int bestStart=0, bestScore=-1;
        for (int start=genreTotal/5; start+listRows<=genreTotal-genreTotal/8; ++start) {
            const int score=variety(sortedGenres,start,listRows);
            if (score>bestScore) {bestStart=start;bestScore=score;}
        }
        keepWindow("genre",genreTotal,bestStart,sortedGenres,bestStart);
        finishWindows();
        for (const auto& id : covers_) {
            auto data = source_.sub("getCoverArt",{{"id",id},{"size","200"}});
            if (data.size()<12 || (data.compare(0,2,"\xff\xd8") && data.compare(0,4,"\x89PNG") && data.compare(0,4,"RIFF")))
                throw Error("Expected a 200-pixel cover image.");
            auto digest = tangerine::md5(data);
            Query(db_,"INSERT OR IGNORE INTO artwork(server,cover_key,data,last_use,requested_max_edge_px,cached_longest_edge_px) "
                      "VALUES(1,?,?,0,200,200)").bind(digest).blob(data).next();
            fixture_.coverAliases.emplace(id,digest);
        }
        // Navidrome's unversioned album and media IDs identify the same artwork.
        for (const auto& [key,entry]:fixture_.objects) {
            auto cover=fixture_.coverAliases.find(entry.cover);
            if (cover==fixture_.coverAliases.end()) continue;
            if (entry.kind=="album") fixture_.coverAliases.emplace("al-"+entry.id,cover->second);
            if (entry.kind=="song") {
                fixture_.coverAliases.emplace("mf-"+entry.id,cover->second);
                Document song(entry.data);
                auto albumId=field(song.root,"albumId");
                if (!albumId.empty()) fixture_.coverAliases.emplace("al-"+albumId,cover->second);
            }
        }
        fixture_.save(db_);
        db_.exec("COMMIT; VACUUM;");
        std::cout << allFavorites.size() << " favorites and " << covers_.size() << " cover references exported.\n";
    }
private:
    Database& db_;
    Fixture fixture_;
    Source& source_;
    Entry firstAlbum_;
    std::set<std::string> covers_;
    struct TextWindow {std::string list; int start; Entry first; bool ordered;};
    std::vector<TextWindow> textWindows_;
    static bool byName(const Entry& a, const Entry& b) {
        auto left=tangerine::wide(a.name), right=tangerine::wide(b.name);
        return CompareStringEx(LOCALE_NAME_USER_DEFAULT,NORM_IGNORECASE|SORT_DIGITSASNUMBERS,
            left.c_str(),-1,right.c_str(),-1,nullptr,nullptr,0)==CSTR_LESS_THAN;
    }
    void wantCover(const Entry& entry) { if (!entry.cover.empty()) covers_.insert(entry.cover); }
    void albumShowcase(const std::set<std::string>& favorites) {
        struct RankedAlbum {Entry entry; int64_t plays; bool eligible;};
        std::vector<RankedAlbum> ranked;
        constexpr int pageSize=128, visible=12, rowAlignment=12; // three rows in Big Bento's smaller cover grid
        int total=0;
        for(int start=0; ;start+=pageSize) {
            Document page(source_.list("album",pageSize,{},total,"playCount",true,start));
            for(auto node : page.root.get_array().value()) {
                auto album=project("album",node);
                auto label=field(node,"albumArtist")+" "+album.name;
                std::transform(label.begin(),label.end(),label.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                int64_t plays=0;
                if (node["playCount"].get_int64().get(plays)) plays=0;
                bool eligible=plays>0 && !favorites.count(album.id) && label.find("death in june")==label.npos;
                ranked.push_back({std::move(album),plays,eligible});
            }
            std::stable_sort(ranked.begin(),ranked.end(),[&](const auto& a,const auto& b){
                return a.plays!=b.plays ? a.plays>b.plays : byName(a.entry,b.entry);
            });
            for(size_t first=0;first+visible<=ranked.size();first+=rowAlignment) {
                auto begin=ranked.begin()+first, end=begin+visible;
                if (start+pageSize<total && (end-1)->plays==ranked.back().plays) break;
                if (!std::all_of(begin,end,[](const auto& album){return album.eligible;})) continue;
                collection("album",total);
                collection("album-showcase",visible);
                for(auto it=begin;it!=end;++it) {
                    add("album-showcase",it->entry);
                    add("album",it->entry);
                    wantCover(it->entry);
                }
                firstAlbum_=begin->entry;
                int omitted=static_cast<int>(std::count_if(ranked.begin(),begin,[&](const auto& a){return !favorites.count(a.entry.id);}));
                pose("albumStart",std::to_string(first));
                fixture_.collections.at("album").leadingBlanks=omitted;
                std::cout<<"album: "<<total<<", unstarred most-played viewport at "<<first<<"\n";
                return;
            }
            if(start+pageSize>=total || (!ranked.empty() && ranked.back().plays==0)) break;
        }
        throw Error("Could not find two rows of played, unstarred albums for the gallery.");
    }
    void collection(const std::string& name,int total) {
        if (!fixture_.collections.emplace(name,Collection{total,{}}).second) throw Error("Duplicate demo collection: "+name);
    }
    void add(const std::string& list,const Entry& entry) {
        ObjectKey key{entry.kind,entry.id};
        fixture_.objects.emplace(key,entry);
        auto& members=fixture_.collections.at(list).members;
        if (list.rfind("playlist/",0)==0 || std::find(members.begin(),members.end(),key)==members.end())
            members.push_back(std::move(key));
    }
    // Favor a readable spread of names, artists and styles over repeated prefixes and editions.
    static int variety(const std::vector<Entry>& entries,int first,int count) {
        std::set<std::string> words, artists, genres, titles;
        for (int i=first;i<first+count;++i) {
            const auto& entry=entries[i];
            Document doc(entry.data);
            auto label=entry.name, artist=field(doc.root,"artist");
            std::string credit=label+" "+artist+" "+field(doc.root,"albumArtist");
            std::transform(credit.begin(),credit.end(),credit.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
            if (credit.find("death in june")!=credit.npos) return -1;
            std::transform(label.begin(),label.end(),label.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
            titles.insert(label);
            if (label.rfind("the ",0)==0) label.erase(0,4);
            words.insert(label.substr(0,label.find_first_of(" -/(")));
            if (!artist.empty()) artists.insert(artist);
            auto genre=field(doc.root,"genre");
            if (!genre.empty()) genres.insert(genre);
        }
        return static_cast<int>(words.size()*3+artists.size()*2+genres.size()+titles.size());
    }
    void keepWindow(const std::string& list,int total,int start,const std::vector<Entry>& entries,int offset=0,bool ordered=false) {
        collection(list,total);
        if (entries.empty()) return;
        textWindows_.push_back({list,start,entries.at(offset),ordered});
        for (int i=offset;i<std::min(static_cast<int>(entries.size()),offset+listRows);++i) add(list,entries[i]);
        std::cout<<list<<": "<<total<<", viewport at "<<start<<" ("<<entries[offset].name<<")\n";
    }
    void nativeWindow(const std::string& list,const std::string& kind,tangerine::Parameters params={}) {
        int total=0;
        source_.list(kind,1,params,total);
        int bestScore=-2, bestStart=0;
        std::vector<Entry> best;
        // Album Artists and Artists have different membership. Sample different parts of each alphabet.
        const int phase=list=="albumartist"?13:list=="song"?23:0;
        for (int fraction:{20,34,48,62}) {
            const int start=std::max(0,total-listRows)*(fraction+phase)/100;
            Document page(source_.list(kind,listRows,params,total,"",false,start));
            std::vector<Entry> entries;
            for (auto node:page.root.get_array().value()) entries.push_back(project(kind,node));
            std::stable_sort(entries.begin(),entries.end(),byName);
            const int score=variety(entries,0,static_cast<int>(entries.size()));
            if (score>bestScore) {bestScore=score;bestStart=start;best=std::move(entries);}
            if (total<=listRows) break;
        }
        if (bestScore<0) throw Error("Could not find an eligible viewport for "+list);
        keepWindow(list,total,bestStart,best);
    }
    void finishWindows() {
        std::string windows="{";
        for (const auto& view:textWindows_) {
            auto& section=fixture_.collections.at(view.list);
            int earlier=0;
            if (!view.ordered)
                for (const auto& key:section.members) if (byName(fixture_.objects.at(key),view.first)) ++earlier;
            section.leadingBlanks=std::clamp(view.start-earlier,0,section.total-static_cast<int>(section.members.size()));
            if (windows.size()>1) windows+=',';
            const auto label=view.list.rfind("playlist/",0)==0?"playlistTracks":view.list;
            windows+=jsonString(label)+":{\"start\":"+std::to_string(section.leadingBlanks+earlier)+
                ",\"total\":"+std::to_string(section.total)+",\"first\":"+jsonString(view.first.name)+"}";
        }
        pose("textWindows",windows+"}");
    }
    bool havePlaylist_=false;
    void playlists() {
        int total=0;
        nativeWindow("playlist","playlist");
        // Prefer a playlist visible in the catalogue, then look beyond it for a populated one.
        const auto& playlistView=fixture_.collections.at("playlist");
        if (!playlistView.total) {pose("playlist","");return;}
        const int visibleStart=textWindows_.back().start;
        for (int pageNumber=-1; ; ++pageNumber) {
            const int start=pageNumber<0?visibleStart:pageNumber*listRows;
            if (start>=playlistView.total) break;
            Document page(source_.list("playlist",listRows,{},total,"name",false,start));
            for (auto node : page.root.get_array().value()) {
                const auto entry=project("playlist",node);
                int64_t count=0;
                if (havePlaylist_ || (!node["songCount"].get_int64().get(count) && count==0)) continue;
                Document detail(source_.sub("getPlaylist",{{"id",entry.id}}));
                auto songs=rows(detail.root["playlist"].value(),"entry");
                if (songs.empty()) continue;
                havePlaylist_=true;
                add("playlist",entry);
                pose("playlist",entry.name);
                const int countSongs=static_cast<int>(songs.size());
                const int first=std::max(0,countSongs-listRows)/2;
                std::vector<Entry> selected;
                for (int i=first;i<std::min(countSongs,first+listRows);++i) selected.push_back(project("song",songs[i]));
                keepWindow("playlist/"+entry.id,countSongs,first,selected,0,true);
            }
            if (havePlaylist_) break;
        }
        if (!havePlaylist_) pose("playlist","");
        std::cout << "playlist: " << total << '\n';
    }
    void pose(const std::string& key,const std::string& value) {
        fixture_.poses.emplace(key,value);
    }
    void filterExamples() {
        const std::string& fallback=fixture_.poses.at("song");
        for (const std::string& title : std::vector<std::string>{"Blue Monday","Helter Skelter","Burning Down",fallback}) {
            int total=0;
            Document page(source_.list("song",listRows,{{"title",title}},total));
            if (total<2 || (title!=fallback && total>8)) continue;
            collection("filtered-tracks",total);
            pose("filter",title);
            for (auto node : page.root.get_array().value()) add("filtered-tracks",project("song",node));
            return;
        }
        throw Error("The filtered track search needs multiple library matches.");
    }
    void albumSearch() {
        for(const std::string term:{"Massive Attack","Talking Heads","Radiohead","Selected Ambient","Love"}) {
            int total=0;
            Document page(source_.list("album",8,{{"name",term}},total));
            if(total<2 || total>8) continue;
            collection("album-search",total);
            pose("albumQuery",term);
            for(auto node:page.root.get_array().value()) {
                auto album=project("album",node);
                add("album-search",album);
                wantCover(album);
            }
            return;
        }
        throw Error("The album search pose needs two to eight matching albums.");
    }
    void playbackQueue(const std::vector<Entry>& favorites,const std::string& openingTrack) {
        std::vector<Entry> queue;
        std::set<std::string> titles;
        const size_t required=havePlaylist_?19:18;
        auto choose=[&](const Entry& song) {
            Document doc(song.data);
            double seconds=0;
            const auto artist=field(doc.root,"artist");
            if (doc.root["duration"].get_double().get(seconds) || seconds<75 || seconds>1800 ||
                !_stricmp(artist.c_str(),"Death in June") || !titles.insert(song.name).second) return false;
            queue.push_back(song);
            return true;
        };
        for(const auto& entry:favorites) if (entry.kind=="song" && queue.size()<required) choose(entry);
        for(const auto& album:favorites) {
            if (album.kind!="album") continue;
            Document doc(source_.sub("getAlbum",{{"id",album.id}}));
            for(auto node:rows(doc.root["album"].value(),"song"))
                if(choose(project("song",node))) break;
        }
        if(queue.size()<required) {
            for (const auto& key:fixture_.collections.at("song").members) {
                if (queue.size()==required) break;
                choose(fixture_.objects.at(key));
            }
        }
        if(queue.size()<required) throw Error("The demo needs a different playable track for every playing pose.");
        std::vector<SongLyrics> lyrics;
        for(const auto& song:queue) lyrics.push_back(songLyrics(source_,song));
        const size_t bento=required-4;
        auto richest=std::max_element(lyrics.begin(),lyrics.end(),[](const auto& a,const auto& b) {
            return a.distinctTimedLines<b.distinctTimedLines;
        });
        if(richest->distinctTimedLines<20 || richest->lines.size()<32)
            throw Error("Big Bento needs a track with at least 32 synchronized lyric lines.");
        const size_t chosen=static_cast<size_t>(richest-lyrics.begin());
        std::swap(queue[bento],queue[chosen]);
        std::swap(lyrics[bento],lyrics[chosen]);
        // Olive also has a tall Lyrics pane. Prefer a remaining track with enough
        // distinct timed lines to fill it, keeping the other poses' songs in place.
        const size_t olive=required-2;
        auto oliveLyrics=std::max_element(lyrics.begin()+olive,lyrics.end(),[](const auto& a,const auto& b) {
            return a.distinctTimedLines<b.distinctTimedLines;
        });
        const size_t oliveTrack=static_cast<size_t>(oliveLyrics-lyrics.begin());
        std::swap(queue[olive],queue[oliveTrack]);
        std::swap(lyrics[olive],lyrics[oliveTrack]);
        const size_t winamp3=required-1;
        // Reuse the featured album's artwork when its opening track is still available.
        auto opener=std::find_if(queue.begin()+winamp3,queue.end(),[&](const Entry& song) {
            return song.id==openingTrack;
        });
        if(opener!=queue.end()) {
            const auto index=static_cast<size_t>(opener-queue.begin());
            std::swap(queue[winamp3],queue[index]);
            std::swap(lyrics[winamp3],lyrics[index]);
        }
        queue.resize(required);
        for(size_t i=0;i<queue.size();++i) {
            const auto& song=queue[i];
            add("song",song);
            std::string excerpt="[]";
            // Add Server precedes connection, and the two editors fill their own frame.
            const size_t firstEditor = havePlaylist_ ? 12 : 11;
            if (i>=1 && i!=firstEditor && i!=firstEditor+1) {
                wantCover(song);
                size_t first=0, count=i==olive?26:12;
                if(i==bento || i==winamp3) {
                    if(i==bento) count=40; // visible rows in the tall native Lyrics pane
                    Document track(song.data);
                    const auto targetMs=static_cast<int64_t>(track.root["duration"].get_double().value()*(i==bento?330:400));
                    const auto& lines=lyrics[i].lines;
                    size_t cue=0;
                    for(size_t n=0;n<lines.size();++n)
                        if(lines[n].startMs>=0 && lines[n].startMs<=targetMs && !lines[n].text.empty()) cue=n;
                    first=std::min(cue>2?cue-2:0,lines.size()>count?lines.size()-count:0);
                    const auto seekMs=lines.empty() || lines[cue].startMs<0 ? targetMs : lines[cue].startMs;
                    pose(i==bento?"bentoSeek":"winamp3Seek",std::to_string(std::max<int64_t>(1,seekMs/1000)));
                    if(i==bento) std::cout<<"Big Bento: "<<song.name<<", "<<lyrics[i].distinctTimedLines<<" distinct timed lines\n";
                }
                excerpt=lyricExcerpt(lyrics[i],first,count);
            }
            fixture_.playback.push_back(song.id);
            fixture_.lyrics.emplace(song.id,std::move(excerpt));
        }
    }
    std::string detail(const Entry& album) {
        pose("album",album.name);
        wantCover(album);
        Document doc(source_.sub("getAlbum",{{"id",album.id}}));
        auto songs=rows(doc.root["album"].value(),"song");
        if (songs.empty()) throw Error("The featured album has no tracks.");
        collection("album/"+album.id,static_cast<int>(songs.size()));
        for (size_t i=0;i<std::min<size_t>(listRows,songs.size());++i) {
            auto song=project("song",songs[i]);
            add("album/"+album.id,song);
            add("song",song);
            if (i==0) {
                pose("song",song.name);
                wantCover(song);
            }
        }
        return field(songs.front(),"id");
    }
};
} // namespace

void exportLibrary(const std::filesystem::path& destination,const std::string& user,const std::string& password) {
    Source source(user,password);
    // Publish only after the transaction, integrity checks and final close succeed.
    auto staging=destination;
    staging+=L"."+tangerine::wide(tangerine::randomHex())+L".tmp";
    try {
        {
            Database db(staging,SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE);
            Export(db,source).run();
            Query integrity(db,"PRAGMA integrity_check");
            if (!integrity.next() || integrity.bytes()!="ok") throw Error("Demo database failed its integrity check.");
        }
        if (!MoveFileExW(staging.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw Error("Could not publish the demo database.");
    } catch (...) {
        std::filesystem::remove(staging);
        throw;
    }
}
} // namespace demo
