@echo off
SETLOCAL
REM CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
set _ROOT=%CD%

if not exist build\64 mkdir build\64
pushd build\64
cmake -G "Visual Studio 15 2017 Win64" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/64" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64" ^
	"-DUSE_SSL=ON"
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake -G "Visual Studio 15 2017" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/32" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32" ^
	"-DUSE_SSL=ON"
popd

ENDLOCAL
