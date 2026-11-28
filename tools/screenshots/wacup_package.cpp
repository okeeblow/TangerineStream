#include "capture_host.hpp"
#include <bcrypt.h>
#include <wincrypt.h>
#include <winhttp.h>
#include <cstdint>
#include <cstring>
#include <regex>

namespace capture {
namespace {
std::string digest(const std::string& bytes,const wchar_t* algorithm) {
    BYTE hash[32]; DWORD size=sizeof(hash);
    if (!CryptHashCertificate2(algorithm,0,nullptr,reinterpret_cast<const BYTE*>(bytes.data()),
                              static_cast<DWORD>(bytes.size()),hash,&size)) winError("CryptHashCertificate2");
    return {reinterpret_cast<char*>(hash),size};
}
std::wstring sha256(const Path& file) {
    constexpr wchar_t hex[]=L"0123456789ABCDEF";
    std::wstring result;
    for (unsigned char byte:digest(readFile(file),BCRYPT_SHA256_ALGORITHM)) {
        result+=hex[byte>>4];result+=hex[byte&15];
    }
    return result;
}
struct CloseInternet {void operator()(void* handle) const {WinHttpCloseHandle(handle);}};
using Internet=std::unique_ptr<void,CloseInternet>;
void download(const std::wstring& route,const Path& file) {
    Internet session(WinHttpOpen(L"Tangerine Stream screenshot setup",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,nullptr,nullptr,0));
    if (!session) winError("WinHttpOpen");
    Internet origin(WinHttpConnect(session.get(),L"getwacup.com",INTERNET_DEFAULT_HTTPS_PORT,0));
    if (!origin) winError("WinHttpConnect");
    Internet request(WinHttpOpenRequest(origin.get(),L"GET",route.c_str(),nullptr,nullptr,nullptr,WINHTTP_FLAG_SECURE));
    if (!request || !WinHttpSendRequest(request.get(),nullptr,0,nullptr,0,0,0) || !WinHttpReceiveResponse(request.get(),nullptr))
        winError("Download WACUP component");
    DWORD status=0,size=sizeof(status);
    if (!WinHttpQueryHeaders(request.get(),WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&size,nullptr) || status!=200)
        throw std::runtime_error("WACUP component download returned HTTP "+std::to_string(status));
    std::ofstream out(file,std::ios::binary);
    out.exceptions(std::ios::failbit|std::ios::badbit);
    char buffer[65536]; DWORD count=0;
    do {
        if (!WinHttpReadData(request.get(),buffer,sizeof(buffer),&count)) winError("Read WACUP component");
        out.write(buffer,count);
    } while (count);
}
// These are 7-Zip arguments, not shell commands. Archive entries can include wildcards.
Handle sevenZipProcess(const Path& executable,const std::wstring& arguments,HANDLE output) {
    SECURITY_ATTRIBUTES access{sizeof(access),nullptr,TRUE};
    auto nul=checked(CreateFileW(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&access,OPEN_EXISTING,0,nullptr),"Open NUL");
    STARTUPINFOW startup{sizeof(startup)};
    startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdInput=startup.hStdError=nul.get();startup.hStdOutput=output?output:nul.get();
    PROCESS_INFORMATION child{};
    auto command=quoted(executable)+L" "+arguments;
    if (!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child))
        winError("Start 7-Zip");
    CloseHandle(child.hThread);
    return Handle(child.hProcess);
}
void extract(const Path& sevenZip,const Path& file,const Path& to,const std::wstring& entries,bool flat=false) {
    auto command=std::wstring(flat?L"e":L"x")+L" -y -o"+quoted(to)+L" "+quoted(file)+L" "+entries;
    auto process=sevenZipProcess(sevenZip,command,nullptr);
    DWORD code=1;
    if (WaitForSingleObject(process.get(),INFINITE)!=WAIT_OBJECT_0 || !GetExitCodeProcess(process.get(),&code) || code)
        throw std::runtime_error("Could not unpack "+pathText(file)+" (7-Zip "+std::to_string(code)+").");
}
uint32_t number(const std::string& bytes,size_t position,size_t width=4) {
    if (position>bytes.size() || width>bytes.size()-position || width>4) throw std::runtime_error("Incomplete WACUP package data.");
    uint32_t value=0;
    for (size_t i=0;i<width;++i) value|=uint32_t(static_cast<unsigned char>(bytes[position+i]))<<(8*i);
    return value;
}
std::wstring manifest(const Path& run,const Path& installer,const Path& sevenZip) {
    const auto bytes=readFile(installer);
    size_t offset=8;
    while (offset+25<bytes.size() && bytes.compare(offset,12,"NullsoftInst")!=0) offset+=512;
    if (offset+25>=bytes.size()) throw std::runtime_error("Unrecognized WACUP NSIS installer.");
    const auto length=number(bytes,offset+12);
    if (length<16 || length>32*1024*1024 || length%2) throw std::runtime_error("Invalid WACUP package header size.");
    const auto archive=run/L"installer-header.lzma";
    writeFile(archive,bytes.substr(offset+20,5)+std::string(8,'\xff')+bytes.substr(offset+25));
    SECURITY_ATTRIBUTES access{sizeof(access),nullptr,TRUE};
    HANDLE reader,writer;
    if (!CreatePipe(&reader,&writer,&access,0)) winError("CreatePipe");
    Handle input(reader),output(writer);
    if (!SetHandleInformation(reader,HANDLE_FLAG_INHERIT,0)) winError("SetHandleInformation");
    auto proc=sevenZipProcess(sevenZip,L"x -so -tlzma "+quoted(archive),writer);
    output.reset();
    std::string header(4+length,'\0');
    size_t used=0;
    while (used<header.size()) {
        DWORD count=0;
        if (!ReadFile(reader,header.data()+used,static_cast<DWORD>(header.size()-used),&count,nullptr) || !count) break;
        used+=count;
    }
    // The rest of the solid archive is the application itself. Only its header is needed here.
    TerminateProcess(proc.get(),0);WaitForSingleObject(proc.get(),INFINITE);
    if (used!=header.size() || number(header,0)!=length) throw std::runtime_error("Could not decode the WACUP package header.");
    std::wstring result(length/sizeof(wchar_t),L'\0');
    std::memcpy(result.data(),header.data()+4,length);
    return result;
}
Path component(const Path& run,const std::wstring& header,const wchar_t* filename,const std::wstring& route) {
    if (const auto* local=_wgetenv(L"LOCALAPPDATA")) {
        auto cached=Path(local)/L"WACUP"/filename;
        if (fs::is_regular_file(cached) && header.find(sha256(cached))!=header.npos) return cached;
    }
    auto file=run/filename;
    download(route,file);
    if (header.find(sha256(file))==header.npos) throw std::runtime_error("Incorrect checksum for "+pathText(file.filename()));
    return file;
}
// Decode the copy and insert records in the VPatch deltas shipped by WACUP.
void applyDelta(const Path& file,const Path& delta) {
    const auto input=readFile(file),patch=readFile(delta);
    if (number(patch,0)!=0x54415056 || number(patch,4)!=0x80000001 || patch.size()<48)
        throw std::runtime_error("Invalid WACUP skin-support patch.");
    auto hash=digest(input,BCRYPT_MD5_ALGORITHM);
    if (hash==patch.substr(28,16)) return;
    if (hash!=patch.substr(12,16)) throw std::runtime_error("Incorrect original "+pathText(file));
    size_t pos=48;
    std::string result;
    for (uint32_t i=0;i<number(patch,8);++i) {
        auto op=number(patch,pos++,1);
        if (op==255) {pos+=8;continue;}
        if (op<1 || op>7 || op==4) throw std::runtime_error("Invalid VPatch operation.");
        const auto width=op%4==3?4:op%4;
        auto count=number(patch,pos,width);pos+=width;
        auto from=op<4?number(patch,pos):pos;
        const auto& source=op<4?input:patch;
        if (from>source.size() || count>source.size()-from) throw std::runtime_error("Invalid VPatch range.");
        result.append(source,from,count);
        pos+=op<4?4:count;
    }
    if (digest(result,BCRYPT_MD5_ALGORITHM)!=patch.substr(28,16)) throw std::runtime_error("Incorrect patched "+pathText(file));
    writeFile(file,result);
}
void patchClassicFrame(const Path& file) {
    const auto xml=readFile(file);
    const std::regex group(R"xml(<groupdef\b[^>]*\bid="wasabi\.standardframe\.nostatusbar"[^>]*>)xml",std::regex::icase);
    std::sregex_iterator found(xml.begin(),xml.end(),group),end;
    if (found==end) throw std::runtime_error("Missing Classic Modern window-frame group.");
    const auto match=*found;
    if (++found!=end) throw std::runtime_error("Ambiguous Classic Modern frame group.");
    auto tag=match.str();
    if (tag.find("rectrgn=\"1\"")!=tag.npos) return;
    std::regex attribute(R"xml(\brectrgn="[^"]*")xml");
    tag=std::regex_search(tag,attribute)?std::regex_replace(tag,attribute,"rectrgn=\"1\""):tag.insert(tag.size()-1," rectrgn=\"1\"");
    auto fixed=xml;fixed.replace(match.position(),match.length(),tag);writeFile(file,fixed);
    if (readFile(file).find(tag)==std::string::npos) throw std::runtime_error("Classic Modern frame patch failed.");
}
} // namespace

void installWacup(const Path& run,const Path& installer,const Path& sevenZip) {
    const auto header=manifest(run,installer,sevenZip);
    std::wsmatch link;
    if (!std::regex_search(header,link,std::wregex(LR"(https://getwacup\.com(/files/wacup_modern_skins_[0-9]+\.7z))")))
        throw std::runtime_error("The installer did not identify a Modern skin archive.");
    const auto skins=component(run,header,L"wacup_default_modern_skins.7z",link[1].str());
    const auto redux=component(run,header,L"winamp_5666_full_redux_files.7z",L"/get_redux.php");
    const auto app=run/L"WACUP", profile=run/L"profile", plugins=app/L"Plugins";
    extract(sevenZip,installer,app,LR"(*.dll wacup.exe winamp.exe Microsoft.VC*.CRT System Plugins\in_wave.dll Plugins\in_wav.dll Plugins\gen_lyrics.dll Plugins\ml_ll.dll Plugins\ml_playlists.dll Plugins\ml_bookmarks.dll Plugins\ml_history.dll Plugins\gen_ff.patch)");
    extract(sevenZip,redux,app,LR"(Plugins\gen_ff.dll Plugins\freeform Microsoft.VC90.CRT)");
    extract(sevenZip,skins,app/L"Skins",LR"("Winamp Classic Modern" "Big Bento Modern")");
    extract(sevenZip,skins,plugins,L"freeform");
    extract(sevenZip,installer,profile,LR"(*\winamp.q1)",true);
    extract(sevenZip,installer,profile/L"Plugins/vis_classic",LR"(*\Plugins\vis_classic\*.ini)",true);
    extract(sevenZip,installer,plugins,LR"(*\vis_classic.dll)",true);
    applyDelta(plugins/L"gen_ff.dll",plugins/L"gen_ff.patch");
    auto font=plugins/L"freeform/wacs/freetype";
    applyDelta(font/L"freetype.wac",font/L"freetype.patch");
    patchClassicFrame(app/L"Skins/Winamp Classic Modern/xml/standardframe.xml");
    for (auto path:{app/L"wacup.exe",app/L"Skins/Big Bento Modern/skin.xml",profile/L"winamp.q1",plugins/L"vis_classic.dll"})
        if (!fs::is_regular_file(path)) throw std::runtime_error("Portable WACUP is missing "+pathText(path));
}
} // namespace capture
