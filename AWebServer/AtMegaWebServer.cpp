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

#include "Arduino.h"

extern "C" {

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
}

#include <Ethernet.h>
#include <Flash.h>
#include "AtMegaWebServer.h"



// Offset for text/html in `mime_types' above.
static const AtMegaWebServer::MimeType text_html_content_type = 4;
// Temporary buffer.
static char buffer[BUFFER_SIZE];


FLASH_STRING(mime_types,
  "HTM*text/html|"
  "TXT*text/plain|"
  "CSS*text/css|"
  "XML*text/xml|"
  "JS*text/javascript|"

  "GIF*image/gif|"
  "JPG*image/jpeg|"
  "PNG*image/png|"
  "ICO*image/vnd.microsoft.icon|"

  "MP3*audio/mpeg|"
);
FLASH_STRING(content_type_msg, "Content-Type: ");

void *malloc_check(size_t size) {
  void* r = malloc(size);
#if DEBUG
  if (!r) {
    Serial << F("No space for malloc: " ); Serial.println(size, DEC);
  }
#endif
  return r;
}


AtMegaWebServer::AtMegaWebServer(PathHandler handlers[],
			     const char** headers)
  : handlers_(handlers),
    server_(EthernetServer(80)),
    path_(NULL),
    request_type_(UNKNOWN_REQUEST),
    client_(EthernetClient(255))
     {
  if (headers) initHeaders(headers);
  }

void AtMegaWebServer::initHeaders(const char** headers)
{
    int size = 0;
    for (int i = 0; headers[i]; i++) {
      size++;
    }
    headers_ = (HeaderValue*)malloc_check(sizeof(HeaderValue) * (size + 1));
    if (headers_) {
      for (int i = 0; i < size; i++) {
        headers_[i].header = headers[i];
        headers_[i].value = NULL;
      }
      headers_[size].header = NULL;
    }
}

void AtMegaWebServer::begin() {
  server_.begin();
}

boolean AtMegaWebServer::readLine(int timeOut){
  int idx = 0;
  char c = 0;
  while(idx < sizeof(buffer) && waitClientAvailable(timeOut)){
    c = client_.read();
#if DEBUG
    Serial.print(c);
#endif
    if(c == '\r') continue;
    if(c == '\n') break;
    buffer[idx++] = c;
  }
  buffer[idx] = 0;
  return c == '\n';
}

boolean AtMegaWebServer::processRequest() {
  client_ = server_.available();
  if (!client_.connected() || !client_.available()) {
    return false;
  }
#if DEBUG
  Serial << F("WebServer: New request:\n");
#endif

  if(!readLine() || !*buffer){
    sendHttpResult(408); // 408 Request Time-out
    client_.stop();
    return false;
  }

  char* start = buffer;
  while(isspace(*start)) start++;
  if (!strncmp("GET", start, 3)) {
    request_type_ = GET;
  } else if (!strncmp("POST", start, 4)) {
    request_type_ = POST;
  } else if (!strncmp("PUT", start, 3)) {
    request_type_ = PUT;
  } else if (!strncmp("DELETE", start, 6)) {
    request_type_ = DELETE;
  } else if (!strncmp("MOVE", start, 4)) {
    request_type_ = MOVE;
  }else request_type_ = UNKNOWN_REQUEST;

  while(*start && !isspace(*start)) start++; // end of request_type_
  while(*start && isspace(*start)) start++; // skip spaces, begin of path
  unescapeChars(start); // decode %-sequences of path
  char *end = start;
  while(*end && !isspace(*end)) end++; // end of path

  path_ = (char*) malloc_check(end - start + 1);
  if (path_) {
    memcpy(path_, start, end - start);
    path_[end - start] = 0;
  }

  boolean found = false, success = false;
  while(readLine()){
	  if(!*buffer){
		  success = true; // there are 2 x CRLF at end of header
		  break;
	  }
	  found |= assignHeaderValue();
  }
  // if found == true: at least 1 header value could be assigned
  // success indicates that there 2 x CRLF at end, so every thing is fine
  // if not you can sendHttpResult(404); or try to find a handler, maybe the 1st line
  // has been correct. If this fails sendHttpResult(404);
  // is called anyhow 
  // Header processing finished. Identify the handler to call.

  boolean should_close = true;
  found = false;
  for (int i = 0; handlers_[i].path; i++) {
    int len = strlen(handlers_[i].path);
    boolean match = !strcmp(path_, handlers_[i].path);
    if (!match && handlers_[i].path[len - 1] == '*') {
      match = !strncmp(path_, handlers_[i].path, len - 1);
    }
    if (match && (handlers_[i].type == ANY || handlers_[i].type == request_type_)) {
      found = true;
      should_close = (handlers_[i].handler)(*this);
      break;
    }
  }

  if (!found) {
    sendHttpResult(404);
  }
  if (should_close) {
    client_.stop();
  }

  freeHeaders();
  free(path_);
  return true;
}

