@echo off
if exist config.bat call config.bat
SETLOCAL
pushd ..
set _ROOT=%CD%
set mode=Release
pushd external\reshade
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
msbuild ReShade.sln /property:Configuration=%mode% "/property:Platform=64-bit"
popd

if not exist build\64 mkdir build\64
pushd build\64
cmake --build . --config %mode%
popd

@echo on
copy /y external\reshade\bin\x64\%mode%\ReShade64.dll "%gamedir64%\dxgi.dll"
copy /y external\reshade\bin\x64\%mode%\ReShade64.dll "bin\64\%mode%\dxgi.dll"
copy /y bin\64\%mode%\ActWebSocketImguiOverlay.dll "%gamedir64%\%name64%_mod.dll"
copy /y bin\64\%mode%\ActWebSocketImguiOverlayWithLua.dll "%gamedir64%\%name64%_mod.dll"

ENDLOCAL
popd
