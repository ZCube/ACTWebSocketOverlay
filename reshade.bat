@echo off
"D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe" external\reshade\ReShade.sln /build "Debug|32-bit"
"D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe" external\reshade\ReShade.sln /build "Release|32-bit"
"D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe" external\reshade\ReShade.sln /build "Debug|64-bit"
"D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe" external\reshade\ReShade.sln /build "Release|64-bit"

xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\Win32\Release\ReShade32.dll bin\32\Debug\d3d9.*
xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\Win32\Release\ReShade32.dll bin\32\Debug\dxgi.*
xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\Win32\Release\ReShade32.dll bin\32\Release\d3d9.*
xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\Win32\Release\ReShade32.dll bin\32\Release\dxgi.*
xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\x64\Release\ReShade64.dll bin\64\Debug\d3d9.*
xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\x64\Release\ReShade64.dll bin\64\Debug\dxgi.*
xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\x64\Release\ReShade64.dll bin\64\Release\d3d9.*
xcopy /hrkys /exclude:exclude_files_2.txt external\reshade\bin\x64\Release\ReShade64.dll bin\64\Release\dxgi.*
