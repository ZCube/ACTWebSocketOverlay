@echo off
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
set _ROOT=%CD%

xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\64\* bin\64\Release\*
xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\64\* bin\64\Debug\*
xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\32\* bin\32\Release\*
xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\32\* bin\32\Debug\*


if not exist build\64 mkdir build\64
pushd build\64
cmake -G "Visual Studio 15 2017 Win64" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/64" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64"
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake -G "Visual Studio 15 2017" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/32" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32"
popd

