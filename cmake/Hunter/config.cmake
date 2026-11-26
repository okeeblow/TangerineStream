hunter_config(simdjson
    VERSION 5.3.0
    URL "https://github.com/simdjson/simdjson/archive/refs/tags/v5.3.0.tar.gz"
    SHA1 e08ea28bb6d401ed9b93f6512548dc5bf21afe38
    CMAKE_ARGS
        BUILD_SHARED_LIBS=OFF
        SIMDJSON_ENABLE_THREADS=OFF
        CMAKE_POLICY_DEFAULT_CMP0091=NEW
        CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>
)

hunter_config(date
    VERSION 3.0.5
    URL "https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.5.tar.gz"
    SHA1 94cb7b3427d24fe52949aab149976a622314b68b
    CMAKE_ARGS BUILD_TZ_LIB=OFF
)

hunter_config(uriparser
    VERSION 1.0.3
    URL "https://github.com/uriparser/uriparser/archive/refs/tags/uriparser-1.0.3.tar.gz"
    SHA1 4efa4d737e25d7b9d0eb60d87ce7e16cf8afde2a
    CMAKE_ARGS
        URIPARSER_SHARED_LIBS=OFF
        URIPARSER_MSVC_STATIC_CRT=ON
        URIPARSER_BUILD_WCHAR_T=OFF
        URIPARSER_BUILD_DOCS=OFF
        URIPARSER_BUILD_TESTS=OFF
        URIPARSER_BUILD_TOOLS=OFF
)
