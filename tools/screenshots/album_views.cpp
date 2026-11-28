#include "demo.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>

namespace demo {
namespace {
struct Album {
    Entry entry;
    std::wstring title;
    int plays, songs, year;
    bool eligible;
};
using Metric = int Album::*;
using Row = const Album*;
struct Window {
    const char* key;
    Metric metric;
    int start;
    std::vector<Row> albums;
};

bool before(const Album& a,const Album& b,Metric metric) {
    if (metric && a.*metric!=b.*metric) return a.*metric>b.*metric;
    return CompareStringEx(LOCALE_NAME_USER_DEFAULT,NORM_IGNORECASE|SORT_DIGITSASNUMBERS,
        a.title.c_str(),-1,b.title.c_str(),-1,nullptr,nullptr,0)==CSTR_LESS_THAN;
}
int number(Element node,const char* fieldName) {
    int64_t value=0;
    return node[fieldName].get_int64().get(value) ? 0 : static_cast<int>(value);
}
Album albumFrom(const Entry& entry,bool favorite) {
    simdjson::dom::parser parser;
    auto node=parser.parse(entry.data).value();
    int year=number(node,"releaseYear");
    if (!year) year=number(node,"year");
    if (!year) year=number(node,"maxYear");
    auto credit=tangerine::wide(entry.name+" "+field(node,"albumArtist")+" "+field(node,"artist"));
    CharLowerBuffW(credit.data(),static_cast<DWORD>(credit.size()));
    const int plays=number(node,"playCount");
    return {entry,tangerine::wide(entry.name),plays,number(node,"songCount"),year,
            plays>=2 && !favorite && !entry.cover.empty() && credit.find(L"death in june")==credit.npos};
}
} // namespace

void addAlbumViews(Fixture& fixture,const std::vector<Entry>& library,std::set<std::string>& covers) {
    auto& root=fixture.collections.at("album");
    const auto& stars=fixture.collections.at("starred/album").members;
    const std::set<ObjectKey> favorites(stars.begin(),stars.end());
    std::set<ObjectKey> retained(root.members.begin(),root.members.end());
    std::map<std::string,Album> albums;
    auto load=[&](const Entry& entry) {
        ObjectKey key{"album",entry.id};
        auto stored=fixture.objects.find(key);
        const auto& payload=stored==fixture.objects.end()?entry:stored->second;
        albums.emplace(entry.id,albumFrom(payload,favorites.count(key)!=0));
    };
    for (const auto& entry:library) load(entry);
    for (const auto& key:root.members) if (!albums.count(key.second)) load(fixture.objects.at(key));

    Window classicModern{"playCount",&Album::plays,std::stoi(fixture.poses.at("albumStart")),{}};
    std::set<std::string> usedTitles;
    for (const auto& key:fixture.collections.at("album-showcase").members) {
        const auto& album=albums.at(key.second);
        usedTitles.insert(album.entry.name);
        classicModern.albums.push_back(&album);
    }
    std::vector<Window> windows{classicModern};
    // All four grids have four columns. Bento and WACUP Classic also expose part of a third row.
    struct Pose {const char* key; Metric metric; int percent, tiles;};
    for (const Pose pose: {Pose{"name",nullptr,27,12},Pose{"songCount",&Album::songs,51,12},
                           Pose{"releaseYear",&Album::year,74,8}}) {
        std::vector<Row> ranked;
        for (const auto& [id,album]:albums) ranked.push_back(&album);
        std::stable_sort(ranked.begin(),ranked.end(),[&](Row a,Row b){return before(*a,*b,pose.metric);});
        // Keep the real ranks for the scrollbar. Only played albums and required fixture
        // records can enter the photographed grid after the unpictured rows are compressed.
        std::vector<int> candidates;
        for (int i=0;i<static_cast<int>(ranked.size());++i)
            if (ranked[i]->eligible || retained.count({"album",ranked[i]->entry.id})) candidates.push_back(i);
        Window window{pose.key,pose.metric,0,{}};
        int bestDistance=std::numeric_limits<int>::max();
        const int target=static_cast<int>(ranked.size())*pose.percent/100;
        for (int i=0;i+pose.tiles<=static_cast<int>(candidates.size());++i) {
            const int distance=std::abs(candidates[i]-target);
            if (distance>=bestDistance) continue;
            const int start=candidates[i]/4*4;
            if (std::any_of(windows.begin(),windows.end(),[&](const Window& previous) {
                return std::abs(start-previous.start)<std::max(pose.tiles,root.total/20);
            })) continue;
            std::set<std::string> titles;
            std::vector<Row> visible;
            for (int j=i;j<i+pose.tiles;++j) {
                auto album=ranked[candidates[j]];
                if (!album->eligible || (pose.metric && album->*pose.metric<=0) ||
                    usedTitles.count(album->entry.name) || !titles.insert(album->entry.name).second) break;
                visible.push_back(album);
            }
            if (static_cast<int>(visible.size())!=pose.tiles) continue;
            bestDistance=distance;
            window.start=start;
            window.albums=std::move(visible);
        }
        if (window.albums.empty()) throw Error("No distinct played album window for "+std::string(pose.key)+".");
        for (auto album:window.albums) {
            const auto& entry=album->entry;
            ObjectKey key{"album",entry.id};
            if (retained.insert(key).second) root.members.push_back(key);
            fixture.objects.emplace(key,entry);
            covers.insert(entry.cover);
            usedTitles.insert(entry.name);
        }
        windows.push_back(std::move(window));
    }
    std::string views="{";
    for (const auto& window:windows) {
        int earlier=0;
        const Album& first=*window.albums.front();
        for (const auto& key:root.members) if (before(albums.at(key.second),first,window.metric)) ++earlier;
        const int padding=window.start-earlier;
        if (padding<0 || padding>root.total-static_cast<int>(root.members.size()))
            throw Error("Could not position the "+std::string(window.key)+" album grid.");
        if (window.metric==&Album::plays) root.leadingBlanks=padding;
        if (views.size()>1) views+=',';
        views+=jsonString(window.key)+":{\"start\":"+std::to_string(window.start)+",\"padding\":"+std::to_string(padding)+",\"titles\":[";
        for (size_t i=0;i<window.albums.size();++i) {
            if (i) views+=',';
            views+=jsonString(window.albums[i]->entry.name);
        }
        views+="]}";
        std::cout<<window.key<<": album "<<window.start+1<<" of "<<root.total<<" ("<<first.entry.name<<")\n";
    }
    fixture.poses["albumWindows"]=views+"}";
}
} // namespace demo
