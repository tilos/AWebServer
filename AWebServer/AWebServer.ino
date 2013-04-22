/*

Copyright (c) 2013 Tilo Szepan, Immo Wache <https://github.com/tilos/AWebServer.git>

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

// The DEBUG flag will enable serial console logging in this library and is 
// defined in global.h
//
// There is an overall size increase of about 5100 bytes in code size
// when the debugging is enabled


// if Json should be supported set JSON to 1, otherwise 0
// There is an overall size increase of about 5600 bytes in code size for Json-Support 
#include "global.h"
#if UNO
#define JSON 0
#else
#define JSON 1
#endif

#include <SPI.h>
#include <Ethernet.h>
#include <Flash.h>
#include <SdFat.h>
#include "AtMegaWebServer.h"
#include "UdpServices.h"


// aJson is a Json-Parser from Marcus Nowotny and as lib for Arduino available
#if JSON
#include <aJSON.h>
#endif


AtMegaWebServer::PathHandler handlers[] = {
  {"/" "*", AtMegaWebServer::PUT, &WebServerHandler::put_handler},
  {"/" "*", AtMegaWebServer::GET, &WebServerHandler::get_handler},
  {"/" "*", AtMegaWebServer::DELETE, &WebServerHandler::delete_handler},
#if !UNO
  {"/" "*", AtMegaWebServer::MOVE, &WebServerHandler::move_handler},
#endif
#if JSON
// an example how to add a free number of values and giving the sum as result
  {"/" "*", AtMegaWebServer::POST, &json_handler},
#endif
  {NULL}
};

const char* headers[] = {
  "Content-Length",
  NULL
};

AtMegaWebServer web = AtMegaWebServer(handlers, headers);

static uint8_t mac[] = {
  0x90, 0xA2, 0xDA, 0x0D, 0x97, 0x1A };

const int LEDPIN = 7;


void setup() {
#if DEBUG
  Serial.begin(115200);
  freeMem("freeMem begin");
#endif     

   if(!WebServerHandler::init()){
#if DEBUG
    // this unfortunately happens sometimes, and then no handler will work (no file system)
    // the only thing that helps is restart Arduino by switching power off and on again
     Serial << F("sdfat.begin() failed\n");
#endif     
   }

//  pinMode(LEDPIN, OUTPUT);


   // Initialize the Ethernet.
//  Serial << F("Setting up the Ethernet card...\n");
  Ethernet.begin(mac);
#if DEBUG
  Serial << F("Arduino Mega is at ");
  Serial.println(Ethernet.localIP());
#endif     

  // Start the web server.
//  Serial << F("Web server starting...\n");
  web.begin();
#if DEBUG
  Serial << F("Ready to accept HTTP requests.\n\n");
#endif     

  // pass a description and a prefix (path) for Udp discovery, might be 0
  UdpServices::begin("Arduino Mega", "web/js");
  UdpServices::maintainTime();
  SdBaseFile::dateTimeCallback(&UdpServices::dateTime);

#if DEBUG && JSON
  int res;
  if(parseJson("{ \"action\": \"add\", \"values\":[3, 4, 5 ] }", &res))
    Serial << LF << "Json sum: " << res << LF << LF;
#endif

#if DEBUG
  freeMem("freeMem Setup end");
#endif
}

void loop() {
  if(web.processRequest())
#if DEBUG
    freeMem("freeMem after request")
#endif
  ;
#if !UNO
  UdpServices::serveDiscovery();
#endif
  UdpServices::maintainTime();
}



#if JSON
// send with "POST" - command as example: { "action": "add", "values":[3, 4, 5 ...] }
boolean json_handler(AtMegaWebServer& web_server){
  const char* length_str = web_server.get_header_value("Content-Length");
  long length = atol(length_str);
  
  char buffer[length + 1];
  if(!buffer){
#if DEBUG
    Serial << "Content to large: " << length << '\n';
#endif
    return true;
  }
  EthernetClient client = web_server.get_client();
  long size = 0;
  while(size < length && web_server.waitClientAvailable()){
    size += client.read((uint8_t*)(buffer + size), sizeof(buffer) - size);
  }
  buffer[size] = 0;
  int val;
  if(parseJson(buffer, &val)){
#if DEBUG
      Serial << "json parsed: " << val << LF << " request: " << LF << buffer << LF;;
#endif
    web_server.sendHttpResult(200);
    web_server << "{\"result\": " << val << "}";
  }else{
    web_server.sendHttpResult(404);
  }

  return true;
}

// this example just adds all int values in a given json-string and returns the parsing success
// the sum is stored in res 
boolean parseJson(char *jsonString, int *res)
{
    boolean success = false;
    aJsonObject* root = aJson.parse(jsonString);

    if (root != NULL) {
        aJsonObject* action = aJson.getObjectItem(root, "action");

        if (action != NULL) {
            if (!strcmp("add", action->valuestring)) {
              aJsonObject* vals = aJson.getObjectItem(root, "values");
              if(vals){
                *res = 0;
                uint8_t cnt = aJson.getArraySize(vals);
                for(int i = 0; i < cnt; i++)
                  *res += aJson.getArrayItem(vals, i)->valueint;
                  
                success = true;
              }
            }
        }
        aJson.deleteItem(root);
    }
    return success;
}

#endif


#if DEBUG
//Code to print out the free memory

struct __freelist {
  size_t sz;
  struct __freelist *nx;
};

extern char * const __brkval;
extern struct __freelist *__flp;

uint16_t freeMem(uint16_t *biggest)
{
  char *brkval;
  char *cp;
  unsigned freeSpace;
  struct __freelist *fp1, *fp2;

  brkval = __brkval;
  if (brkval == 0) {
    brkval = __malloc_heap_start;
  }
  cp = __malloc_heap_end;
  if (cp == 0) {
    cp = ((char *)AVR_STACK_POINTER_REG) - __malloc_margin;
  }
  if (cp <= brkval) return 0;

  freeSpace = cp - brkval;

  for (*biggest = 0, fp1 = __flp, fp2 = 0;
     fp1;
     fp2 = fp1, fp1 = fp1->nx) {
      if (fp1->sz > *biggest) *biggest = fp1->sz;
    freeSpace += fp1->sz;
  }

  return freeSpace;
}

uint16_t biggest;

void freeMem(char* message) {
  Serial.print(message);
  Serial.print(":\t");
  Serial.println(freeMem(&biggest));
  Serial << F("biggest free()'d block: ") << biggest << LF;
}
#endif


