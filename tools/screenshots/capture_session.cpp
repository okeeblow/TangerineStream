#include "capture_host.hpp"
#include <objbase.h>
#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <regex>

namespace capture {
namespace {
constexpr auto sessionMutex=L"Local\\TangerineStream-Screenshots";
constexpr auto marker=L".capture-workspace";
bool workspaceName(const Path& path) {
    static const std::wregex pattern(L"TangerineShots-[0-9a-f]{32}");
    return std::regex_match(path.filename().wstring(),pattern) && fs::equivalent(path.parent_path(),fs::temp_directory_path());
}
bool ownedRun(const Path& path) {
    const auto attr=GetFileAttributesW(path.c_str());
    return workspaceName(path) && attr!=INVALID_FILE_ATTRIBUTES && !(attr&FILE_ATTRIBUTE_REPARSE_POINT) &&
           fs::is_regular_file(path/marker) && readFile(path/marker)=="Tangerine Stream";
}
void removeRun(const Path& run) {
    if (!fs::exists(run)) return;
    if (!ownedRun(run)) throw std::runtime_error("Unrecognized capture workspace.");
    for (int attempt=0;;++attempt) {
        try {removeOwnedTree(run,fs::temp_directory_path());return;}
        catch (const fs::filesystem_error&) {if (attempt==12) throw;Sleep(250);}
    }
}
class SessionLock {
    Handle mutex_;
public:
    explicit SessionLock(DWORD timeout) : mutex_(checked(CreateMutexW(nullptr,FALSE,sessionMutex),"CreateMutex")) {
        auto state=WaitForSingleObject(mutex_.get(),timeout);
        if (state!=WAIT_OBJECT_0 && state!=WAIT_ABANDONED) throw std::runtime_error("Another screenshot run is active.");
    }
    ~SessionLock() {ReleaseMutex(mutex_.get());}
};
Path scriptRoot(const Path& script) {
    if (script.filename()!=L"regenerate-screenshots.bat" || !fs::is_regular_file(script))
        throw std::runtime_error("Expected the repository screenshot script.");
    return fs::canonical(script).parent_path();
}
std::wstring newName() {
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid))) throw std::runtime_error("Could not name the private desktop.");
    wchar_t buffer[39];
    StringFromGUID2(guid,buffer,39);
    std::wstring name=L"TangerineShots-";
    for (wchar_t c:std::wstring(buffer)) if (iswxdigit(c)) name+=static_cast<wchar_t>(towlower(c));
    return name;
}
// A detached guard survives closure of the launching console. The inherited
// process handle still identifies this supervisor even if Windows reuses its PID.
void startGuard(const Path& script,const Path& run) {
    wchar_t module[32768];
    if (!GetModuleFileNameW(nullptr,module,32768)) winError("GetModuleFileName");
    HANDLE inherited;
    if (!DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),GetCurrentProcess(),&inherited,SYNCHRONIZE,TRUE,0))
        winError("Duplicate supervisor handle");
    Handle owner(inherited);
    auto args=quoted(module)+L" cleanup "+std::to_wstring(reinterpret_cast<uintptr_t>(inherited))+L" "+quoted(script)+L" "+quoted(run);
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION child{};
    if (!CreateProcessW(module,args.data(),nullptr,nullptr,TRUE,DETACHED_PROCESS|CREATE_NEW_PROCESS_GROUP,nullptr,nullptr,&startup,&child))
        winError("Start screenshot cleanup guard");
    CloseHandle(child.hThread);CloseHandle(child.hProcess);
}
void finishCleanup(const Path& root,const Path& run) {
    try {restoreGallery(root);}
    catch (...) {removeRun(run);throw;}
    removeRun(run);
}
struct DesktopClose {void operator()(void* desktop) const {CloseDesktop(static_cast<HDESK>(desktop));}};
class Worker {
    std::unique_ptr<void,DesktopClose> desktop_;
    Handle job_,process_;
public:
    Worker(const std::wstring& name,const Path& script,const Path& run,const std::wstring& compiler,bool refresh)
      : desktop_(CreateDesktopW(name.c_str(),nullptr,nullptr,0,GENERIC_ALL,nullptr)),
        job_(checked(CreateJobObjectW(nullptr,nullptr),"Create capture job")) {
        if (!desktop_) winError("Create private desktop");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job_.get(),JobObjectExtendedLimitInformation,&limits,sizeof(limits))) winError("Set capture job limits");
        if (!SetEnvironmentVariableW(L"TS_SHOT_WORKER",name.c_str()) || !SetEnvironmentVariableW(L"TS_SHOT_RUN",run.c_str()))
            winError("Set capture environment");
        SECURITY_ATTRIBUTES inherit{sizeof(inherit),nullptr,TRUE};
        auto log=checked(CreateFileW((run/L"worker-output.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,&inherit,CREATE_ALWAYS,0,nullptr),"Open worker log");
        Handle input;
        if (refresh) {
            HANDLE copy;
            if (!DuplicateHandle(GetCurrentProcess(),GetStdHandle(STD_INPUT_HANDLE),GetCurrentProcess(),&copy,0,TRUE,DUPLICATE_SAME_ACCESS))
                winError("Connect password pipe");
            input.reset(copy);
        } else input=checked(CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&inherit,OPEN_EXISTING,0,nullptr),"Open NUL");
        auto shell=Path(_wgetenv(L"ComSpec"));
        BOOL wow64=FALSE;
        if (IsWow64Process(GetCurrentProcess(),&wow64) && wow64)
            shell=Path(_wgetenv(L"SystemRoot"))/L"Sysnative/cmd.exe";
        auto command=L"cmd.exe /d /c \""+quoted(script)+L" WACUP "+compiler+(refresh?L" rebuild-db":L"")+L" --no-pause\"";
        std::wstring desktop=L"WinSta0\\"+name;
        STARTUPINFOW startup{sizeof(startup)};
        startup.lpDesktop=desktop.data();startup.dwFlags=STARTF_USESTDHANDLES;
        startup.hStdInput=input.get();startup.hStdOutput=startup.hStdError=log.get();
        PROCESS_INFORMATION child{};
        if (!CreateProcessW(shell.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW,
                            nullptr,script.parent_path().c_str(),&startup,&child)) winError("Start private screenshot worker");
        process_.reset(child.hProcess);Handle thread(child.hThread);
        if (!AssignProcessToJobObject(job_.get(),process_.get()) || ResumeThread(thread.get())==DWORD(-1)) {
            TerminateProcess(process_.get(),1);winError("Start worker in capture job");
        }
    }
    ~Worker() {
        TerminateJobObject(job_.get(),1);
        WaitForSingleObject(process_.get(),5000);
    }
    void wait(HANDLE owner,const Path& run) {
        HANDLE waitFor[]{process_.get(),owner};
        std::streamoff offset=0;
        for (;;) {
            auto state=WaitForMultipleObjects(2,waitFor,FALSE,250);
            std::ifstream log(run/L"progress",std::ios::binary);
            if (log && log.seekg(offset)) {
                std::string more{std::istreambuf_iterator<char>(log),{}};
                if (!more.empty()) {std::cout<<more<<std::flush;offset+=more.size();}
            }
            if (state==WAIT_OBJECT_0) break;
            if (state==WAIT_OBJECT_0+1) throw std::runtime_error("The screenshot launcher closed.");
            if (state!=WAIT_TIMEOUT) winError("Wait for screenshot worker");
        }
        DWORD code=1;
        if (!GetExitCodeProcess(process_.get(),&code) || code || !fs::is_regular_file(run/L"complete")) {
            std::cerr<<readFile(run/L"worker-output.log");
            throw std::runtime_error("The private screenshot worker failed.");
        }
    }
};
void runSession(const Path& script,const std::wstring& compiler,const std::wstring& action,DWORD ownerPid) {
    if ((compiler!=L"msys2" && compiler!=L"vs") || (action!=L"capture" && action!=L"refresh"))
        throw std::runtime_error("Invalid screenshot target.");
    auto owner=checked(OpenProcess(SYNCHRONIZE,FALSE,ownerPid),"Open screenshot launcher");
    SessionLock lock(0);
    auto root=scriptRoot(script);
    restoreGallery(root);
    for (const auto& entry:fs::directory_iterator(fs::temp_directory_path())) if (ownedRun(entry.path())) removeRun(entry.path());
    auto name=newName();
    auto run=fs::temp_directory_path()/name;
    startGuard(script,run);
    if (!fs::create_directory(run)) throw std::runtime_error("Could not create the capture workspace.");
    writeFile(run/marker,"Tangerine Stream");
    try {
        for (const auto* dir:{L"shots",L"Roaming",L"Local",L"Temp"}) fs::create_directory(run/dir);
        {Worker worker(name,script,run,compiler,action==L"refresh");worker.wait(owner.get(),run);}
        publish(root,run,action==L"refresh");
    } catch (...) {finishCleanup(root,run);throw;}
    finishCleanup(root,run);
    std::cout<<(action==L"refresh"?"Updated the demo database.":"Saved screenshots to "+pathText(root/L"screenshots"))<<'\n';
}
} // namespace
} // namespace capture

int wmain(int argc,wchar_t** argv) {
    using namespace capture;
    _setmode(_fileno(stdout),_O_BINARY);
    try {
        if (argc==6 && std::wstring_view(argv[1])==L"run") runSession(argv[2],argv[3],argv[4],std::stoul(argv[5]));
        else if (argc==5 && std::wstring_view(argv[1])==L"cleanup") {
            auto owner=Handle(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(std::stoull(argv[2]))));
            if (WaitForSingleObject(owner.get(),INFINITE)!=WAIT_OBJECT_0) winError("Wait for capture cleanup");
            SessionLock lock(INFINITE);
            finishCleanup(scriptRoot(argv[3]),argv[4]);
        } else if (argc==5 && std::wstring_view(argv[1])==L"unpack") {
            if (!ownedRun(argv[2])) throw std::runtime_error("Unrecognized WACUP workspace.");
            installWacup(argv[2],argv[3],argv[4]);
        } else throw std::runtime_error("Start screenshots with regenerate-screenshots.bat.");
        return 0;
    } catch (const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
