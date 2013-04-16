/*
Copyright (c) 2013 Tilo Szepan, Immo Wache

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef UdpServices_h
#define UdpServices_h

#include <SPI.h>
#include <Ethernet.h>
#include <SdFat.h>


namespace UdpServices{

void begin(const char *_description, const char *_prefix);

// call this in loop and your device can be detected via Udp
void serveDiscovery();

void sendDiscoveryPacket(IPAddress& udpRemoteIp, unsigned int udpRemotePort);

// call this in loop, after TIME_REQU_INTV the local time will be syncronized with timeServer
void maintainTime();

// millisecs since last syncronisation
unsigned long currMillis();

// secs since 1970 incl. TimeOffset 
unsigned long localTime();

bool requestTime(int repeat);

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address);

// helper for formatted output of time, msg is optional and printed first
void writeTime(Print *pr, unsigned long secs, const char *msg);

// callback for SdFat
void dateTime(uint16_t* date, uint16_t* time);
}
#endif
