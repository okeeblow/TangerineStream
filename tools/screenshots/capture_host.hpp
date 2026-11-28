#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

namespace capture {
namespace fs = std::filesystem;
using Path = fs::path;
inline std::string pathText(const Path& path) {
    const auto bytes=path.u8string();
    return {bytes.begin(),bytes.end()};
}
struct Close { void operator()(void* handle) const { CloseHandle(handle); } };
using Handle = std::unique_ptr<void, Close>;
inline void winError(const char* operation) { throw std::system_error(GetLastError(),std::system_category(),operation); }
inline Handle checked(HANDLE value,const char* operation) {
    if (!value || value==INVALID_HANDLE_VALUE) winError(operation);
    return Handle(value);
}
inline std::wstring quoted(const Path& path) { return L"\""+path.wstring()+L"\""; }
inline std::string readFile(const Path& file) {
    std::ifstream in(file,std::ios::binary);
    if (!in) throw std::runtime_error("Could not read "+pathText(file));
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
inline void writeFile(const Path& file,const std::string& bytes) {
    std::ofstream out(file,std::ios::binary);
    out.exceptions(std::ios::badbit|std::ios::failbit);
    out.write(bytes.data(),bytes.size());
    out.close();
}
void installWacup(const Path& run,const Path& installer,const Path& sevenZip);
void removeOwnedTree(const Path& path,const Path& parent);
void restoreGallery(const Path& root);
void publish(const Path& root,const Path& run,bool refresh);
} // namespace capture
