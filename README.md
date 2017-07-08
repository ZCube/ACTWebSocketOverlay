# ActWebSocketOverlay
ActWebSocket overlay utility based on ImGui

[![Build Status](https://jenkins.zcube.kr/buildStatus/icon?job=ACTWebSocketOverlay)](https://jenkins.zcube.kr/job/ACTWebSocketOverlay/)

*ActWebSocket needed.

## Goal
The goal is to develop overlay that is easy to use and scriptable. 

## Interface
Button or Key | Description
-------- | ----
Shift + F3 | Show/Hide
Gear icon | Show preference window.
Move icon | Click-through.
Ctrl + Alt + Shift | Click-through.
"Name" | Show/Hide names.
Control + Scroll | Zoom.

## Screenshot
![N|Solid](https://raw.githubusercontent.com/ZCube/ACTWebSocketOverlay/master/screenshot.png)

## Font
* Search Path
1. Game Directory
2. Windows/Fonts

Default Setting

Language | File | Description
-------- | ---- | --------
Default | Default | ProggyClean.ttf font by Tristan Grimmer (MIT license).
Japanese | ArialUni.ttf | -
Korean | gulim.ttc | -

## File Overview
Path | Description | License 
---- | ----------- | -------
[/src/awio/overlay.cpp](/src/awio/overlay.cpp) | Overlay Main | BSD License
[/src/awio/sandbox.cpp](/src/awio/sandbox.cpp) | V8 Overlay Sandbox (under development) | BSD License
[/src/awio/main.cpp](/src/awio/main.cpp) | Modified Dear ImGui example for overlay test | MIT License
[/src/awio/main_dx9.cpp](/src/awio/main_dx9.cpp) | Modified Dear ImGui example for overlay test | MIT License

## Used Libraries
Library | Description
------- | -----------
[Boost](https://boost.org) | Boost Library (ASIO, filesystem)
[Beast](https://github.com/vinniefalco/Beast) | WebSocket Library
[ImGui](https://github.com/ocornut/imgui) | Dear ImGui 
[JsonCPP](https://github.com/open-source-parsers/jsoncpp) | Json Library for parsing Message and Settings
[Reshade](https://github.com/crosire/reshade) | Used as overlay injector

## [Installation Guide](/docs/Installation/Installation.md)

## Release
[Latest](https://www.dropbox.com/s/rcypgitu9icz7kp/ACTWebSocketOverlay_latest.zip?dl=1)
[Release](https://github.com/ZCube/ActWebSocketOverlay/releases)

## Build Tool
Microsoft Visual Studio Community 2017

## Build Instruction
* Boost is now built using cmake.  see [boost-cmake](https://github.com/ZCube/boost-cmake)

* Texture build step
    1. cd textures
    2. gen_atlas.bat
    
* ACTWebSocketOverlay build step
    1. configure.bat
    2. build.bat

## Contributors

[Falgern](https://github.com/Falgern/ACTWebSocketOverlay) : I refer to various things such as wrong word usage, implementation of Click-Through with button, and some bug fix.

## Features
* Script Engine ( Lua )
* dear imgui Script binding ( Lua ) Using https://github.com/patrickriordan/imgui_lua_bindings
* Texture atlas module for resource managing
* Runtime resource loading.

## TODO
* 9-patch draw

## Links
[CEFOverlayEngine](https://github.com/ZCube/CEFOverlayEngine) : Overlay engine with CEF. HTML supported.

##

NO WARRANTY. ANY USE OF THE SOFTWARE IS ENTIRELY AT YOUR OWN RISK.
