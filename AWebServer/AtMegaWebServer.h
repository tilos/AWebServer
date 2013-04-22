/*

AtMegaWebServer Library, Copyright (c) 2013 Tilo Szepan, Immo Wache <https://github.com/tilos/AWebServer.git>

Based on the TinyWebServer Library, Copyright (C) 2010 Ovidiu Predescu <ovidiu@gmail.com>

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

#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include <Print.h>
#include <SdFat.h>
#include "global.h"

const int BUFFER_SIZE = 255;
// max secs to wait for read available
const int TIME_OUT = 30;


class AtMegaWebServer;


namespace WebServerHandler {
  const int SDC_PIN = 4;

  boolean init(uint8_t rate = SPI_FULL_SPEED, uint8_t pin = SDC_PIN);
  boolean put_handler(AtMegaWebServer& web_server);
  boolean move_handler(AtMegaWebServer& web_server);
  boolean delete_handler(AtMegaWebServer& web_server);
  boolean get_handler(AtMegaWebServer& web_server);
  void listDirectory(AtMegaWebServer& web_server, SdBaseFile* file);
  void listFiles(const char* path, SdBaseFile* file, Client* client, uint8_t flags);
  void printFatDate(Client* client, uint16_t fatDate);
  void printFatTime(Client* client, uint16_t fatTime);
  void printTwoDigits(Client* client, uint8_t v);
};


class AtMegaWebServer : public Print {
public:
  // An HTTP path handler. The handler function takes the path it
  // registered for as argument, and the Client object to handle the
  // response.
  //
  // The function should return true if it finished handling the request
  // and the connection should be closed.
  typedef boolean (*WebHandlerFn)(AtMegaWebServer& web_server);

  enum HttpRequestType {
    UNKNOWN_REQUEST,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    MOVE,
    ANY,
  };

  // An identifier for a MIME type. The number is opaque to a human,
  // but it's really an offset in the `mime_types' array.
  typedef uint16_t MimeType;

  typedef struct {
    const char* path;
    HttpRequestType type;
    WebHandlerFn handler;
  } PathHandler;

  // Initialize the web server using a NULL terminated array of path
  // handlers, and a NULL terminated array of headers the handlers are
  // interested in.
  //
  // NOTE: Make sure the header names are all lowercase.
  AtMegaWebServer(PathHandler handlers[], const char** headers);

  // Call this method to start the HTTP server
  void begin();

// stores incomming data into buffer until CRLF or only LF occurs
// CRLF will NOT be stored, so an empty line leads to an empty buffer
// and returns true
// It waits up to timeOut secs for data, then returns false
  boolean readLine(int timeOut = TIME_OUT);
  
  // Handles a possible HTTP request. It will return immediately if no
  // client has connected. Otherwise the request is handled
  // synchronously.
  //
  // Call this method from the main loop() function to have the Web
  // server handle incoming requests.
  boolean processRequest();


  // translates a single hex char ('0' - '9', 'a' | 'A' - 'f' | 'F') to int (0 ... 15) 
  // if c is not a hex char -1 will be returned
  int parseHexChar(char c);
  
  // looks for '%' in str and trys to parse the following 2 chars (hex values)
  // and stores the result in str
  // it returns the number of chars str length has decreased (2 x number of '%'s)
  // so you can: if(unescapeChars(char* str)){ char *newstr = (char*)malloc(strlen(str) + 1) ... free(str)
  // if you want to save each byte or do it in a buffer before allocating
  int unescapeChars(char* str);
  
  // waits up to paramvalue secs (default is TIME_OUT ( = 30)) for incomming data
  // and returns if some available
  boolean waitClientAvailable(int sec = TIME_OUT);
  
  // output standard headers indicating "200 Success" by calling without params. You can change the
  // type of the data you're outputting (MimeType get_mime_type_from_filename(const char* filename);)
  // If you want to send a file with a non-supported MimeType you can:
  // web_server.sendHttpResult(200, 0, "Content-Type: image/tiff" CRLF);
  // or also add extra headers like "Refresh: 1" CRLF.
  // Extra headers should each be terminated with CRLF.
  void sendHttpResult(int code = 200, MimeType mime = 0, const char *extraHeaders = 0);
  
  // assigns the values for the requested headers passed with the constructor
  // if there are some stored in buffer 
  boolean assignHeaderValue();
  
  // free's all header values from last request
  void freeHeaders();



  const char* get_path();
  const HttpRequestType get_type();
  const char* get_header_value(const char* header);
  EthernetClient& get_client() { return client_; }

  // Guesses a MIME type based on the extension of `filename'. If none
  // could be guessed, the equivalent of text/html is returned.
  static MimeType get_mime_type_from_filename(const char* filename);

  // Sends the contents of `file' to the currently connected
  // client. The file must be opened in read mode.
  //
  // This is mainly an optimization to reuse the internal static
  // buffer used by this class, which saves us some RAM.
  void send_file(SdFile& file);

  // These methods write directly in the response stream of the
  // connected client
  virtual size_t write(uint8_t c);
  virtual size_t write(const char *str);
  virtual size_t write(const uint8_t *buffer, size_t size);

 
 typedef struct {
    const char* header;
    char* value;
  } HeaderValue;

void initHeaders(const char** headers);

private:
  // The path handlers
  PathHandler* handlers_;

  // The headers
  HeaderValue* headers_;

  // The TCP/IP server we use.
  EthernetServer server_;

  char* path_;
  HttpRequestType request_type_;
  EthernetClient client_;

};

#endif /* __WEB_SERVER_H__ */
