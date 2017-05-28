@echo off
if exist config.bat call config.bat
pushd ..
pushd src
call revision.bat
if %errorlevel% neq 0 exit /b %errorlevel%
popd
REM call configure.bat
REM if %errorlevel% neq 0 exit /b %errorlevel%
REM call build.bat
REM if %errorlevel% neq 0 exit /b %errorlevel%

if not exist dist\temp mkdir dist\temp
if not exist dist\ffxiv mkdir dist\ffxiv
if not exist dist\temp\32 mkdir dist\temp\32
if not exist dist\ffxiv\32 mkdir dist\ffxiv\32
if not exist dist\temp\64 mkdir dist\temp\64
if not exist dist\ffxiv\64 mkdir dist\ffxiv\64
popd

xcopy /hrkysd /exclude:exclude_files.txt "..\bin\64\Release\*.dll" "temp\64\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd /exclude:exclude_files.txt "..\bin\32\Release\*.dll" "temp\32\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\64\Release\ActWebSocketImguiOverlay.dll" "temp\64\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\ActWebSocketImguiOverlay.dll" "temp\32\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%


xcopy /hrkysd "..\bin\64\Release\loader.dll" "temp\ACTWebSocketOverlay64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\loader.dll" "temp\ACTWebSocketOverlay32.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\x64\Release\ReShade64.dll" "temp\ReShade64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\Win32\Release\ReShade32.dll" "temp\ReShade32.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\resource\overlay_atlas.*" "temp\overlay_atlas.*"
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy /hrkysd "..\bin\64\Release\*.dll" /exclude:exclude_files.txt "ffxiv\64\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\*.dll" /exclude:exclude_files.txt "ffxiv\32\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\64\Release\ActWebSocketImguiOverlay.dll" "ffxiv\64\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\ActWebSocketImguiOverlay.dll" "ffxiv\32\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy /hrkysd "..\bin\64\Release\loader.dll" "ffxiv\ffxiv_dx11_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\loader.dll" "ffxiv\ffxiv_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\x64\Release\ReShade64.dll" "ffxiv\dxgi.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\Win32\Release\ReShade32.dll" "ffxiv\d3d9.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\resource\overlay_atlas.*" "ffxiv\overlay_atlas.*"
if %errorlevel% neq 0 exit /b %errorlevel%

if exist ACTWebSocketOverlay_latest.zip del ACTWebSocketOverlay_latest.zip
pushd temp
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_latest.zip *
if %errorlevel% neq 0 exit /b %errorlevel%
popd temp

if exist ACTWebSocketOverlay_ffxiv_latest.zip del ACTWebSocketOverlay_ffxiv_latest.zip
pushd ffxiv
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_ffxiv_latest.zip *
if %errorlevel% neq 0 exit /b %errorlevel%
popd ffxiv

set /p version=<..\src\version
set /p tag=<..\src\tag
SET OWNER=ZCube
SET REPO=ACTWebSocketOverlay

xcopy /hrkyd ACTWebSocketOverlay_latest.zip ACTWebSocketOverlay_%version%.*
xcopy /hrkyd ACTWebSocketOverlay_ffxiv_latest.zip ACTWebSocketOverlay_ffxiv_%version%.*

echo ==========================================================================================
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_%version%.zip
@py -2 github_uploader.py %GITHUB_TOKEN% %OWNER% %REPO% %tag% ACTWebSocketOverlay_ffxiv_%version%.zip
echo ==========================================================================================


:SetVariables
FOR /F "tokens=3 delims= " %%G IN ('REG QUERY "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal"') DO (SET DocumentDir=%%G)
if exist "%APPDATA%\Dropbox\info.json" SET DROP_INFO=%APPDATA%\Dropbox\info.json
if exist "%LOCALAPPDATA%\Dropbox\info.json" SET DROP_INFO=%LOCALAPPDATA%\Dropbox\info.json
for /f "tokens=*" %%a in ( 'powershell -NoProfile -ExecutionPolicy Bypass -Command "( ( Get-Content -Raw -Path %DROP_INFO% | ConvertFrom-Json ).personal.path)" ' ) do set dropBoxRoot=%%a
echo %dropBoxRoot%

if not exist "%dropBoxRoot%\share" mkdir "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_latest.zip" /exclude:exclude_files.txt "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_ffxiv_latest.zip" /exclude:exclude_files.txt "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
copy /y "%CD%\..\src\version" "%dropBoxRoot%\share\ACTWebSocketOverlay_version"
if %errorlevel% neq 0 exit /b %errorlevel%