@echo off
set _ROOT=%CD%
if not exist build\64 mkdir build\64
pushd build\64
cmake --build . --config Release
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake --build . --config Release
popd

