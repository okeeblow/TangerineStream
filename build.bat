@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "ts_toolchain=msys2"
set "ts_target=wacup_plugins"
set "ts_screenshots=OFF"
set "ts_no_pause="
set "ts_pushed="
set "ts_exit=1"

:arguments
if "%~1"=="" goto start
if /i "%~1"=="msys2" (
    set "ts_toolchain=msys2"
) else if /i "%~1"=="vs" (
    set "ts_toolchain=vs"
) else if /i "%~1"=="--install" (
    set "ts_target=install"
) else if /i "%~1"=="--screenshots" (
    set "ts_screenshots=ON"
) else if /i "%~1"=="--screenshot-host" (
    set "ts_screenshots=ON"
    set "ts_target=screenshot-host"
) else if /i "%~1"=="--no-pause" (
    set "ts_no_pause=1"
) else (
    echo Usage: %~nx0 [msys2^|vs] [--install] [--screenshots^|--screenshot-host] [--no-pause]
    goto finish
)
shift
goto arguments

:start
pushd "%~dp0" || goto finish
set "ts_pushed=1"
if /i "%ts_toolchain%"=="vs" goto setup_vs

if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
set "ts_mingw=%MSYS2_ROOT%\mingw32\bin"
if not exist "%ts_mingw%\g++.exe" (
    echo The MSYS2 32-bit compiler was not found in "%ts_mingw%".
    echo In the MSYS2 MINGW32 shell, run: pacman -S --needed mingw-w64-i686-gcc
    goto finish
)
set "MSYSTEM=MINGW32"
set "PATH=%ts_mingw%;%PATH%"
set "CC=%ts_mingw%\gcc.exe"
set "CXX=%ts_mingw%\g++.exe"
where /q cmake.exe
if errorlevel 1 goto missing_cmake
for %%I in (ninja.exe) do set "ts_ninja=%%~$PATH:I"
if not defined ts_ninja (
    echo Ninja was not found on PATH.
    echo In the MSYS2 MINGW32 shell, run: pacman -S --needed mingw-w64-i686-ninja
    goto finish
)
set "ts_build=build-mingw32"
if "%ts_screenshots%"=="ON" set "ts_build=build-screenshots-mingw32"
set "ts_parallel=--parallel"
set "ts_mingw_cmake=%ts_mingw:\=/%"
set "ts_ninja_cmake=%ts_ninja:\=/%"
cmake.exe -S . -B "%ts_build%" -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    "-DCMAKE_C_COMPILER=%ts_mingw_cmake%/gcc.exe" ^
    "-DCMAKE_CXX_COMPILER=%ts_mingw_cmake%/g++.exe" "-DCMAKE_RC_COMPILER=%ts_mingw_cmake%/windres.exe" ^
    "-DCMAKE_MAKE_PROGRAM=%ts_ninja_cmake%" -DTANGERINE_SCREENSHOTS=%ts_screenshots%
goto build

:setup_vs
set "ts_vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%ts_vswhere%" set "ts_vswhere=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
set "ts_vsdir="
if exist "%ts_vswhere%" (
    for /f "usebackq delims=" %%I in (`"%ts_vswhere%" -products * -property installationPath`) do (
        for /d %%T in ("%%I\VC\Tools\MSVC\14.44.*") do (
            if not defined ts_vsdir if exist "%%T\bin\Hostx86\x86\cl.exe" set "ts_vsdir=%%I"
        )
    )
)
if not defined ts_vsdir (
    echo Install the MSVC v143 14.44 x64/x86 build tools in Visual Studio Installer.
    goto finish
)
call "%ts_vsdir%\VC\Auxiliary\Build\vcvars32.bat" -vcvars_ver=14.44
if errorlevel 1 goto finish
rem Hunter locates v143 through the VS 2022 tools variable even inside a newer IDE.
set "VS170COMNTOOLS=%ts_vsdir%\Common7\Tools\"
set "CC=cl.exe"
set "CXX=cl.exe"
where /q cmake.exe
if errorlevel 1 goto missing_cmake
set "ts_build=build-vs-v143"
if "%ts_screenshots%"=="ON" set "ts_build=build-screenshots-vs-v143"
set "ts_parallel="
cmake.exe -S . -B "%ts_build%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DTANGERINE_SCREENSHOTS=%ts_screenshots%
goto build

:missing_cmake
echo CMake was not found on PATH.
goto finish

:build
if errorlevel 1 (
    set "ts_exit=%errorlevel%"
    goto finish
)
if "%ts_screenshots%:%ts_target%"=="ON:wacup_plugins" set "ts_target=screenshot_tools"
cmake.exe --build "%ts_build%" --config Release %ts_parallel% --target %ts_target%
set "ts_exit=%errorlevel%"
if not "%ts_exit%"=="0" goto finish
echo Built %ts_target% in "%ts_build%".

:finish
if defined ts_pushed popd
if not "%ts_exit%"=="0" echo Build or installation failed.
if not defined ts_no_pause pause
endlocal & exit /b %ts_exit%
