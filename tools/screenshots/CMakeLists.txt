# Only the demo service needs an HTTP server. Hunter caches its header-only release.
hunter_private_data(
    URL "https://codeload.github.com/yhirose/cpp-httplib/tar.gz/refs/tags/v0.18.7"
    SHA1 4da8f9964b05a3fd13f7ad2588b667547ac74a68
    LOCATION httplib_source
)
add_executable(imposter-navidrome imposter-navidrome.cpp export.cpp fixture.cpp "${PROJECT_SOURCE_DIR}/src/server.cpp")
target_include_directories(imposter-navidrome PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/generated")
target_include_directories(imposter-navidrome SYSTEM PRIVATE "${httplib_source}")
target_link_libraries(imposter-navidrome PRIVATE simdjson::simdjson uriparser::uriparser tangerine_migrations winhttp ws2_32 crypt32 bcrypt)

target_compile_features(imposter-navidrome PRIVATE cxx_std_17)
target_compile_definitions(imposter-navidrome PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN WINVER=0x0601 _WIN32_WINNT=0x0601)
set_target_properties(imposter-navidrome PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(MINGW)
    target_link_options(imposter-navidrome PRIVATE -municode -static -static-libgcc -static-libstdc++)
elseif(MSVC)
    set_property(TARGET imposter-navidrome PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_library(out_screenshot MODULE screenshot-output.cpp)
target_include_directories(out_screenshot SYSTEM PRIVATE "${sdk_directory}")
target_compile_features(out_screenshot PRIVATE cxx_std_17)
target_compile_definitions(out_screenshot PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
set_target_properties(out_screenshot PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(MINGW)
    target_link_options(out_screenshot PRIVATE -static -static-libgcc -static-libstdc++)
elseif(MSVC)
    set_property(TARGET out_screenshot PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_custom_target(screenshot_tools DEPENDS wacup_plugins imposter-navidrome out_screenshot)