void AtMegaWebServer::sendHttpResult(int code, MimeType mime, const char *extraHeaders){
#if DEBUG
  Serial << F("WebServer: Returning ") << code << '\n';
#endif
  client_ << F("HTTP/1.1 ");
  client_ << code;
  client_ << F(" OK\r\n");
  if (mime) {
    client_ << content_type_msg;
    char ch;
    int i = mime;
    while ((ch = mime_types[i++]) != '|') {
      client_.print(ch);
    }
    client_.println();
  }
  if(extraHeaders){
    client_ << extraHeaders;
  }
  client_ << CRLF;
}

boolean AtMegaWebServer::assignHeaderValue(){
  if (!headers_) {
    return false;
  }
  char *head = buffer;
  while(*head && isspace(*head)) head++;
  char *val = head;
  while(*val && *val != ':') val++;
  *val++ = 0;
  for (int i = 0; headers_[i].header; i++) {
    if (!strcmp(head, headers_[i].header)) {

      headers_[i].value = (char*)malloc_check(strlen(val) + 1);
      if (headers_[i].value) {
        strcpy(headers_[i].value, val);
        return true;
      }
    }
  }
  return false;
}

void AtMegaWebServer::freeHeaders(){
  if (headers_) {
    for (int i = 0; headers_[i].header; i++) {
      if (headers_[i].value) {
        free(headers_[i].value);
        // Ensure the pointer is cleared once the memory is freed.
        headers_[i].value = NULL;
      }
    }
  }
}

int AtMegaWebServer::parseHexChar(char c){
	int ret = -1;
	if(c >= '0' && c <= '9') ret = c - '0';
	else if(c >= 'A' && c <= 'F') ret = c - 'A' + 10;
	else if(c >= 'a' && c <= 'f') ret = c - 'a' + 10;
	return ret;
}

int AtMegaWebServer::unescapeChars(char* str){
	int ret = 0;
	while(*str && *str != '%') str++;
	if(*str){
		char* dest = str;
		do{
			if(parseHexChar(*(str + 1)) >= 0 && parseHexChar(*(str + 2)) >= 0){
				*dest = (parseHexChar(*(str + 1)) << 4) + parseHexChar(*(str + 2));
				str += 3;
				dest++;
				ret += 2;
			}
			while(*str && *str != '%'){
				*dest = *str;
				dest++; str++;
			}
		}while(*str);
		*dest = 0;
	}
	return ret;
}

boolean AtMegaWebServer::waitClientAvailable(int sec){
  if(client_.available()) return true;
  int max = sec * 100;
  for(int i = 0; i < max && client_.connected(); i++){
    delay(10);
    if(client_.available()){
#if DEBUG
      Serial << "waitClientAvailable: " << i*10 << " msec\n";
#endif
      return true;
    }
  }
#if DEBUG
  Serial << "waitClientAvailable: false  after " << sec << "secs\n";
#endif
  return false;
}

const char* AtMegaWebServer::get_path() { return path_; }

const AtMegaWebServer::HttpRequestType AtMegaWebServer::get_type() {
  return request_type_;
}

const char* AtMegaWebServer::get_header_value(const char* name) {
  if (!headers_) {
    return NULL;
  }
  for (int i = 0; headers_[i].header; i++) {
    if (!strcmp(headers_[i].header, name)) {
      return headers_[i].value;
    }
  }
  return NULL;
}


AtMegaWebServer::MimeType AtMegaWebServer::get_mime_type_from_filename(
    const char* filename) {
  MimeType r = text_html_content_type;
  if (!filename) {
    return r;
  }

  char* ext = strrchr(filename, '.');
  if (ext) {
    // We found an extension. Skip past the '.'
    ext++;

    char ch;
    int i = 0;
    while (i < mime_types.length()) {
      // Compare the extension.
      char* p = ext;
      ch = mime_types[i];
      while (*p && ch != '*' && toupper(*p) == ch) {
		  p++; i++;
		  ch = mime_types[i];
      }
      if (!*p && ch == '*') {
	// We reached the end of the extension while checking
	// equality with a MIME type: we have a match. Increment i
	// to reach past the '*' char, and assign it to `mime_type'.
			r = ++i;
			break;
      } else {
	// Skip past the the '|' character indicating the end of a
	// MIME type.
			while (mime_types[i++] != '|')
	  ;
      }
    }
  }
  return r;
}

