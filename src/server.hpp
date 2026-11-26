#pragma once

#include "model.hpp"
#include <windows.h>
#include <winhttp.h>
#include <atomic>
#include <memory>
#include <list>
#include <mutex>
#include <utility>

namespace tangerine {

using Parameters = std::vector<std::pair<std::string, std::string>>;

bool acceptsCodec(const InputExtensions& inputs, std::string_view codec);

constexpr unsigned originalSource = 1;
struct AudioStream {
    std::string url;
    unsigned choice; // One bit per candidate, for bounded decoder retries.
    std::string format; // Empty for the original; otherwise the output container's file extension.
};

struct CloseInternetHandle {
    void operator()(HINTERNET handle) const { WinHttpCloseHandle(handle); }
};
using InternetHandle = std::unique_ptr<void, CloseInternetHandle>;

class ServerApi;

class HttpClient {
public:
    std::unique_ptr<ServerApi> connect(const Server& server);
private:
    friend class Response;
    std::shared_ptr<void> connection(const std::wstring& host, INTERNET_PORT port, bool secure);
    struct Origin {
        std::wstring host;
        INTERNET_PORT port;
        bool secure;
        std::shared_ptr<void> handle;
    };
    std::mutex mutex_;
    InternetHandle session_;
    std::list<Origin> origins_;
};

// The client outlives its requests. Each response keeps its origin handle alive during eviction.
class Response {
public:
    Response(HttpClient& http, const std::string& url, const wchar_t* method, const std::wstring& headers,
             const std::string& body = {});
    ~Response();
    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    std::string header(DWORD id) const;
    std::string header(const wchar_t* name) const;
    DWORD read(void* buffer, DWORD size);
    std::string text(std::size_t limit = 32 * MiB);
    DWORD status = 0;

private:
    std::shared_ptr<void> connection_;
    InternetHandle request_;
};

class ServerApi {
public:
    virtual ~ServerApi() = default;
    virtual void ping() = 0;
    virtual BrowsePage browse(const BrowseLocation& location, const std::string& query, int offset) = 0;
    virtual std::string cover(const LibraryItem& item, int size) = 0;
    virtual Lyrics lyrics(const Track& track) = 0;
    virtual BrowsePage favorites(const std::atomic_bool& stopping) = 0;
    virtual AudioStream stream(const Track& track, const StreamQuality& quality,
                               const InputExtensions& inputs, unsigned excluded = 0) = 0;
    virtual void submit(const std::vector<Report>& reports) = 0;
};

std::string normalizeUrl(std::string url);
bool sameServerEndpoint(const std::string& savedUrl, const std::string& editedUrl);

} // namespace tangerine
