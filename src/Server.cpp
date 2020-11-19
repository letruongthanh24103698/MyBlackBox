#include "Server.h"

#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

AsyncWebServer sv(80);

char text[1000];

char HTML[] = "<!DOCTYPE html>\
<html>\
<body>\
<h1>My First Web Server with ESP32 - Station Mode &#128522;</h1>\
</body>\
</html>";

void server_setup()
{
// Send web page with input fields to client
    sv.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", HTML);
    });
}