AWebServer
==========
is a Http-Server for Arduino with full file managing support on SD card, file system time support (NTP), UDP broadcast discovery and optionaly Json support.

The code is based on the TinyWebServer Library, Copyright (C) 2010 Ovidiu Predescu https://github.com/ovidiucp/TinyWebServer. 

It supports all file managing http-commands (GET, PUT, DELETE) and additional rename (non http: MOVE).
Uploaded files and folders will get the actual local time. If the DEBUG flag is set, all actions will
be detailed commented. The clock starts automatical and will be set every 2 hours (can be freely selected
by setting the TIME_REQU_INTV in UdpServices.cpp, even the difference to GMT with TimeOffset).
How it works and looks like you can see at the attached png's (using DuinoExplorer).

As full version with Json support and DEBUG flag it takes ~ 45.000 bytes and a Arduino Mega is needed.
Without this and commenting out UdpServices::serveDiscovery(); in loop() it takes ~32.800 bytes. 
( and allmost fits on a Arduino UNO, soon I'll publish a version that fits)


_____________________
External dependencies:
=====================

AWebServer depends on the external library SdFat ( (C) 2012 by William Greiman ) (http://code.google.com/p/sdfatlib/)
and, if Json is needed, on aJson lib ( (c) 2001, Marcus Nowotny ) (https://github.com/interactive-matter/aJson).

There is also a real time support and Udp-discovery included. All features can be tested with 
DuinoExplorer from Windows (available on codeplex http://duinoexplorer.codeplex.com/) or DuinoFinder from iOS (https://duinofinder.codeplex.com/).
