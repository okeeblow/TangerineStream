cmake_minimum_required(VERSION 3.20...3.27)

if(NOT DEFINED ENV{HUNTER_ROOT} AND DEFINED ENV{USERPROFILE})
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}/.hunter" default_hunter_root)
    set(HUNTER_ROOT "${default_hunter_root}" CACHE PATH "Hunter package cache")
endif()
set(HUNTER_USE_CACHE_SERVERS NO CACHE STRING "Use Hunter remote binary caches")
include(cmake/HunterGate.cmake)
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.26.12.tar.gz"
    SHA1 6498c5d0dec25d7fffb2b0574ebaf265179b894d
    LOCAL
)
project(ml_tangerinestream VERSION 0.6.5 LANGUAGES C CXX RC)

if(MSVC_VERSION GREATER_EQUAL 1950)
    message(FATAL_ERROR "The v145 runtime requires Windows 10. Select MSVC v143 to target Windows 7.")
endif()

hunter_add_package(date)
find_package(date CONFIG REQUIRED)
hunter_add_package(simdjson)
find_package(simdjson CONFIG REQUIRED)
hunter_add_package(uriparser)
find_package(uriparser CONFIG REQUIRED)

# SQLite publishes its amalgamation without a CMake project. Let Hunter cache
# the official source archive and compile it with the plug-in's toolchain.
include(hunter_private_data)
hunter_private_data(
    URL "https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip"
    SHA1 dbe0fa9aaeac8ad29fc3e094792896d556a57163
    LOCATION sqlite_source
)
add_library(tangerine_sqlite STATIC "${sqlite_source}/sqlite3.c")
target_include_directories(tangerine_sqlite PUBLIC "${sqlite_source}")
target_compile_definitions(tangerine_sqlite PRIVATE WINVER=0x0601 _WIN32_WINNT=0x0601 SQLITE_OMIT_LOAD_EXTENSION SQLITE_ENABLE_FTS5)
if(MSVC)
    set_property(TARGET tangerine_sqlite PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(WINAMP_SDK "" CACHE PATH "Winamp SDK directory override")
set(sdk_directory "${WINAMP_SDK}")
if(NOT sdk_directory)
    set(sdk_directory "$ENV{WACUP_SDK_HOME}")
endif()
if(NOT sdk_directory)
    set(sdk_directory "${CMAKE_CURRENT_SOURCE_DIR}/SDKs/Winamp-5.666")
endif()
file(TO_CMAKE_PATH "${sdk_directory}" sdk_directory)
cmake_path(ABSOLUTE_PATH sdk_directory BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE)
foreach(header IN ITEMS gen_ml/ml.h gen_ml/ml_ipc_0313.h Winamp/wa_ipc.h Winamp/in2.h)
    if(NOT EXISTS "${sdk_directory}/${header}")
        message(FATAL_ERROR "Missing ${header} in ${sdk_directory}. Set WACUP_SDK_HOME or WINAMP_SDK to a Winamp SDK directory.")
    endif()
endforeach()
message(STATUS "Using Winamp SDK at ${sdk_directory}")

set(WACUP_PLUGINS_DIR "" CACHE PATH "WACUP Plugins directory (empty to detect the installed copy)")
set(plugin_destination "${WACUP_PLUGINS_DIR}")
if(NOT plugin_destination)
    set(plugin_destination "$ENV{WACUP_PLUGINS_DIR}")
endif()

if(NOT plugin_destination AND CMAKE_HOST_WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    foreach(hive IN ITEMS HKCU HKLM)
        set(uninstall_key "${hive}/SOFTWARE/Microsoft/Windows/CurrentVersion/Uninstall/WACUP")
        cmake_host_system_information(RESULT wacup_dir QUERY WINDOWS_REGISTRY
            "${uninstall_key}" VALUE InstallLocation VIEW 32)
        if(NOT wacup_dir OR NOT IS_DIRECTORY "${wacup_dir}/Plugins")
            cmake_host_system_information(RESULT wacup_icon QUERY WINDOWS_REGISTRY
                "${uninstall_key}" VALUE DisplayIcon VIEW 32)
            string(REGEX REPLACE ",[ \t]*-?[0-9]+$" "" wacup_exe "${wacup_icon}")
            string(REPLACE "\"" "" wacup_exe "${wacup_exe}")
            get_filename_component(wacup_dir "${wacup_exe}" DIRECTORY)
        endif()
        if(wacup_dir AND IS_DIRECTORY "${wacup_dir}/Plugins")
            set(plugin_destination "${wacup_dir}/Plugins")
            break()
        endif()
    endforeach()
endif()

if(NOT plugin_destination)
    foreach(program_files IN ITEMS "$ENV{ProgramFiles\(x86\)}" "$ENV{ProgramFiles}")
        if(program_files AND IS_DIRECTORY "${program_files}/WACUP/Plugins")
            set(plugin_destination "${program_files}/WACUP/Plugins")
            break()
        endif()
    endforeach()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    install(CODE [[message(FATAL_ERROR "Use a 32-bit build to install into WACUP.")]])
elseif(NOT plugin_destination)
    install(CODE [[message(FATAL_ERROR "WACUP was not found. Set WACUP_PLUGINS_DIR to its Plugins directory.")]])
else()
    file(TO_CMAKE_PATH "${plugin_destination}" plugin_destination)
    cmake_path(ABSOLUTE_PATH plugin_destination BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE)
    message(STATUS "WACUP plugin install destination: ${plugin_destination}")
endif()

add_custom_target(wacup_plugins ALL)

function(add_wacup_plugin target)
    add_library(${target} MODULE ${ARGN})
    set_target_properties(${target} PROPERTIES PREFIX "")
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_compile_definitions(${target} PRIVATE WINVER=0x0601 _WIN32_WINNT=0x0601 UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_include_directories(${target} SYSTEM PRIVATE "${sdk_directory}")
    if(MINGW)
        target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
        target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>")
    elseif(MSVC)
        set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    add_dependencies(wacup_plugins ${target})
    if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND plugin_destination)
        install(TARGETS ${target} LIBRARY DESTINATION "${plugin_destination}")
    endif()
endfunction()

set_property(SOURCE src/input.rc src/plugin.rc APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/tangerine.ico")
add_wacup_plugin(in_tangerinestream src/input.cpp src/input.rc)
target_compile_definitions(in_tangerinestream PRIVATE UNICODE_INPUT_PLUGIN)
target_link_libraries(in_tangerinestream PRIVATE user32)

add_wacup_plugin(ml_tangerinestream
    src/plugin.cpp src/list_scroll.cpp src/server.cpp src/store.cpp src/playback.cpp src/artwork.cpp src/browse_sort.cpp
    src/host_metadata.cpp src/plugin.rc)
include(cmake/Migrations.cmake)
embed_migrations(ml_tangerinestream)
# The 5.666 package omits the Wasabi art and metadata service headers.
target_include_directories(ml_tangerinestream SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/SDKs/Winamp-5.55/Wasabi"
    "${CMAKE_CURRENT_SOURCE_DIR}/SDKs/Winamp-5.55/Agave")
target_link_libraries(ml_tangerinestream PRIVATE
    date::date simdjson::simdjson uriparser::uriparser tangerine_sqlite comctl32 winhttp crypt32 bcrypt windowscodecs ole32 gdi32 uuid)
