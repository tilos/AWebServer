AWebServer
==========
is a Http-Server for Arduino with full file managing support, real time clock, Udp discovery
and optionaly Json support.

As full version with Json support and DEBUG flag it takes ~ 45.000 bytes and a Arduino Mega is needed.
Without this and commenting out UdpServices::serveDiscovery(); in loop() it takes ~32.800 bytes. 
( and allmost fits on a Arduino UNO, soon I'll publish a version that fits)


External dependencies

AWebServer depends on the external library SdFat ( (C) 2012 by William Greiman ) (http://code.google.com/p/sdfatlib/)
and, if Json is needed, on aJson lib ( (c) 2001, Marcus Nowotny ) (https://github.com/interactive-matter/aJson).

There is also a real time support and Udp-discovery included. All features can be tested with 
DuinoExplorer, available on codeplex (http://duinoexplorer.codeplex.com/).
