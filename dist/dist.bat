@echo off
if exist config.bat call config.bat
call prepare_ssl.bat

set /p version=<..\src\version
set /p tag=<..\src\tag
SET OWNER=ZCube
SET REPO=ACTWebSocketOverlay

echo ==========================================================================================
REM @py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_ssl_%version%.zip
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_lua_ssl_%version%.zip
REM @py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_ffxiv_ssl_%version%.zip
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_ffxiv_lua_ssl_%version%.zip
echo ==========================================================================================
