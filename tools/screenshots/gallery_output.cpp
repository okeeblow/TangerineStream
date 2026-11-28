#include "capture_host.hpp"
#include <set>

namespace capture {
namespace {
void plainDirectory(const Path& path) {
    DWORD attributes=GetFileAttributesW(path.c_str());
    if (attributes==INVALID_FILE_ATTRIBUTES || !(attributes&FILE_ATTRIBUTE_DIRECTORY) || (attributes&FILE_ATTRIBUTE_REPARSE_POINT))
        throw std::runtime_error("Expected an ordinary directory: "+pathText(path));
}
bool pngName(const std::string& name) {
    return name.size()>4 && name.compare(name.size()-4,4,".png")==0 &&
        name.substr(0,name.size()-4).find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==std::string::npos;
}
void installFile(const Path& input,const Path& output,const Path& backup) {
    if (fs::exists(output)) {
        if (!ReplaceFileW(output.c_str(),input.c_str(),backup.c_str(),0,nullptr,nullptr)) winError("ReplaceFile");
    } else if (!MoveFileExW(input.c_str(),output.c_str(),MOVEFILE_WRITE_THROUGH)) winError("MoveFileEx");
}
} // namespace

void removeOwnedTree(const Path& path,const Path& parent) {
    if (!fs::exists(path)) return;
    if (fs::canonical(parent)!=fs::canonical(path.parent_path())) throw std::runtime_error("Unexpected cleanup path.");
    plainDirectory(path);
    // A junction inside a capture workspace belongs to the workspace; its target does not.
    for (const auto& entry:fs::directory_iterator(path)) {
        if (entry.path().filename()==L".capture-workspace") continue;
        DWORD attributes=GetFileAttributesW(entry.path().c_str());
        if (attributes==INVALID_FILE_ATTRIBUTES) winError("GetFileAttributes");
        if (attributes&FILE_ATTRIBUTE_READONLY) SetFileAttributesW(entry.path().c_str(),attributes&~FILE_ATTRIBUTE_READONLY);
        if ((attributes&FILE_ATTRIBUTE_DIRECTORY) && !(attributes&FILE_ATTRIBUTE_REPARSE_POINT))
            removeOwnedTree(entry.path(),path);
        else fs::remove(entry.path());
    }
    fs::remove(path/L".capture-workspace");
    fs::remove(path);
}

void restoreGallery(const Path& root) {
    const Path stage=root/L"build-screenshots-publish", gallery=root/L"screenshots";
    if (!fs::exists(stage)) return;
    plainDirectory(stage);
    if (fs::exists(stage/L"new-files") && !fs::exists(stage/L"complete")) {
        plainDirectory(gallery);
        plainDirectory(stage/L"backup");
        std::ifstream journal(stage/L"new-files");
        for (std::string name;std::getline(journal,name);) {
            if (!name.empty() && name.back()=='\r') name.pop_back();
            if (!pngName(name)) throw std::runtime_error("Invalid gallery publish journal.");
            fs::remove(gallery/name);
        }
        for (const auto& entry:fs::directory_iterator(stage/L"backup")) {
            const auto name=entry.path().filename();
            if (!pngName(pathText(name))) throw std::runtime_error("Invalid gallery backup.");
            fs::copy_file(entry.path(),gallery/name,fs::copy_options::overwrite_existing);
        }
    }
    removeOwnedTree(stage,root);
}

void publish(const Path& root,const Path& run,bool refresh) {
    const Path stage=root/L"build-screenshots-publish", gallery=root/L"screenshots";
    fs::create_directory(stage);
    try {
        if (refresh) {
            fs::copy_file(run/L"demo.sqlite3",stage/L"demo.sqlite3");
            installFile(stage/L"demo.sqlite3",root/L"tools/screenshots/demo.sqlite3",stage/L"previous.sqlite3");
        } else {
            fs::create_directories(gallery);
            plainDirectory(gallery);
            fs::create_directory(stage/L"ready");
            fs::create_directory(stage/L"backup");
            std::set<std::string> captured;
            std::string newFiles;
            for (const auto& entry:fs::directory_iterator(run/L"shots")) {
                const auto name=pathText(entry.path().filename());
                if (!pngName(name)) throw std::runtime_error("Unexpected screenshot filename.");
                fs::copy_file(entry.path(),stage/L"ready"/name);
                captured.insert(name);
                if (!fs::exists(gallery/name)) newFiles+=name+"\n";
            }
            if (captured.empty()) throw std::runtime_error("The capture produced no screenshots.");
            writeFile(stage/L"new-files.tmp",newFiles);
            fs::rename(stage/L"new-files.tmp",stage/L"new-files");
            for (const auto& name:captured) installFile(stage/L"ready"/name,gallery/name,stage/L"backup"/name);
            for (const auto* name:{"layouts.png","classic-modern-folders.png","classic-modern-folder-contents.png",
                    "classic-modern-folder-contents-library.png","classic-modern-folders-library.png","classic-modern-playlist-tracks.png"}) {
                if (!captured.count(name) && fs::exists(gallery/name)) fs::rename(gallery/name,stage/L"backup"/name);
            }
            writeFile(stage/L"complete","ok");
        }
    } catch (...) {restoreGallery(root);throw;}
    restoreGallery(root);
}
} // namespace capture
