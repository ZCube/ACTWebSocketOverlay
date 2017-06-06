@echo off
pushd ..
pushd src
call revision.bat
if %errorlevel% neq 0 exit /b %errorlevel%
popd
call configure_nonssl.bat
if %errorlevel% neq 0 exit /b %errorlevel%
call build.bat
if %errorlevel% neq 0 exit /b %errorlevel%
popd

set postfix=_nonssl
set use_ssl=FALSE
set use_atlas=TRUE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=ReShade64.*
set reshade32dst=ReShade32.*
set overlay64dst=ACTWebSocketOverlay64.*
set overlay32dst=ACTWebSocketOverlay32.*
set overlay64src=ActWebSocketImguiOverlay.dll
set overlay32src=ActWebSocketImguiOverlay.dll
call prepare_dist.bat

set postfix=_lua_nonssl
set use_ssl=FALSE
set use_atlas=FALSE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=ReShade64.*
set reshade32dst=ReShade32.*
set overlay64dst=ACTWebSocketOverlay64.*
set overlay32dst=ACTWebSocketOverlay32.*
set overlay64src=ActWebSocketImguiOverlayWithLua.dll
set overlay32src=ActWebSocketImguiOverlayWithLua.dll
call prepare_dist.bat

set postfix=_ffxiv_nonssl
set use_ssl=FALSE
set use_atlas=TRUE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=dxgi.*
set reshade32dst=d3d9.*
set overlay64dst=ffxiv_dx11_mod.*
set overlay32dst=ffxiv_mod.*
set overlay64src=ActWebSocketImguiOverlay.dll
set overlay32src=ActWebSocketImguiOverlay.dll
call prepare_dist.bat

set postfix=_ffxiv_lua_nonssl
set use_ssl=FALSE
set use_atlas=FALSE
set dest=ACTWebSocketOverlay%postfix%
set reshade64dst=dxgi.*
set reshade32dst=d3d9.*
set overlay64dst=ffxiv_dx11_mod.*
set overlay32dst=ffxiv_mod.*
set overlay64src=ActWebSocketImguiOverlayWithLua.dll
set overlay32src=ActWebSocketImguiOverlayWithLua.dll
call prepare_dist.bat
