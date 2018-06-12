@echo off
SETLOCAL

set _ROOT=%CD%
pushd external\reshade
set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\MSBuild\15.0\Bin;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin;C:\Program Files\dotnet\sdk\NuGetFallbackFolder\microsoft.build.runtime\15.3.409\contentFiles\any\net46;%PATH%
REM CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
msbuild ReShade.sln /property:Configuration=Release "/property:Platform=32-bit"
msbuild ReShade.sln /property:Configuration=Release "/property:Platform=64-bit"
popd

if not exist build\64 mkdir build\64
pushd build\64
cmake --build . --config Release
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake --build . --config Release
popd

ENDLOCAL
