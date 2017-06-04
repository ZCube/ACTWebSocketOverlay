@echo off
if exist config.bat call config.bat
call prepare_ssl.bat
call prepare_nonssl.bat

set /p version=<..\src\version
set /p tag=<..\src\tag
SET OWNER=ZCube
SET REPO=ACTWebSocketOverlay

echo ==========================================================================================
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_nonssl_%version%.zip
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_lua_nonssl_%version%.zip
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_ffxiv_nonssl_%version%.zip
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_ffxiv_lua_nonssl_%version%.zip
echo ==========================================================================================

:SetVariables
FOR /F "tokens=3 delims= " %%G IN ('REG QUERY "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal"') DO (SET DocumentDir=%%G)
if exist "%APPDATA%\Dropbox\info.json" SET DROP_INFO=%APPDATA%\Dropbox\info.json
if exist "%LOCALAPPDATA%\Dropbox\info.json" SET DROP_INFO=%LOCALAPPDATA%\Dropbox\info.json
for /f "tokens=*" %%a in ( 'powershell -NoProfile -ExecutionPolicy Bypass -Command "( ( Get-Content -Raw -Path %DROP_INFO% | ConvertFrom-Json ).personal.path)" ' ) do set dropBoxRoot=%%a
echo %dropBoxRoot%

if not exist "%dropBoxRoot%\share" mkdir "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_lua_nonssl_latest.zip" /exclude:exclude_files.txt "%dropBoxRoot%\share\ACTWebSocketOverlay_latest.zip"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_ffxiv_lua_nonssl_latest.zip" /exclude:exclude_files.txt "%dropBoxRoot%\share\ACTWebSocketOverlay_ffxiv_latest.zip"
if %errorlevel% neq 0 exit /b %errorlevel%
copy /y "%CD%\..\src\version" "%dropBoxRoot%\share\ACTWebSocketOverlay_version"
if %errorlevel% neq 0 exit /b %errorlevel%