void AtMegaWebServer::send_file(SdFile& file) {
  size_t size;
  while ((size = file.read(buffer, sizeof(buffer))) > 0) {
    if (!client_.connected()) {
      break;
    }
    client_.write((uint8_t*)buffer, size);
  }
}

size_t AtMegaWebServer::write(uint8_t c) {
  return client_.write(c);
}

size_t AtMegaWebServer::write(const char *str) {
  return client_.write(str);
}

size_t AtMegaWebServer::write(const uint8_t *buffer, size_t size) {
  return client_.write(buffer, size);
}


namespace WebServerHandler {
  SdFat sdfat;

  boolean init(uint8_t rate, uint8_t pin){
	return sdfat.init(rate, pin);// .begin(4, SPI_FULL_SPEED);
  }


  boolean put_handler(AtMegaWebServer& web_server) {
	const char* length_str = web_server.get_header_value("Content-Length");
	long length = atol(length_str);
	const char *path = web_server.get_path();

	SdFile file;
	if(!file.open(path, O_CREAT | O_WRITE | O_TRUNC)){
		 // maybe the folder must be created
		char *c = strrchr(path, '/');
		if(c){
			*c = 0;
			if(sdfat.mkdir(path)){
#if DEBUG
				Serial << "put_handler make DIR: ok " << path <<'\n';
#endif
				*c = '/';
				if(!file.open(path, O_CREAT | O_WRITE | O_TRUNC)){
#if DEBUG
					Serial << "put_handler open FILE: failed " << path <<'\n';
#endif
				}
			}
			*c = '/';
		}
	}
	if(file.isOpen()){

		EthernetClient client = web_server.get_client();
		long size = 0;
		int read = 0;
		while(size < length && web_server.waitClientAvailable()){
			read = client.read((uint8_t*)buffer, sizeof(buffer));
			file.write(buffer, read);
			size += read;
		}
		file.close();
#if DEBUG
		Serial << "file written: " << size << " of: " << length << '\n';
#endif
		if(size < length){
			web_server.sendHttpResult(404);
		}else{
			web_server.sendHttpResult(200);
		}

	}else{
		web_server.sendHttpResult(422); // assuming it's a bad filename (non 8.3 name)
#if DEBUG
		Serial << F("put_handler open file failed: send 422 ") << path <<'\n';
#endif
	}
	return true;
  }

// for renaming files and dirs
boolean move_handler(AtMegaWebServer& web_server){
	const char *path = web_server.get_path();

#if DEBUG
	Serial << F("move_handler filename: ") << path << '\n';
#endif
    const char* length_str = web_server.get_header_value("Content-Length");
    int len = atoi(length_str);

    Client& client = web_server.get_client();

    int i = 0;
    int baselen = 0;
    char* c;
    if((c = strrchr(path, '/'))) baselen = c - path + 1;
    char buf[baselen + len + 1];
    if(baselen) strncpy(buf, path, baselen);
    i = baselen;

    for(; i < (baselen + len) && web_server.waitClientAvailable(); i++) {
      buf[i] = client.read();// (char)
    }
    buf[i] = 0;
#if DEBUG
    Serial << buf << "|end: " << i << '\n';
#endif

    if(i == (len + baselen)){
      if(sdfat.rename(path, buf)){
#if DEBUG
      Serial << "renaming: " << path << " to: " << buf << '\n';
#endif
        web_server.sendHttpResult(200);
      	web_server << buf;
      }else{
#if DEBUG
        Serial << "renaming: failed\n" << buf << LF;
#endif
        web_server.sendHttpResult(422);
      }
    }else{
      web_server.sendHttpResult(404);
    }
    return true;
  }

  boolean delete_handler(AtMegaWebServer& web_server){
	const char *path = web_server.get_path();
#if DEBUG
	Serial << "delete_handler: " << path << '\n';
#endif
	int len = strlen(path);
	char *c = (char *)(path + len - 1);
	if(*c == '/') *c = 0;// remove tailing '/'

	if(sdfat.remove(path) || sdfat.rmdir(path)){
#if DEBUG
		Serial << "delete: " << path << '\n';
#endif
		web_server.sendHttpResult(200);
		web_server << path;
	}else{
		web_server.sendHttpResult(404);
//		web_server << "not exists or failed deleting: " << path;
#if DEBUG
		Serial << F("not exists or failed deleting: ") << path << '\n';
#endif
	}
	return true;
  }

