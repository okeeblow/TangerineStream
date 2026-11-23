#pragma once

#include "model.hpp"
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <atomic>
#include <memory>
#include <utility>

namespace tangerine {

using Parameters = std::vector<std::pair<std::string, std::string>>;

// Owns the upstream response, including the handles needed to read its body.
class Response {
public:
    Response(const std::string& url, const wchar_t* method, const std::wstring& headers,
             const std::string& body = {});
    ~Response();
    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    std::string header(DWORD id) const;
    std::string header(const wchar_t* name) const;
    DWORD read(void* buffer, DWORD size);
    std::string text();
    DWORD status = 0;

private:
    struct Close {
        void operator()(void* handle) const { WinHttpCloseHandle(handle); }
    };
    std::unique_ptr<void, Close> session_, connection_, request_;
};

class ServerApi {
public:
    virtual ~ServerApi() = default;
    virtual std::vector<Track> songs(const std::string& query, int offset) = 0;
    virtual std::vector<Track> favorites(const std::atomic_bool& stopping) = 0;
    virtual std::unique_ptr<Response> audio(const Track& track, const std::string& range,
                                            bool head) = 0;
    virtual void submit(const std::vector<Report>& reports) = 0;
};

std::unique_ptr<ServerApi> connect(const Server& server);
std::string normalizeUrl(std::string url);

} // namespace tangerine
