AWebServer
==========
is a Http-Server for Arduino with full file managing support on SD card, file system time support (NTP), UDP broadcast discovery and optionaly Json support. 
This repository is designed as a single sketch for testing and developing purposes. If you prefer it as a library take https://github.com/tilos/AtMegaWebServer

The code is based on the TinyWebServer Library, Copyright (C) 2010 Ovidiu Predescu https://github.com/ovidiucp/TinyWebServer. 


It supports all file managing http-commands (GET, PUT, DELETE) and additional rename (non http (WebDAV): MOVE).
Uploaded files and folders will get the actual local time. The clock starts automatical and will be set every 2 hours (can be freely selected
by setting the TIME_REQU_INTV in UdpServices.cpp, even the difference to GMT with TimeOffset).
How it works and looks like you can see here:

![screenshot](https://github.com/tilos/AWebServer/raw/master/AWS_in_Mozilla.PNG) 

or here (using DuinoExplorer from http://duinoexplorer.codeplex.com/ as client).

![screenshot](https://github.com/tilos/AWebServer/raw/master/explore_AWS.PNG) 


If the DEBUG flag is set, all actions will be detailed commented, including all headers received by requests and a memory
test will be done after each request.

![screenshot](https://github.com/tilos/AWebServer/raw/master/requests_AWS.PNG)


UDP broadcast discovery makes it easy to find your device in your local network, especially if it takes it's ip address
from the router (and is not hard coded in your software).

![screenshot](https://github.com/tilos/AWebServer/raw/master/discover_AWS.PNG)


With the optional JSON flag you can include a simple json handler example, which adds all posted int values.
It can be tested with JSEditor from DuinoExplorer.

![screenshot](https://github.com/tilos/AWebServer/raw/master/json_AWS.PNG)


As full version with Json support and DEBUG flag it takes ~ 45.000 bytes and a Arduino Mega is needed.

A reduced version without DEBUG, discovery, Json and move_handler (for renaming files and directories on SD card)
needs 31.974 bytes and will fit on Arduino UNO. For that, simply set the UNO flag in global.h to 1.


_____________________
External dependencies:
=====================

AWebServer depends on the external library SdFat ( (C) 2012 by William Greiman ) (http://code.google.com/p/sdfatlib/), 
Flash version 4.0 (http://arduiniana.org/libraries/flash/) and, if Json is needed, on aJson lib ( (c) 2001, Marcus Nowotny ) (https://github.com/interactive-matter/aJson).

All features can be tested with DuinoExplorer from Windows (available on codeplex http://duinoexplorer.codeplex.com/) 
or DuinoFinder from iOS (https://duinofinder.codeplex.com/).