  boolean get_handler(AtMegaWebServer& web_server){
	const char* filename = web_server.get_path();
#if DEBUG
	Serial << F("file_handler path: ") << filename << '\n';
#endif

  if (!filename) {
    web_server.sendHttpResult(404);
#if DEBUG
    Serial << F("Could not parse URL");
#endif
    return true;
  }

  if(filename[0] == '/' && filename[1] == 0){ // root dir, file.open() will fail
    listDirectory(web_server, SdBaseFile::cwd());
	return true;
  }

  SdFile file;
  if(file.open(filename, O_READ)){
#if DEBUG
     Serial << "file isOpen: " << filename << '\n';
#endif
	  if (file.isDir())
	  {
		listDirectory(web_server, &file);
	  }
	  else
	  {
		AtMegaWebServer::MimeType mime_type
		  = AtMegaWebServer::get_mime_type_from_filename(filename);

  // If you want to send a file with a non-supported MimeType you can:
  // web_server.sendHttpResult(200, 0, "Content-Type: image/tiff" CRLF);
  
        web_server.sendHttpResult(200, mime_type);
#if DEBUG
		Serial << F("Read file ");
		Serial.println(filename);
#endif
		web_server.send_file(file);
	  }
	  file.close();
    }else{
      web_server.sendHttpResult(404);
    }
    return true;
}

void listDirectory(AtMegaWebServer& web_server, SdBaseFile* file)
{
  const char* path =  web_server.get_path();
  web_server.sendHttpResult(200);

  Client& client = web_server.get_client();
  web_server << F("<html><head><title>");
  web_server << (path);
  web_server << F("</title></head><body><h1>");
  web_server << (path);
  web_server << F("</h1><hr><pre>");
  listFiles(path, file, &client, LS_DATE | LS_SIZE);
  web_server << F("</pre><hr></body></html>\n");
}

void listFiles(const char* path, SdBaseFile* file, Client* client, uint8_t flags) {
  // This code is just copied from SdFile.cpp in the SDFat library
  // and tweaked to print to the client output in html!
  dir_t p;

  if(strcmp(path, "/") != 0)
  {
    int idx = strrchr( path, '/' ) - path;
    char parent[idx + 1];
    strncpy(parent, path, idx);
    parent[idx] = 0;
    client->print(F("<a href=\""));
    client->print(F("http://"));
    client->print(Ethernet.localIP());
    client->print(parent);
    client->println(F("\">[To Parent Directory]</a>\n"));
  }

  file->rewind();
  while (file->readDir(&p) > 0)
  {
    // done if past last used entry
    if (p.name[0] == DIR_NAME_FREE) break;

    // skip deleted entry and entries for . and  ..
    if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;

    // only list subdirectories and files
    if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;

    // print modify date/time if requested
    if (flags & LS_DATE) {
      printFatDate(client, p.lastWriteDate);
      client->print(' ');
      printFatTime(client, p.lastWriteTime);
    }

    // print size if requested
    if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE)) {
      client->print(' ');
      client->print(p.fileSize);
    }

    if (DIR_IS_SUBDIR(&p)) {
      client->print(F(" &lt;dir&gt;"));
    }

    // print file name with possible blank fill
    char name[13];
    file->dirName(p, name);
    client->print(F(" <a href=\""));
    client->print(F("http://"));
    client->print(Ethernet.localIP());

    client->print(path);
    if(strcmp(path, "/") != 0)
      client->print('/');

    client->print(name);
    client->print("\">");
    client->print(name);
    client->println("</a>");
  }
}
void printFatDate(Client* client, uint16_t fatDate)
{
  client->print(FAT_YEAR(fatDate));
  client->print('-');
  printTwoDigits(client, FAT_MONTH(fatDate));
  client->print('-');
  printTwoDigits(client, FAT_DAY(fatDate));
}

void printFatTime(Client* client, uint16_t fatTime)
{
  printTwoDigits(client, FAT_HOUR(fatTime));
  client->print(':');
  printTwoDigits(client, FAT_MINUTE(fatTime));
  client->print(':');
  printTwoDigits(client, FAT_SECOND(fatTime));
}

void printTwoDigits(Client* client, uint8_t v)
{
  char str[3];
  str[0] = '0' + v/10;
  str[1] = '0' + v % 10;
  str[2] = 0;
  client->print(str);
}
}
