#include <httplib.h>
#include "demo.hpp"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <map>

namespace demo {
namespace {
using Request = httplib::Request;
using Reply = httplib::Response;

std::string param(const Request& request,const char* key) { return request.get_param_value(key); }
int index(const Request& request,const char* key,int fallback) {
    auto text=param(request,key);
    int result=0;
    auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    return parsed.ec==std::errc{} && parsed.ptr==text.data()+text.size() && result>=0 ? result : fallback;
}

void json(Reply& reply,const std::string& body) { reply.set_content(body,"application/json; charset=utf-8"); }
std::string property(const std::string& key,const std::string& value) { return jsonString(key)+":"+value; }
void subsonic(Reply& reply,const std::string& contents={},bool ok=true) {
    json(reply,"{\"subsonic-response\":{\"version\":\"1.16.1\",\"type\":\"navidrome\",\"openSubsonic\":true,\"status\":"+
        jsonString(ok?"ok":"failed")+(contents.empty()?"":","+contents)+"}}");
}
std::string lower(std::string text) {
    std::transform(text.begin(),text.end(),text.begin(),[](unsigned char ch){return static_cast<char>(std::tolower(ch));});
    return text;
}
struct Item {
    std::string id,name,data;
};
struct Collection {
    int total=0;
    std::vector<Item> items;
};

class Catalog {
public:
    explicit Catalog(const std::filesystem::path& file) : db_(file,SQLITE_OPEN_READONLY|SQLITE_OPEN_FULLMUTEX) {
        Query version(db_,"PRAGMA user_version");
        if (!version.next() || version.number()!=2) throw Error("Refresh the demo database with regenerate-screenshots.bat WACUP rebuild-db.");
        Query lists(db_,"SELECT name,total FROM collection");
        while (lists.next()) collections_[lists.bytes()].total=lists.number(1);
        Query rows(db_,R"SQL(SELECT m.collection,o.remote_id,COALESCE(json_extract(o.data,'$.name'),
            json_extract(o.data,'$.title'),json_extract(o.data,'$.value')),o.data
            FROM member m JOIN object o ON o.id=m.object ORDER BY m.collection,m.position)SQL");
        while (rows.next()) collections_.at(rows.bytes()).items.push_back({rows.bytes(1),rows.bytes(2),rows.bytes(3)});
        Query settings(db_,"SELECT key,value FROM pose");
        while (settings.next()) pose_[settings.bytes()]=settings.bytes(1);
    }
    std::string pose(const std::string& key) const { return pose_.at(key); }
    int total(const std::string& collection) const { return collections_.at(collection).total; }
    std::string queue() {
        Query rows(db_,"SELECT json_group_array(json(data)) FROM (SELECT data FROM playback p JOIN object o ON o.id=p.object ORDER BY p.position)");
        if (!rows.next()) throw Error("Missing demo queue.");
        return rows.bytes();
    }
    std::string lyrics(const std::string& id,const std::string& title="") {
        Query row(db_,"SELECT lyrics FROM playback p JOIN object o ON o.id=p.object WHERE o.remote_id=? OR json_extract(data,'$.title')=?");
        return row.bind(id).bind(title).next()?row.bytes():"[]";
    }
    std::string list(const std::string& name,int start,int count,const std::string& filter="",int* totalOut=nullptr) const {
        auto where=collections_.find(name);
        if (where==collections_.end()) {
            if (totalOut) *totalOut=0;
            return "[]";
        }
        const auto& list=where->second;
        const auto needle=lower(filter);
        std::string result="[";
        int matching=0, emitted=0;
        auto append=[&](const std::string& object){
            if (emitted++) result+=',';
            result+=object;
        };
        for (const auto& item:list.items) {
            if (!needle.empty() && lower(item.name).find(needle)==std::string::npos) continue;
            if (matching++>=start && emitted<count) append(item.data);
        }
        int total=needle.empty()?list.total:matching;
        const int leadingBlanks=name=="album" ? std::stoi(pose("albumLeadingBlanks")) : 0;
        // Unseen objects share one empty representation on disk (the remaining count).
        // Distinct wire IDs keep paginated rows distinct in the plug-in's own catalog.
        for (int i=std::max(matching,start);i<total && emitted<count;++i) {
            // U+2800 is visibly blank and sorts after names in Windows' library collation.
            const std::string blank="\xe2\xa0\x80";
            if (name=="genre") {
                // OpenSubsonic uses the genre label itself as its key.
                std::string label=blank;
                for(unsigned bits=i+1;bits;bits>>=1) label+=bits&1?"\xe2\x80\x8b":"\xe2\x81\xa0";
                append("{"+property("value",jsonString(label))+"}");
            } else {
                auto id=jsonString("padding-"+name+"-"+std::to_string(i));
                auto higherRank=i-matching<leadingBlanks ? ",\"playCount\":2147483647" : "";
                append("{"+property("id",id)+","+property("name",jsonString(blank))+","+property("title",jsonString(blank))+higherRank+"}");
            }
        }
        if (totalOut) *totalOut=total;
        return result+"]";
    }
    std::string object(const std::string& kind,const std::string& id) {
        Query row(db_,"SELECT data FROM object WHERE kind=? AND remote_id=?");
        return row.bind(kind).bind(id).next()?row.bytes():"{}";
    }
    bool cover(Reply& reply,const std::string& id) {
        Query row(db_,"SELECT data FROM cover JOIN image USING(digest) WHERE remote_id=?");
        if (!row.bind(id).next()) return false;
        auto bytes=row.bytes();
        auto mime=bytes.compare(0,2,"\xff\xd8")==0?"image/jpeg":bytes.compare(0,4,"RIFF")==0?"image/webp":"image/png";
        reply.set_content(std::move(bytes),mime);
        return true;
    }
private:
    Database db_;
    std::map<std::string,Collection> collections_;
    std::map<std::string,std::string> pose_;
};

void visualizationAudio(Reply& reply,int seconds,const std::string& trackId) {
    constexpr unsigned sampleRate=22050;
    const auto frames=static_cast<unsigned>(std::clamp(seconds,1,1800))*sampleRate;
    const auto bytes=frames*2;
    std::string wav="RIFF";
    auto little=[&](unsigned value,int width) {
        for(int i=0;i<width;++i) wav.push_back(static_cast<char>(value>>(8*i)));
    };
    little(36+bytes,4); wav+="WAVEfmt "; little(16,4); little(1,2); little(1,2);
    little(sampleRate,4); little(sampleRate*2,4); little(2,2); little(16,2); wav+="data"; little(bytes,4);
    // A deterministic spectral pattern for each track. Only the screenshot output
    // sink is installed in the player. It discards the PCM after visualization.
    auto seed=std::stoul(tangerine::md5(trackId).substr(0,6),nullptr,16);
    std::string pattern;
    constexpr double tau=6.283185307179586;
    for(unsigned frame=0;frame<sampleRate*2;++frame) {
        const double t=double(frame)/sampleRate;
        double mix=0;
        for(unsigned band=0;band<12;++band) {
            double frequency=(72+(seed>>band)%40)*std::pow(1.49,band);
            double pulse=0.55+0.45*std::cos(tau*(t*(1+band%4)/2+(seed%73+band)*0.16));
            mix+=std::sin(tau*frequency*t)*pulse;
        }
        auto sample=static_cast<uint16_t>(static_cast<int16_t>(mix*2450));
        pattern.push_back(static_cast<char>(sample)); pattern.push_back(static_cast<char>(sample>>8));
    }
    wav.reserve(44+bytes);
    for(unsigned remaining=bytes;remaining;) {
        auto size=std::min<size_t>(remaining,pattern.size());
        wav.append(pattern,0,size);
        remaining-=static_cast<unsigned>(size);
    }
    reply.set_content(std::move(wav),"audio/wav");
}

void rest(Catalog& db,const Request& request,Reply& reply) {
    std::string route=request.matches[1];
    auto id=param(request,"id");
    if (route=="ping" || route=="scrobble") return subsonic(reply);
    if (route=="getOpenSubsonicExtensions") return subsonic(reply,"\"openSubsonicExtensions\":[]");
    if (route=="getCoverArt") { if (!db.cover(reply,id)) reply.status=404; return; }
    if (route=="stream") {
        simdjson::dom::parser parser;
        auto text=db.object("song",id);
        if (text=="{}") {reply.status=404;return;}
        double duration=0;
        auto error=parser.parse(text)["duration"].get_double().get(duration);
        return visualizationAudio(reply,error?30:static_cast<int>(duration),id);
    }
    auto group=[&](const char* outer,const char* key,const std::string& value) {
        subsonic(reply,property(outer,"{"+property(key,value)+"}"));
    };
    if (route=="getStarred2") {
        subsonic(reply,"\"starred2\":{"+property("album",db.list("starred/album",0,INT_MAX))+","+
            property("artist",db.list("starred/artist",0,INT_MAX))+","+property("song",db.list("starred/song",0,INT_MAX))+"}");
    } else if (route=="getAlbum") group("album","song",db.list("album/"+id,0,INT_MAX));
    else if (route=="getPlaylist") group("playlist","entry",db.list("playlist/"+id,0,INT_MAX));
    else if (route=="getSong") subsonic(reply,property("song",db.object("song",id)));
    else if (route=="getMusicFolders") group("musicFolders","musicFolder",db.list("folder",0,INT_MAX));
    else if (route=="getIndexes") group("indexes","index","[{\"artist\":"+db.list("folder/"+param(request,"musicFolderId"),0,INT_MAX)+"}]");
    else if (route=="getGenres") group("genres","genre",db.list("genre",0,INT_MAX));
    else if (route=="getPlaylists") group("playlists","playlist",db.list("playlist",0,INT_MAX));
    else if (route=="getArtists") group("artists","index","[{\"artist\":"+db.list("albumartist",0,INT_MAX)+"}]");
    else if (route=="getArtist") group("artist","album",db.list("artist/"+id,0,INT_MAX));
    else if (route=="getMusicDirectory") group("directory","child",db.list("directory/"+id,0,INT_MAX));
    else if (route=="getAlbumList2") group("albumList2","album",db.list(param(request,"type")=="byGenre"?
        "genre/"+param(request,"genre"):"album",index(request,"offset",0),index(request,"size",10)));
    else if (route=="search3") {
        std::string values;
        for (std::string kind:{"album","artist","song"}) {
            if (!values.empty()) values+=',';
            values+=property(kind,db.list(kind,index(request,(kind+"Offset").c_str(),0),
                index(request,(kind+"Count").c_str(),20),param(request,"query")));
        }
        subsonic(reply,property("searchResult3","{"+values+"}"));
    } else if (route=="getLyricsBySongId") {
        group("lyricsList","structuredLyrics",db.lyrics(id));
    } else if (route=="getLyrics") {
        auto variants=db.lyrics("",param(request,"title"));
        simdjson::dom::parser parser;
        std::string plain;
        for(auto variant:parser.parse(variants).get_array().value())
            for(auto line:variant["line"].get_array().value()) plain+=field(line,"value")+"\n";
        group("lyrics","value",jsonString(plain));
    }
    else subsonic(reply,"\"error\":{\"code\":70,\"message\":\"Resource is outside the screenshot fixture\"}",false);
}
} // namespace

