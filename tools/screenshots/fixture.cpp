#include "demo.hpp"
#include <algorithm>
#include <set>

namespace demo {
namespace {
using tangerine::BrowseKind;

// Store each browsable object under its native root and destination kind.
struct CatalogKind {const char* wireName; BrowseKind parent, target;};
constexpr CatalogKind kinds[] = {
    {"album", BrowseKind::albums, BrowseKind::albumTracks},
    {"artist", BrowseKind::artists, BrowseKind::artistAlbums},
    {"genre", BrowseKind::genres, BrowseKind::genreAlbums},
    {"playlist", BrowseKind::playlists, BrowseKind::playlistTracks},
    {"folder", BrowseKind::folders, BrowseKind::folderIndex},
    {"directory", BrowseKind::directory, BrowseKind::directory}
};

void append(std::string& array, const std::string& value) {
    if (array.size()>1) array+=',';
    array+=value;
}
void setting(Database& db, const std::string& name, const std::string& value) {
    Query(db,"INSERT INTO settings(name,value) VALUES(?,?)").bind("imposter."+name).bind(value).next();
}
void saveTrack(Database& db, const Entry& entry, bool favorite) {
    Query(db,R"SQL(WITH source(j) AS (VALUES(?))
        INSERT INTO tracks(server,remote_id,title,artist,album,suffix,mime,duration,favorite,
                           cover_id,album_id,artist_id,album_artist,genre,disc_number,track_number,year)
        SELECT 1,j->>'$.id',j->>'$.title',coalesce(j->>'$.artist',''),coalesce(j->>'$.album',''),
               j->>'$.suffix',j->>'$.contentType',coalesce(j->>'$.duration',0),?,
               coalesce(j->>'$.coverArt',''),coalesce(j->>'$.albumId',''),coalesce(j->>'$.artistId',''),
               coalesce(j->>'$.albumArtist',''),coalesce(j->>'$.genre',''),coalesce(j->>'$.discNumber',0),
               coalesce(j->>'$.trackNumber',j->>'$.track',0),coalesce(j->>'$.year',0) FROM source)SQL")
        .bind(entry.data).bind(favorite).next();
}
void saveCatalogItem(Database& db, const Entry& entry) {
    auto kind=std::find_if(std::begin(kinds),std::end(kinds),[&](const auto& k){return entry.kind==k.wireName;});
    if (kind==std::end(kinds)) throw Error("Unknown screenshot catalog kind "+entry.kind);
    simdjson::dom::parser parser;
    const auto doc=parser.parse(entry.data).value();
    tangerine::SortTags tags;
    for (auto [key,value]:doc.get_object().value()) {
        if (key=="id" || key=="name" || key=="title" || key=="value" || key=="coverArt" ||
            key=="albumArtist" || key=="songCount" || key=="duration") continue;
        tags.emplace(key,value.is_string()?std::string(value.get_string().value()):simdjson::minify(value));
    }
    const int target=static_cast<int>(kind->target);
    Query(db,R"SQL(INSERT INTO catalog(server,parent_kind,parent_id,kind,id,slot,title,subtitle,cover_id,track_key,sort_tags)
        VALUES(1,?,'',?,?,0,?,?,?,0,?))SQL").bind(static_cast<int>(kind->parent)).bind(target)
        .bind(entry.id).bind(entry.name).bind(field(doc,"albumArtist")).bind(entry.cover).bind(tangerine::encodeSortTags(tags)).next();
    Query(db,R"SQL(WITH source(j) AS (VALUES(?)) INSERT INTO catalog_totals(server,kind,id,songs,duration)
       SELECT 1,?,j->>'$.id',coalesce(j->>'$.songCount',-1),coalesce(j->>'$.duration',0)
       FROM source WHERE j->>'$.songCount' IS NOT NULL OR j->>'$.duration' IS NOT NULL)SQL").bind(entry.data).bind(target).next();
}
void loadCatalog(Fixture& fixture, Database& db) {
    Query rows(db,R"SQL(SELECT c.kind,c.id,title,subtitle,cover_id,sort_tags,coalesce(songs,-1),coalesce(duration,0)
        FROM catalog c LEFT JOIN catalog_totals t USING(server,kind,id) WHERE c.server=1)SQL");
    while (rows.next()) {
        auto kind=std::find_if(std::begin(kinds),std::end(kinds),[&](const auto& k){return static_cast<int>(k.target)==rows.number(0);});
        if (kind==std::end(kinds)) throw Error("Unknown item kind in the demo library.");
        Entry e{kind->wireName,rows.bytes(1),rows.bytes(2),rows.bytes(4),{}};
        e.data="{\"id\":"+jsonString(e.id);
        const char* label=e.kind=="genre"?"value":e.kind=="directory"?"title":"name";
        e.data+=","+jsonString(label)+":"+jsonString(e.name);
        if (!e.cover.empty()) e.data+=",\"coverArt\":"+jsonString(e.cover);
        if (!rows.bytes(3).empty()) e.data+=",\"albumArtist\":"+jsonString(rows.bytes(3));
        if (rows.number(6)>=0) e.data+=",\"songCount\":"+rows.bytes(6);
        if (rows.bytes(7)!="0") e.data+=",\"duration\":"+rows.bytes(7);
        for (const auto& [key,value]:tangerine::decodeSortTags(rows.bytes(5))) {
            const bool numeric=key=="year" || key=="maxYear" || key=="albumCount" || key=="playCount" ||
                               key=="discNumber" || key=="track" || key=="trackNumber";
            const bool literal=(numeric && !value.empty() && value.find_first_not_of("0123456789")==value.npos) ||
                               (key=="isDir" && (value=="true" || value=="false"));
            e.data+=","+jsonString(key)+":"+(literal?value:jsonString(value));
        }
        e.data+='}';
        fixture.objects.emplace(ObjectKey{e.kind,e.id},e);
    }
    Query songs(db,R"SQL(SELECT remote_id,title,cover_id,json_object('id',remote_id,'title',title,'artist',artist,
        'album',album,'albumArtist',album_artist,'albumId',album_id,'artistId',artist_id,'genre',genre,'year',year,
        'coverArt',cover_id,'duration',duration,'track',track_number,'trackNumber',track_number,'discNumber',disc_number,
        'suffix',suffix,'contentType',mime) FROM tracks WHERE server=1)SQL");
    while (songs.next()) {
        Entry song{"song",songs.bytes(),songs.bytes(1),songs.bytes(2),songs.bytes(3)};
        fixture.objects.emplace(ObjectKey{song.kind,song.id},song);
    }
}
} // namespace

