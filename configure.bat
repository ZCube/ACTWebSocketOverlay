@echo off
set _ROOT=%CD%

if not exist build\64 mkdir build\64
pushd build\64
cmake -G "Visual Studio 14 2015 Win64" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/64" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64"
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake -G "Visual Studio 14 2015" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/32" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32"
popd

