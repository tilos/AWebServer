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

#include <Flash.h>
#include <SdFat.h>
#include "UdpServices.h"
#include "global.h"

namespace UdpServices{

const unsigned int localPort = 8221;      // local port to listen on
// Interval for time requests in secs; 
// tests over a few days have shown that the accuracy of millis() on my device is ca. + 6 sec / h
// so every 2 hours seems to be ok
EthernetUDP Udp;


IPAddress timeServer(65, 55, 21, 23); // time.windows.com NTP server
const unsigned long TIME_REQU_INTV = 3600 * 2; 
const int TimeOffset = 3600 * 2; // sec/h * h Diff to GMT

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
unsigned long startMillis, secsSince1970 = 0;

byte packetBuffer[NTP_PACKET_SIZE];
char receiveBuffer[UDP_TX_PACKET_MAX_SIZE];

String prefix;// = "web/js";
String description;// = "Arduino Mega";

void begin(const char *_description, const char *_prefix){
  description = _description;
  prefix = _prefix;
  Udp.begin(localPort);
}

void serveDiscovery()
{
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(Udp.available())
  {
    IPAddress udpRemoteIp = Udp.remoteIP();
    unsigned int udpRemotePort = Udp.remotePort();

#if DEBUG
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    Serial.print(Udp.remoteIP());
    Serial.print(", port ");
    Serial.println(udpRemotePort);
#endif
    // read the packet into receiveBuffer
    Udp.read(receiveBuffer, UDP_TX_PACKET_MAX_SIZE);
    sendDiscoveryPacket(udpRemoteIp, udpRemotePort);
  }
}

void sendDiscoveryPacket(IPAddress& udpRemoteIp, unsigned int udpRemotePort)
{
  int bytesWrite = 0;
  long stringLen;

  String ipAddress = "";
  IPAddress localIp = Ethernet.localIP();
  for (int i =0; i < 4; i++)
  {
    ipAddress += String(localIp[i], DEC);
    if (i < 3)
      ipAddress += ".";
  }

  long port = 80;

  if(prefix){
    stringLen = prefix.length();
    memcpy(&packetBuffer[bytesWrite], &stringLen, sizeof(stringLen));
    bytesWrite += sizeof(stringLen);
    prefix.getBytes(&packetBuffer[bytesWrite], stringLen + 1);
    bytesWrite += stringLen;
  }
  
  if(description){
    stringLen = description.length();
    memcpy(&packetBuffer[bytesWrite], &stringLen, sizeof(stringLen));
    bytesWrite += sizeof(stringLen);
    description.getBytes(&packetBuffer[bytesWrite], stringLen + 1);
    bytesWrite += stringLen;
  }

  stringLen = ipAddress.length();
  memcpy(&packetBuffer[bytesWrite], &stringLen, sizeof(stringLen));
  bytesWrite += sizeof(stringLen);
  ipAddress.getBytes(&packetBuffer[bytesWrite], stringLen + 1);
  bytesWrite += stringLen;

  memcpy(&packetBuffer[bytesWrite], &port, sizeof(port));
  bytesWrite += sizeof(port);

  Udp.beginPacket(udpRemoteIp, udpRemotePort);
  Udp.write(packetBuffer, bytesWrite);
  Udp.endPacket();
}

// call this in loop, after TIME_REQU_INTV the local time will be syncronized with timeServer
// and Ethernet.maintain() will be called
void maintainTime(){
  if(currMillis() > TIME_REQU_INTV * 1000 || secsSince1970 == 0){

	  int ether = Ethernet.maintain(); // the renewal of DHCP leases
	  int repeat = 3;
	  while(ether == 1 || ether == 3 && --repeat > 0){ // an error occured
#if DEBUG
		  Serial << "Ethernet.maintain(): an error occured: " << ether << '\n';
#endif
		  ether = Ethernet.maintain();
	  }
#if DEBUG
	  Serial << "Ethernet.maintain(): " << ether << '\n';
	  writeTime(&Serial, localTime(),  "Start Request time at local time: ");
#endif

	  if(requestTime(2)){
#if DEBUG
	    writeTime(&Serial, localTime(), "time is set to: ");
#endif
	  }
  }
}

// returns msecs since last update
unsigned long currMillis(){
  unsigned long curr = millis(); // max: 4,294,967,295 : after apr. 50d it is 0 again
  if(startMillis <= curr){
    return curr - startMillis;
  }else{
    return 0xFFFFFFFFUL - startMillis + curr;
  }
}

// current local time
unsigned long localTime(){
  return secsSince1970 + TimeOffset + currMillis() / 1000;
}


// sometimes time requests fail or replies come to late, so try it repeat times
bool requestTime(int repeat){
#if DEBUG
  Serial << "Request time from server ..." << repeat << '\n';
#endif
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  int wait = 300; // = 3 sec
  bool response = false;
  while (wait > 0)
  {
    delay(10);
    if (Udp.parsePacket())
    {
      response = true;
      break;
    }
    wait--;
  }

  if (response)
  {
    startMillis = millis();

#if DEBUG
    Serial << "Millis: " << startMillis << '\n';
#endif

    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    secsSince1970 = (highWord << 16 | lowWord) - seventyYears;

#if DEBUG
    writeTime(&Serial, secsSince1970 + TimeOffset, "after request: ");
#endif
  }else{
    if(repeat > 0) return requestTime(repeat - 1);
#if DEBUG
    else Serial << F("Got no reply from time server\n");
#endif
  }
  return response;
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}

void writeTime(Print *pr, unsigned long secs, const char *msg){
    if(msg) *pr << msg << CRLF;
//    *pr << "Seconds since Jan 1 1970 = ";
//    *pr << secs << '\n';

    // print the hour, minute and second:
    *pr << (secs  % 86400L) / 3600; // print the hour (86400 equals secs per day)
    *pr << ':';
    int val = (secs % 3600) / 60;
    if ( val < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      *pr << '0';
    }
    *pr << val; // print the minute (3600 equals secs per minute)
    *pr << ':';
    val = secs % 60;
    if ( val < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      *pr << '0';
    }
    *pr << val << CRLF; // print the second
}

// dateTimeCallback for SdFat - lib
void dateTime(uint16_t* date, uint16_t* time) {
  unsigned long curr = localTime();
  uint16_t year;
  uint8_t month, day,
  hour = (curr % 86400L) / 3600,
  minute = (curr % 3600) / 60,
  second = curr % 60;

  unsigned long epochDays, B, C, D, E;

  /* 1. Determine the number of days since the epoch. */
  epochDays = curr / 86400ul;


  /* 6. Find year, month and day.                                           */
  /*    Algorithm is analog to calculation of Julian Day                    */
  /*    see: http://de.wikipedia.org/wiki/Julianisches_Datum                */
  /*    section: "Berechnung des Kalenderdatums aus dem Julianischen Datum" */

  /* 6.1 Pay attention for year 2100 is not a leap year */
  if (epochDays >= 47541ul)
    epochDays++;

  /*    epoch days offset is = 2 * 365 + 1 + 1524 = 2255 */
  B = epochDays + 2255ul;
  C = ((B * 100 - 12210ul) / 36525ul);
  D = (36525ul * C) / 100ul;
  E = ((B - D) * 10000ul / 306001ul);

  day = B - D - (E * 306001ul) / 10000ul;

  month = E - 1;
  if (E >= 14)
    month -= 12;

  year = C + 1965;
  if (month > 2)
    year -= 1;

      // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(year, month, day);

      // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(hour, minute, second);
}
}