void Fixture::save(Database& db) const {
    const auto& stars=collections.at("starred/song").members;
    const std::set<ObjectKey> favorites(stars.begin(),stars.end());
    for (const auto& [key,entry]:objects) {
        if (entry.kind=="song") saveTrack(db,entry,favorites.count(key)!=0);
        else saveCatalogItem(db,entry);
    }
    for (const auto& [id,variants]:lyrics) {
        if (variants=="[]") continue;
        // Store the same OpenSubsonic envelope that the plug-in caches.
        Query(db,R"SQL(INSERT INTO lyrics(track,plain,structured,checked)
            SELECT key,'',json_object('subsonic-response',json_object('status','ok','lyricsList',
                json_object('structuredLyrics',json(?)))),1 FROM tracks WHERE server=1 AND remote_id=?)SQL")
            .bind(variants).bind(id).next();
    }
    // These settings describe the simulated server: visible membership, counts for
    // pagination, cover aliases and the screenshot itinerary. Library data lives in the native tables.
    setting(db,"format","1");
    for (const auto& [name,collection]:collections) {
        if (collection.total<static_cast<int>(collection.members.size())) throw Error("Library changed during export: "+name);
        std::string members="[";
        for (const auto& [kind,id]:collection.members) append(members,"["+jsonString(kind)+","+jsonString(id)+"]");
        setting(db,"collection."+name,"{\"total\":"+std::to_string(collection.total)+",\"leading\":"+
                std::to_string(collection.leadingBlanks)+",\"members\":"+members+"]}");
    }
    for (const auto& [name,value]:poses) setting(db,"pose."+name,value);
    for (const auto& [id,digest]:coverAliases) setting(db,"cover."+id,digest);
    std::string queue="[";
    for (const auto& id:playback) append(queue,jsonString(id));
    setting(db,"playback",queue+"]");
}

void Fixture::load(Database& db) {
    Query format(db,"SELECT value FROM settings WHERE name='imposter.format'");
    if (!format.next() || format.bytes()!="1") throw Error("Refresh the demo database with regenerate-screenshots.bat WACUP rebuild-db.");
    loadCatalog(*this,db);
    Query config(db,"SELECT substr(name,10),value FROM settings WHERE name GLOB 'imposter.*'");
    while (config.next()) {
        const auto name=config.bytes(), value=config.bytes(1);
        if (name.rfind("pose.",0)==0) poses.emplace(name.substr(5),value);
        else if (name.rfind("cover.",0)==0) coverAliases.emplace(name.substr(6),value);
        else if (name.rfind("collection.",0)==0) {
            simdjson::dom::parser parser;
            const auto doc=parser.parse(value).value();
            auto& list=collections[name.substr(11)];
            list.total=static_cast<int>(doc["total"].get_int64().value());
            int64_t leading=0;
            if (!doc["leading"].get_int64().get(leading)) list.leadingBlanks=static_cast<int>(leading);
            for (auto item:doc["members"].get_array().value()) {
                ObjectKey key{std::string(item.at(0).get_string().value()),std::string(item.at(1).get_string().value())};
                if (!objects.count(key)) throw Error("Missing library object for "+name);
                list.members.push_back(std::move(key));
            }
        } else if (name=="playback") {
            simdjson::dom::parser parser;
            for (auto id:parser.parse(value).get_array().value()) playback.emplace_back(id.get_string().value());
        }
    }
    Query words(db,R"SQL(SELECT remote_id,json_extract(structured,'$."subsonic-response".lyricsList.structuredLyrics')
        FROM lyrics JOIN tracks ON tracks.key=lyrics.track WHERE server=1)SQL");
    while (words.next()) lyrics.emplace(words.bytes(),words.bytes(1));
}
} // namespace demo
