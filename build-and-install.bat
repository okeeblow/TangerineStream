@echo off
call "%~dp0build.bat" --install %*
exit /b %errorlevel%
