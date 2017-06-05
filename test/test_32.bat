@echo off
if exist config.bat call config.bat
SETLOCAL
pushd ..
set _ROOT=%CD%
set mode=Release
pushd external\reshade
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
msbuild ReShade.sln /property:Configuration=%mode% "/property:Platform=32-bit"
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake --build . --config %mode%
popd

@echo on
copy /y external\reshade\bin\Win32\%mode%\ReShade32.dll "%gamedir32%\d3d9.dll"
copy /y external\reshade\bin\Win32\%mode%\ReShade32.dll "bin\32\%mode%\d3d9.dll"
copy /y bin\32\%mode%\ActWebSocketImguiOverlay.dll "%gamedir32%\%name32%_mod.dll"
copy /y bin\32\%mode%\ActWebSocketImguiOverlayWithLua.dll "%gamedir32%\%name32%_mod.dll"

ENDLOCAL
popd
