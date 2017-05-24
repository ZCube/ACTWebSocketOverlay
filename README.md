# ActWebSocketOverlay
ActWebSocket overlay utility based on ImGui

[![Build Status](https://jenkins.zcube.kr/buildStatus/icon?job=ACTWebSocketOverlay)](https://jenkins.zcube.kr/job/ACTWebSocketOverlay/)

*ActWebSocket needed.

## Interface
Button or Key | Descrition
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
Language | File
-------- | ----
Default | Default (Embedded)
Japanese | ArialUni.ttf
Korean | gulim.ttc

## File Overview
Path | Description | License 
---- | ----------- | -------
[/src/awio/overlay.cpp](/src/awio/overlay.cpp) | Overlay Main | GPL License
[/src/awio/main.cpp](/src/awio/main.cpp) | Modified Dear ImGui example for test overlay | MIT License

## Used Libraries
Library | Description
------- | -----------
[Boost](https://boost.org) | Boost Library (ASIO, filesystem)
[Beast](https://github.com/vinniefalco/Beast) | WebSocket Library
[ImGui](https://github.com/ocornut/imgui) | Dear ImGui 
[JsonCPP](https://github.com/open-source-parsers/jsoncpp) | Json Library for parsing Message and Settings

## Manual
[Manual Google Presentation](https://docs.google.com/presentation/d/19uWnxraScX6bXAaX3My4YcTMnHZPDmXxNpg8QXjCeDY/pub?start=false&loop=false&delayms=3000)

## Release
[Latest](https://www.dropbox.com/s/rcypgitu9icz7kp/ACTWebSocketOverlay_latest.zip?dl=1)
[Release](https://github.com/ZCube/ActWebSocketOverlay/releases)

## Build Tool
Microsoft Visual Studio Community 2015

## Build Instruction
* boost build step
    1. download https://dl.bintray.com/boostorg/release/1.64.0/source/:boost_1_64_0.7z
    2. unzip
    3. bootstrap.bat
    4. b2 --stagedir=stage   variant=debug,release address-model=32 threading=multi link=static runtime-link=shared --prefix=c:/boost32 install
    5. b2 --stagedir=stage64 variant=debug,release address-model=64 threading=multi link=static runtime-link=shared --prefix=c:/boost64 install

* Texture build step
    1. cd textures
    2. gen_atlas.bat
    
* ACTWebSocketOverlay build step
    1. configure.bat
    2. build.bat

##

NO WARRANTY. ANY USE OF THE SOFTWARE IS ENTIRELY AT YOUR OWN RISK.
