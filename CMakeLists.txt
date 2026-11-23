cmake_minimum_required(VERSION 3.20)
project(ml_tangerinestream VERSION 0.1.0 LANGUAGES CXX RC)

set(WINAMP_SDK "${CMAKE_CURRENT_SOURCE_DIR}/WA5.55_SDK" CACHE PATH "Nullsoft Winamp SDK")
if(NOT EXISTS "${WINAMP_SDK}/gen_ml/ml.h")
    message(FATAL_ERROR "Set WINAMP_SDK to the extracted Nullsoft Winamp SDK")
endif()

add_library(ml_tangerinestream SHARED
    src/plugin.cpp src/server.cpp src/store.cpp src/playback.cpp src/plugin.rc)
set_target_properties(ml_tangerinestream PROPERTIES PREFIX "")
target_compile_features(ml_tangerinestream PRIVATE cxx_std_17)
target_compile_definitions(ml_tangerinestream PRIVATE
    UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_include_directories(ml_tangerinestream PRIVATE "${WINAMP_SDK}")
target_link_libraries(ml_tangerinestream PRIVATE comctl32 winhttp ws2_32 crypt32 bcrypt)

if(MINGW)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(sqlite_machine i386)
        set(sqlite_def src/winsqlite3-x86.def)
        set(sqlite_options --kill-at)
    else()
        set(sqlite_machine i386:x86-64)
        set(sqlite_def src/winsqlite3.def)
    endif()
    set(sqlite_import "${CMAKE_CURRENT_BINARY_DIR}/libwinsqlite3.a")
    add_custom_command(OUTPUT "${sqlite_import}"
        COMMAND "${CMAKE_DLLTOOL}" -m "${sqlite_machine}" ${sqlite_options}
            -d "${CMAKE_CURRENT_SOURCE_DIR}/${sqlite_def}" -l "${sqlite_import}"
        DEPENDS "${sqlite_def}" VERBATIM)
    add_custom_target(sqlite_import DEPENDS "${sqlite_import}")
    add_dependencies(ml_tangerinestream sqlite_import)
    target_link_libraries(ml_tangerinestream PRIVATE "${sqlite_import}")
    target_link_options(ml_tangerinestream PRIVATE -static -static-libgcc -static-libstdc++)
    target_compile_options(ml_tangerinestream PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>")
else()
    set_property(TARGET ml_tangerinestream PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(ml_tangerinestream PRIVATE winsqlite3)
endif()
