@echo off
set _ROOT=%CD%
if not exist build\64 mkdir build\64
pushd build\64
cmake -G "Visual Studio 14 2015 Win64" %_ROOT%
popd

if not exist build\32 mkdir build\32
pushd build\32
cmake -G "Visual Studio 14 2015" %_ROOT%
popd

