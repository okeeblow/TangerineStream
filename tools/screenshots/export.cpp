#include "demo.hpp"
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

struct Entry {
    std::string kind, id, name, cover, data;
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
        db_.exec(R"SQL(
          PRAGMA foreign_keys=ON;
          BEGIN;
          CREATE TABLE object(id INTEGER PRIMARY KEY, kind TEXT NOT NULL, remote_id TEXT NOT NULL, data TEXT NOT NULL,
                              UNIQUE(kind,remote_id));
          CREATE TABLE collection(name TEXT PRIMARY KEY, total INTEGER NOT NULL CHECK(total>=0)) WITHOUT ROWID;
          CREATE TABLE member(collection TEXT REFERENCES collection, object INTEGER REFERENCES object, position INTEGER NOT NULL,
                              PRIMARY KEY(collection,position)) WITHOUT ROWID;
          CREATE TABLE image(digest TEXT PRIMARY KEY, data BLOB NOT NULL) WITHOUT ROWID;
          CREATE TABLE cover(remote_id TEXT PRIMARY KEY, digest TEXT REFERENCES image) WITHOUT ROWID;
          CREATE TABLE pose(key TEXT PRIMARY KEY, value TEXT NOT NULL) WITHOUT ROWID;
          CREATE TABLE playback(position INTEGER PRIMARY KEY, object INTEGER NOT NULL UNIQUE REFERENCES object, lyrics TEXT NOT NULL);
          PRAGMA user_version=2;
        )SQL");
        native("artist","artist",listRows);
        native("albumartist","artist",listRows,{{"role","albumartist"}});
        native("song","song",listRows);
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
        detail(album);
        std::sort(allFavorites.begin(),allFavorites.end(),byName);
        playbackQueue(allFavorites);
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
        collection("genre",static_cast<int>(genres.size()));
        for (size_t i = 0; i < std::min<size_t>(listRows,sortedGenres.size()); ++i) add("genre",sortedGenres[i]);
        Document folderDoc(source_.sub("getMusicFolders"));
        auto folders = rows(folderDoc.root["musicFolders"].value(),"musicFolder");
        collection("folder",static_cast<int>(folders.size()));
        for (size_t i = 0; i < std::min<size_t>(listRows,folders.size()); ++i) {
            auto folder = project("folder",folders[i]);
            add("folder",folder);
            if (i == 0) {
                Document indexDoc(source_.sub("getIndexes",{{"musicFolderId",folder.id}}));
                std::vector<Entry> dirs;
                for (auto index : rows(indexDoc.root["indexes"].value(),"index"))
                    for (auto dir : rows(index,"artist")) dirs.push_back(project("folder",dir));
                std::sort(dirs.begin(),dirs.end(),byName);
                collection("folder/"+folder.id,static_cast<int>(dirs.size()));
                for (size_t j=0; j<std::min<size_t>(listRows,dirs.size()); ++j) add("folder/"+folder.id,dirs[j]);
            }
        }
        for (const auto& id : covers_) {
            auto data = source_.sub("getCoverArt",{{"id",id},{"size","200"}});
            if (data.size()<12 || (data.compare(0,2,"\xff\xd8") && data.compare(0,4,"\x89PNG") && data.compare(0,4,"RIFF")))
                throw Error("Expected a 200-pixel cover image.");
            auto digest = tangerine::md5(data);
            Query(db_,"INSERT OR IGNORE INTO image VALUES(?,?)").bind(digest).blob(data).next();
            Query(db_,"INSERT INTO cover VALUES(?,?)").bind(id).bind(digest).next();
        }
        // Navidrome's native browser also requests the unversioned album or media ID.
        // Point those aliases at the same 200px image already kept for the photo.
        db_.exec(R"SQL(
          INSERT OR IGNORE INTO cover
          SELECT 'al-'||o.remote_id,c.digest FROM object o JOIN cover c ON c.remote_id=o.data->>'$.coverArt'
          WHERE o.kind='album';
          INSERT OR IGNORE INTO cover
          SELECT 'mf-'||o.remote_id,c.digest FROM object o JOIN cover c ON c.remote_id=o.data->>'$.coverArt'
          WHERE o.kind='song';
          INSERT OR IGNORE INTO cover
          SELECT 'al-'||(o.data->>'$.albumId'),c.digest FROM object o JOIN cover c ON c.remote_id=o.data->>'$.coverArt'
          WHERE o.kind='song' AND o.data->>'$.albumId' IS NOT NULL ORDER BY o.id;
        )SQL");
        Query invalid(db_,"SELECT name FROM collection WHERE total<(SELECT count(*) FROM member WHERE collection=name)");
        if (invalid.next()) throw Error("The library changed during export. Run the refresh again.");
        db_.exec("COMMIT; VACUUM;");
        std::cout << allFavorites.size() << " favorites and " << covers_.size() << " cover references exported.\n";
    }