void serve(const std::filesystem::path& file,const std::string& user,const std::string& password) {
    Catalog catalog(file);
    httplib::Server http;
    http.set_keep_alive_max_count(1);
    auto token=tangerine::randomHex();
    http.new_task_queue=[] {return new httplib::ThreadPool(12);};
    http.Post("/auth/login",[&](const Request& request,Reply& reply) {
        simdjson::dom::parser parser;
        Element body;
        if (parser.parse(request.body).get(body) || field(body,"username")!=user || field(body,"password")!=password)
            reply.status=401;
        else json(reply,"{"+property("token",jsonString(token))+"}");
    });
    http.Get(R"(/api/(album|artist|playlist|song|pose))",[&](const Request& request,Reply& reply) {
        if (request.get_header_value("x-nd-authorization")!="Bearer "+token) {reply.status=401;return;}
        std::string kind=request.matches[1];
        if (kind=="pose") return json(reply,"{"+property("album",jsonString(catalog.pose("album")))+","+
            property("song",jsonString(catalog.pose("song")))+","+property("playlist",jsonString(catalog.pose("playlist")))+","+
            property("albums",std::to_string(catalog.total("album")))+","+property("filter",jsonString(catalog.pose("filter")))+","+
            property("albumStart",catalog.pose("albumStart"))+","+property("albumShowcase",catalog.list("album-showcase",0,INT_MAX))+","+
            property("bentoSeek",catalog.pose("bentoSeek"))+","+
            property("albumQuery",jsonString(catalog.pose("albumQuery")))+","+property("albumMatches",std::to_string(catalog.total("album-search")))+","+
            property("favoriteAlbums",catalog.list("starred/album",0,INT_MAX))+","+
            property("queue",catalog.queue())+"}");
        int start=index(request,"_start",0),end=index(request,"_end",100),total=0;
        auto filter=param(request,kind=="song"?"title":kind=="playlist"?"q":"name");
        if (kind=="song" && lower(filter)==lower(catalog.pose("filter"))) { kind="filtered-tracks"; filter.clear(); }
        if (kind=="album" && lower(filter)==lower(catalog.pose("albumQuery"))) { kind="album-search"; filter.clear(); }
        if (kind=="artist" && param(request,"role")=="albumartist") kind="albumartist";
        auto page=catalog.list(kind,start,std::clamp(end-start,0,500),filter,&total);
        reply.set_header("x-total-count",std::to_string(total)); json(reply,page);
    });
    http.Get(R"(/rest/([A-Za-z0-9]+)\.view)",[&](const Request& request,Reply& reply) {
        auto salt=param(request,"s");
        if (param(request,"u")!=user || salt.empty() || param(request,"t")!=tangerine::md5(password+salt))
            return subsonic(reply,"\"error\":{\"code\":40,\"message\":\"Wrong credentials\"}",false);
        rest(catalog,request,reply);
    });
    int port=http.bind_to_any_port("127.0.0.1");
    if (port<=0) throw Error("Could not bind a loopback port.");
    std::cout<<port<<std::endl;
    if (!http.listen_after_bind()) throw Error("The loopback listener failed.");
}
} // namespace demo

int wmain(int argc,wchar_t** argv) {
    try {
        if (argc!=3) throw tangerine::Error("Usage: imposter-navidrome serve|refresh <database> (username and password on stdin)");
        std::string user,password;
        if (!std::getline(std::cin,user) || !std::getline(std::cin,password) || user.empty() || password.empty())
            throw tangerine::Error("Missing credentials on standard input.");
        if (std::wstring_view(argv[1])==L"refresh") demo::exportLibrary(argv[2],user,password);
        else if (std::wstring_view(argv[1])==L"serve") demo::serve(argv[2],user,password);
        else throw tangerine::Error("Unknown imposter-navidrome command.");
        return 0;
    } catch (const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
