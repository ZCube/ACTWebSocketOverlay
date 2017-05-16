@echo off
pushd ..
pushd src
call revision.bat
if %errorlevel% neq 0 exit /b %errorlevel%
popd
call configure.bat
if %errorlevel% neq 0 exit /b %errorlevel%
call build.bat
if %errorlevel% neq 0 exit /b %errorlevel%

if not exist dist\temp mkdir dist\temp

xcopy /hrkysd "bin\64\Release\ActWebSocketImguiOverlay.dll" "dist\temp\ACTWebSocketOverlay64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "bin\32\Release\ActWebSocketImguiOverlay.dll" "dist\temp\ACTWebSocketOverlay32.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "external\reshade\bin\x64\Release\ReShade64.dll" "dist\temp\ReShade64.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "external\reshade\bin\Win32\Release\ReShade32.dll" "dist\temp\ReShade32.*"
if %errorlevel% neq 0 exit /b %errorlevel%

popd

if exist ACTWebSocketOverlay_latest.7z del ACTWebSocketOverlay_latest.7z
pushd temp
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_latest.7z *
if %errorlevel% neq 0 exit /b %errorlevel%
popd temp

if not exist dist\ffxiv mkdir dist\ffxiv

xcopy /hrkysd "bin\64\Release\ActWebSocketImguiOverlay.dll" "dist\ffxiv\ffxiv_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "bin\32\Release\ActWebSocketImguiOverlay.dll" "dist\ffxiv\ffxiv_dx11_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "external\reshade\bin\x64\Release\ReShade64.dll" "dist\ffxiv\dxgi.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "external\reshade\bin\Win32\Release\ReShade32.dll" "dist\ffxiv\d3d9.*"
if %errorlevel% neq 0 exit /b %errorlevel%

popd

if exist ACTWebSocketOverlay_ffxiv_latest.7z del ACTWebSocketOverlay_ffxiv_latest.7z
pushd ffxiv
"c:\Program Files\7-Zip\7z.exe" a ..\ACTWebSocketOverlay_ffxiv_latest.7z *
if %errorlevel% neq 0 exit /b %errorlevel%
popd ffxiv


:SetVariables
FOR /F "tokens=3 delims= " %%G IN ('REG QUERY "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal"') DO (SET DocumentDir=%%G)
if exist "%APPDATA%\Dropbox\info.json" SET DROP_INFO=%APPDATA%\Dropbox\info.json
if exist "%LOCALAPPDATA%\Dropbox\info.json" SET DROP_INFO=%LOCALAPPDATA%\Dropbox\info.json
for /f "tokens=*" %%a in ( 'powershell -NoProfile -ExecutionPolicy Bypass -Command "( ( Get-Content -Raw -Path %DROP_INFO% | ConvertFrom-Json ).personal.path)" ' ) do set dropBoxRoot=%%a
echo %dropBoxRoot%

if not exist "%dropBoxRoot%\share" mkdir "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_latest.7z" /exclude:exclude_files.txt "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd  "%CD%\ACTWebSocketOverlay_ffxiv_latest.7z" /exclude:exclude_files.txt "%dropBoxRoot%\share"
if %errorlevel% neq 0 exit /b %errorlevel%