private:
    Database& db_;
    Source& source_;
    Entry firstAlbum_;
    std::set<std::string> covers_;
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
                pose("albumLeadingBlanks",std::to_string(omitted));
                std::cout<<"album: "<<total<<", unstarred most-played viewport at "<<first<<"\n";
                return;
            }
            if(start+pageSize>=total || (!ranked.empty() && ranked.back().plays==0)) break;
        }
        throw Error("Could not find two rows of played, unstarred albums for the gallery.");
    }
    void collection(const std::string& name,int total) {
        Query(db_,"INSERT INTO collection VALUES(?,?)").bind(name).bind(total).next();
    }
    void add(const std::string& list,const Entry& entry) {
        Query(db_,"INSERT OR IGNORE INTO object(kind,remote_id,data) VALUES(?,?,?)")
            .bind(entry.kind).bind(entry.id).bind(entry.data).next();
        Query(db_,"INSERT INTO member SELECT ?,o.id,(SELECT count(*) FROM member WHERE collection=?) FROM object o "
                  "WHERE kind=? AND remote_id=? AND (? OR NOT EXISTS(SELECT 1 FROM member WHERE collection=? AND object=o.id))")
            .bind(list).bind(list).bind(entry.kind).bind(entry.id).bind(list.rfind("playlist/",0)==0).bind(list).next();
    }
    void native(const std::string& list,const std::string& kind,int limit,tangerine::Parameters params={},
                const std::string& sort="",bool descending=false) {
        int total=0;
        Document doc(source_.list(kind,limit,std::move(params),total,sort,descending));
        collection(list,total);
        for (auto node : doc.root.get_array().value()) {
            auto entry=project(kind,node);
            add(list,entry);
        }
        std::cout << list << ": " << total << '\n';
    }
    bool havePlaylist_=false;
    void playlists() {
        int total=0;
        for (int start=0; ; start+=listRows) {
            Document page(source_.list("playlist",listRows,{},total,"name",false,start));
            if (!start) collection("playlist",total);
            for (auto node : page.root.get_array().value()) {
                const auto entry=project("playlist",node);
                if (!start) add("playlist",entry);
                int64_t count=0;
                if (havePlaylist_ || (!node["songCount"].get_int64().get(count) && count==0)) continue;
                Document detail(source_.sub("getPlaylist",{{"id",entry.id}}));
                auto songs=rows(detail.root["playlist"].value(),"entry");
                if (songs.empty()) continue;
                havePlaylist_=true;
                if (start) add("playlist",entry);
                pose("playlist",entry.name);
                collection("playlist/"+entry.id,static_cast<int>(songs.size()));
                for (size_t i=0;i<std::min<size_t>(listRows,songs.size());++i)
                    add("playlist/"+entry.id,project("song",songs[i]));
            }
            if (havePlaylist_ || start+listRows>=total) break;
        }
        if (!havePlaylist_) pose("playlist","");
        std::cout << "playlist: " << total << '\n';
    }
    void pose(const std::string& key,const std::string& value) {
        Query(db_,"INSERT INTO pose VALUES(?,?)").bind(key).bind(value).next();
    }
    void filterExamples() {
        Query featured(db_,"SELECT value FROM pose WHERE key='song'");
        featured.next();
        const std::string fallback=featured.bytes();
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
    void playbackQueue(const std::vector<Entry>& favorites) {
        std::vector<Entry> queue;
        std::set<std::string> titles;
        const size_t required=havePlaylist_?21:20;
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
            if (queue.size()==required) break;
            if (album.kind!="album") continue;
            Document doc(source_.sub("getAlbum",{{"id",album.id}}));
            for(auto node:rows(doc.root["album"].value(),"song"))
                if(choose(project("song",node))) break;
        }
        if(queue.size()<required) {
            Query candidates(db_,"SELECT data FROM object WHERE kind='song' ORDER BY id");
            while(queue.size()<required && candidates.next()) {
                Document doc(candidates.bytes());
                choose(project("song",doc.root));
            }
        }
        if(queue.size()!=required) throw Error("The demo needs a different playable track for every screenshot.");
        std::vector<SongLyrics> lyrics;
        for(const auto& song:queue) lyrics.push_back(songLyrics(source_,song));
        const size_t bento=queue.size()-3;
        auto richest=std::max_element(lyrics.begin(),lyrics.end(),[](const auto& a,const auto& b) {
            return a.distinctTimedLines<b.distinctTimedLines;
        });
        if(richest->distinctTimedLines<20 || richest->lines.size()<32)
            throw Error("Big Bento needs a track with at least 32 synchronized lyric lines.");
        const size_t chosen=static_cast<size_t>(richest-lyrics.begin());
        std::swap(queue[bento],queue[chosen]);
        std::swap(lyrics[bento],lyrics[chosen]);
        for(size_t i=0;i<queue.size();++i) {
            const auto& song=queue[i];
            add("song",song);
            std::string excerpt="[]";
            // The first two shots precede connection, and the editors fill their own frame.
            const size_t firstEditor = havePlaylist_ ? 15 : 14;
            if (i>=2 && i!=firstEditor && i!=firstEditor+1) {
                wantCover(song);
                size_t first=0, count=i+1==queue.size()?26:12;
                if(i==bento) {
                    count=40; // visible rows in the tall native Lyrics pane
                    Document track(song.data);
                    auto targetMs=static_cast<int64_t>(track.root["duration"].get_double().value()*330);
                    const auto& lines=lyrics[i].lines;
                    size_t cue=0;
                    for(size_t n=0;n<lines.size();++n)
                        if(lines[n].startMs>=0 && lines[n].startMs<=targetMs && !lines[n].text.empty()) cue=n;
                    first=std::min(cue>2?cue-2:0,lines.size()>count?lines.size()-count:0);
                    pose("bentoSeek",std::to_string(std::max<int64_t>(1,lines[cue].startMs/1000)));
                    std::cout<<"Big Bento: "<<song.name<<", "<<lyrics[i].distinctTimedLines<<" distinct timed lines\n";
                }
                excerpt=lyricExcerpt(lyrics[i],first,count);
            }
            Query(db_,"INSERT INTO playback SELECT ?,id,? FROM object WHERE kind='song' AND remote_id=?")
                .bind(static_cast<int>(i)).bind(excerpt).bind(song.id).next();
        }
    }
    void detail(const Entry& album) {
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
