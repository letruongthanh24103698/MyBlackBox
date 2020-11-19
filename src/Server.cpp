#include "Server.h"

#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

#include "FS.h"
#include "SD.h"

#define _MAX_NUMBER_FILE_PER_PAGE_      20

AsyncWebServer sv(80);

char text[1000];

char HTML[] = "<!DOCTYPE html>\
<html>\
<body>\
<h1>My Black Box - 11A2 - Quang Tri Town High School &#128522;</h1>\
<h2>Name 1</h2><br>\
<h2>Name 2</h2><br>\
<h2>name 3</h2><br>\
<form action=\"/getNameFile_web?page=0\">\
    <input type=\"submit\" value=\"Submit\">\
</form>\
</body>\
</html>";

char HTML_FILE[] = "<!DOCTYPE html>\
<html>\
<head>\
<title>My Black Box</title>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
</head>\
<body>\
<h1>My Black Box - 11A5 - Quang Tri Town High School &#128522;</h1>\
<h2>FILE NAME</h2><br>\
<h3>%s</h3>\
<form action=\"/getNameFile_web\">\
    <input type=\"hidden\" name=\"page\" value=\"%d\">\
    <input type=\"submit\" value=\"Previous\" style=\"float: left\">\
</form>\
<label for=\"Page\" style=\"float: left\">   %d/%d   </label>\
<form action=\"/getNameFile_web\">\
    <input type=\"hidden\" name=\"page\" value=\"%d\">\
    <input type=\"submit\" value=\"Next\">\
</form>\
</body>\
</html>";

File root;

uint32_t GetFileCount()
{
    uint32_t cnt=0;
    File entry;
    while (true) 
    {
        entry =  root.openNextFile();
        
        if (! entry) 
            break;
        
        cnt++;
        entry.close();
    }

    return cnt;
}

void NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void server_setup()
{
    root=SD.open("/");
    // Send web page with input fields to client
    sv.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("Home Page Request");
        request->send_P(200, "text/html", HTML);
    });

    sv.on ("/getNameFile", HTTP_GET, [](AsyncWebServerRequest *request){
        uint32_t file_count_U32=GetFileCount();
        uint32_t page_count_U32=ceil((double)file_count_U32/(double)_MAX_NUMBER_FILE_PER_PAGE_);

        uint32_t request_page_U32=0;

        String inputMessage;
        if (request->hasParam("page")) 
        {
            inputMessage = request->getParam("page")->value();
            request_page_U32=atoi(inputMessage.c_str());
            String filenames;
            String resp;
            sprintf((char *)resp.c_str(), HTML_FILE, filenames.c_str(), request_page_U32-1, request_page_U32, page_count_U32, request_page_U32+1);
            request->send_P(200, "text/html", resp.c_str());
        }
        else 
            request->send_P(200, "text/html", "Invalid Page Input!");
    });

    sv.onNotFound(NotFound);
}

void server_start()
{
    sv.begin();
    Serial.println("Server Started");
}

void server_stop()
{
    sv.end();
    Serial.println("Server Stopped");
}