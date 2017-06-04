@echo off
SETLOCAL
CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
set _ROOT=%CD%

pushd prebuilt
py -3 init_prebuilt.py
popd

REM REM libressl
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\libressl\64\* bin\64\Release\*
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\libressl\64\* bin\64\Debug\*
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\libressl\32\* bin\32\Release\*
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\libressl\32\* bin\32\Debug\*

REM openssl
xcopy /hrkyd /exclude:exclude_files.txt prebuilt\openssl\64\bin\* bin\64\Release\*
xcopy /hrkyd /exclude:exclude_files.txt prebuilt\openssl\64\bin\* bin\64\Debug\*
xcopy /hrkyd /exclude:exclude_files.txt prebuilt\openssl\32\bin\* bin\32\Release\*
xcopy /hrkyd /exclude:exclude_files.txt prebuilt\openssl\32\bin\* bin\32\Debug\*

REM REM v8
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\64\* bin\64\Release\*
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\64\* bin\64\Debug\*
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\32\* bin\32\Release\*
REM xcopy /hrkyd /exclude:exclude_files.txt prebuilt\v8\bin\32\* bin\32\Debug\*


if not exist build\64 mkdir build\64
pushd build\64
cmake -G "Visual Studio 15 2017 Win64" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/64" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/64" ^
	"-DUSE_SSL=OFF"
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake -G "Visual Studio 15 2017" %_ROOT% ^
	"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32" ^
	"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=%_ROOT%/lib/32" ^
	"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=%_ROOT%/bin/32" ^
	"-DUSE_SSL=OFF"
popd

ENDLOCAL
