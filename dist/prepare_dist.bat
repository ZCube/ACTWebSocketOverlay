if not exist %dest%                    mkdir %dest%
if not exist %dest%\sample_scripts     mkdir %dest%\sample_scripts

IF /I "%use_ssl%"=="FALSE" (
    GOTO NON_DEPENDENCY
)
:DEPENDENCY
if not exist %dest%\32                 mkdir %dest%\32
if not exist %dest%\64                 mkdir %dest%\64

rem dependencies
xcopy /hrkysd /exclude:exclude_files.txt "..\bin\64\Release\*.dll" "%dest%\64\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd /exclude:exclude_files.txt "..\bin\32\Release\*.dll" "%dest%\32\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%

rem loader
xcopy /hrkysd "..\bin\64\Release\mod_loader_64.dll" "%dest%\%overlay64dst%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\mod_loader_32.dll" "%dest%\%overlay32dst%"
if %errorlevel% neq 0 exit /b %errorlevel%

rem module
xcopy /hrkysd "..\bin\64\Release\%overlay64src%" "%dest%\64\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\%overlay32src%" "%dest%\32\overlay_mod.*"
if %errorlevel% neq 0 exit /b %errorlevel%
GOTO LAST

:NON_DEPENDENCY
rem module
xcopy /hrkysd "..\bin\64\Release\%overlay64src%" "%dest%\%overlay64dst%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\bin\32\Release\%overlay32src%" "%dest%\%overlay32dst%"
if %errorlevel% neq 0 exit /b %errorlevel%

:LAST
rem reshade
xcopy /hrkysd "..\external\reshade\bin\x64\Release\ReShade64.dll" "%dest%\%reshade64dst%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /hrkysd "..\external\reshade\bin\Win32\Release\ReShade32.dll" "%dest%\%reshade32dst%"
if %errorlevel% neq 0 exit /b %errorlevel%

IF /I "%use_atlas%"=="FALSE" (
    GOTO NON_ATLAS
)
:ATLAS
rem resource
xcopy /hrkysd "..\resource\overlay_atlas.*" "%dest%\overlay_atlas.*"
if %errorlevel% neq 0 exit /b %errorlevel%
GOTO LAST_RESOURCE

:NON_ATLAS
if not exist %dest%\images             mkdir %dest%\images
rem resource
xcopy /hrkysd "..\textures\images\*.png" "%dest%\images\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%

:LAST_RESOURCE
xcopy /hrkysd "..\sample_scripts\*.*" "%dest%\sample_scripts\*.*"
if %errorlevel% neq 0 exit /b %errorlevel%


if exist %dest%_latest.zip del %dest%_latest.zip
pushd %dest%
..\archiver_windows_386 make ..\%dest%_latest.zip .
if %errorlevel% neq 0 exit /b %errorlevel%
popd %dest%

set /p version=<..\src\version
set /p tag=<..\src\tag
SET OWNER=ZCube
SET REPO=ACTWebSocketOverlay

xcopy /hrkyd %dest%_latest.zip %dest%_%version%.*
