# Installation Guide

## Download ACTWebSocket Plugin

Download [ACTWebSocket Plugin](https://github.com/ZCube/ACTWebSocket/releases) and unzip. 

If you use windows defender, you may need to unblock files. See this [link](https://blogs.msdn.microsoft.com/delay/p/unblockingdownloadedfile/)


## Install ACTWebSocket Plugin To Advanced Combat Tracker v3

![N|Solid](https://raw.githubusercontent.com/ZCube/ACTWebSocketOverlay/master/docs/Installation/0.png)

Add "ACTWebSocket.DLL" to Advanced Combat Tracker.

## Turn on ACTWebSocket

![N|Solid](https://raw.githubusercontent.com/ZCube/ACTWebSocketOverlay/master/docs/Installation/2.png)

* Recommended Settings

    * Auto Server Start. Check
    * Server Address ~~127.0.0.1~~ Use "Loopback IPV6([::1])" or FFXIV_ACT_Plugin throws of error messages

    * "Any IPV6([::])" or "Any (0.0.0.0)"
        * If you do not understand this, do not use this. This opens the port to the outside.
    * Server Port 10501
    * Use MiniParse Only
    * And turn on.

## Install Overlay to app

![N|Solid](https://raw.githubusercontent.com/ZCube/ACTWebSocketOverlay/master/docs/Installation/3.png)

* Recommended Settings

    * Select type (DX9 32bit/64bit, DX11 32bit/64bit)
    * Select executable file (*.exe)
    * Install
    * It shows installed version
    
# TroubleShooting

## FFXIV_ACT_Plugin throws a lot of error messages problem.

It throws a lot of error messages like the following.

MEMORY ERROR: 254|2017-05-26T00:14:01.8760000+09:00|Bad Packet header: 81012E81012E81012E81012E81012E81012E81012E81012E
MEMORY ERROR: 254|2017-05-26T00:14:01.8760000+09:00|Correcting network buffer, Discarding [30] of [30] bytes from Connection 127.0.0.1:10501 -> 127.0.0.1:4234: 81012E81012E81012E81012E81012E81012E81012E81012E81012E81012E

1. Use IP V6
    * FFXIV_ACT_Plugin use IPv4 only, so this works well.
    * "Loopback IPV6([::1])" recommended.

2. Local IP Address setting in FFXIV Settings. 

    * Choose Local IP other than 127.0.0.1 where the ACT will work.
    * ACTWebsocket will works on 127.0.0.1

3. Use [WinPCAP](https://www.winpcap.org/)

    * WinPCAP is a windows packet capture library that FFXIV_ACT_Plugin supported.
