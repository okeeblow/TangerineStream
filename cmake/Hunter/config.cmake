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
