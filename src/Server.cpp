#include "Server.h"

#include <WiFi.h>
#include <WebServer.h>

#include "FS.h"
#include "SD.h"

#define _MAX_NUMBER_FILE_PER_PAGE_      5
#define _MAX_BYTE_PER_TIME_             1000

WebServer sv(80);

char text[1000];

char HTML[] = "<!DOCTYPE html>\
<html>\
<head>\
<title>My Black Box</title>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
</head>\
<body>\
<h1>My Black Box - 11A2 - Quang Tri Town High School &#128522;</h1>\
<h2>Name 1</h2><br>\
<h2>Name 2</h2><br>\
<h2>name 3</h2><br>\
<form action=\"/getNameFile\">\
    <input type=\"hidden\" name=\"page\" value=\"0\">\
    <input type=\"submit\" value=\"List File\">\
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
<form action=\"/getNameFile\">\
    <input type=\"hidden\" name=\"page\" value=\"%d\">\
    <input type=\"submit\" value=\"Previous\" style=\"float: left\">\
</form>\
<label for=\"Page\" style=\"float: left\">   %d/%d   </label>\
<form action=\"/getNameFile\">\
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
    root.close();
    root=SD.open("/");
    while (true) 
    {
        entry =  root.openNextFile();
        
        if (! entry) 
            break;

        Serial.print(entry.name());
        Serial.print(" ");
        Serial.println(entry.size());
        
        cnt++;
        entry.close();
    }

    return cnt;
}

void GetFileName_Web(char *buf, uint32_t page)
{
    uint32_t cnt=0;
    File entry;
    root.close();
    root=SD.open("/");
    uint32_t len=0;
    while (true) 
    {
        entry =  root.openNextFile();
        if (! entry) 
            break;
        if ((int)(cnt/_MAX_NUMBER_FILE_PER_PAGE_) == page)
        {
            uint8_t size=strlen(entry.name());
            *(buf + len + 0)=(cnt%_MAX_NUMBER_FILE_PER_PAGE_)/10+0x30;
            *(buf + len + 1)=(cnt%_MAX_NUMBER_FILE_PER_PAGE_)%10+0x30;
            memcpy(buf+len+2,".   ",4);
            memcpy(buf+len+6,entry.name(),size);
            memcpy(buf+len+6+size,"<br>",4);
            buf=buf+10+size;
        }
        
        cnt++;
        entry.close();
    }
}

void NotFound() {
    // Serial.println("Access Missing WebPage!");
    sv.send(404, "text/plain", "Not found");
}

void GetFile()
{
    String inputMessage;
    
    if (sv.hasArg("page")) 
    {
        inputMessage = sv.arg("page");
        char filename[100];
        filename[0]='/';
        memcpy(filename+1,inputMessage.c_str(), inputMessage.length());
        filename[inputMessage.length()+1]=0;
        String str;
        str+=filename;
        Serial.println(str);
        
        if (SD.exists(str))
        {
            File tmp=SD.open(filename);
            // sv.sendHeader("Content-Type", "text/html");
            // sv.sendHeader("Content-Disposition", "attachment; filename="+String(filename));
            // sv.sendHeader("Connection", "close");
            // sv.setContentLength(tmp.size()+1);
            // sv.sendHeader(F("Content-Encoding"), F("gzip"));
            // sv.send(200,"text/html","a");;
            String resp;
            sv._prepareHeader(resp,200,"text/plain",tmp.size());
            // sv.sendHeader("Content-Disposition", "attachment; filename="+String(filename));
            sv.client().print(resp.c_str());
            // sv.sendHeader("Keep-Alive", "timeout=3600, max=100");
            // Serial.println("start");
            Serial.print("Ram Left:");
            Serial.println(esp_get_free_heap_size());
            char buffer[_MAX_BYTE_PER_TIME_+2];
            uint32_t cnt=0;
            uint32_t count=0;
            uint32_t available;
            do
            {
                available=tmp.available();
                if (available==0)
                    break;
                cnt = (available > _MAX_BYTE_PER_TIME_)?_MAX_BYTE_PER_TIME_:available;
                count+= tmp.readBytes(buffer, cnt);
                sv.client().write(buffer,cnt);
                /*buffer[cnt++]=(char)(tmp.read());
                if (cnt==_MAX_BYTE_PER_TIME_)
                {
                    buffer[cnt]=0;
                    // Serial.println(buffer);
                    sv.client().print(buffer);
                    // Serial.println(" ");
                    // sv.client().write(buffer);
                    // sv.client().flush();
                    cnt=0;
                }
                // delay(10);*/
            }while(true);
            // buffer[cnt]=0;
            // sv.client().write(buffer);
            // sv.client().print(buffer);
            // sv.client().flush();
            // Serial.println(buffer);
            sv.streamFile(tmp, "text/plain");
            // sv.client().flush();
            tmp.close();
            Serial.println(count);
            return;
        }
    }
    // sv.client().connect()
    sv.send(404, "text/html", "Invalid File Name Input!");
    sv.client().flush();
}

void DeleteAllFiles()
{
    root.close();
    root=SD.open("/");
    while (1)
    {
        File entry=root.openNextFile();
        if (!entry)
        break;
        SD.remove(entry.name());
    }
    Serial.println("Delete Done");
    sv.send(200,"text/html", "Delete Done");
    sv.client().flush();
}

void GetName()
{
    File entry;
    root.close();
    root=SD.open("/");
    uint32_t len=0;
    char *buf=(char *)malloc(20000*sizeof(char));
    while (true) 
    {
        entry =  root.openNextFile();

        if (! entry) 
            break;

        uint8_t size=strlen(entry.name());
        memcpy(buf+len,entry.name(),size);
        len+=size;
        buf[len++]=',';
        entry.close();
    }

    String resp;
    sv._prepareHeader(resp,200,"text/plain",len);
    sv.client().print(resp.c_str());

    uint32_t cnt=0;
    uint32_t available=len;
    uint32_t written=0;
    do 
    {
        if (available==0)
            break;
        cnt=(available > _MAX_BYTE_PER_TIME_)?_MAX_BYTE_PER_TIME_:available;
        written+=sv.client().write(buf+written,cnt);
        available-=cnt;
    }while (1);

    free(buf);
    sv.client().flush();
}

void GetSize()
{
    if (sv.hasArg("name"))
    {
        String inputstring="/"+sv.arg("name");
        if (SD.exists(inputstring))
        {
            File entry=SD.open(inputstring);
            sv.send(200,"text/plain",String(entry.size()));
            entry.close();
            return;
        }
    } 
    sv.send(404,"text/plain","-1");
}

uint32_t file_count_U32;
void server_setup()
{
    root=SD.open("/");
    file_count_U32=GetFileCount();
    // Send web page with input fields to client
    sv.on("/", [](){
        // Serial.println("Home Page Request");
        sv.send(200, "text/html", HTML);
        sv.client().flush();
    });

    sv.on ("/GetNameFile", [](){
        // Serial.println("Get Name File Page Web request!");
        uint32_t page_count_U32=ceil((double)file_count_U32/(double)_MAX_NUMBER_FILE_PER_PAGE_);
        Serial.println(file_count_U32);

        uint32_t request_page_U32=0;

        String inputMessage;
        if (sv.hasArg("page")) 
        {
            inputMessage = sv.arg("page");
            request_page_U32=atoi(inputMessage.c_str());
            if (request_page_U32<=page_count_U32)
            {
                char filenames[1000];
                GetFileName_Web(filenames,request_page_U32);
                char resp[1000];
                sprintf((char *)resp, HTML_FILE, filenames, request_page_U32-1, request_page_U32, page_count_U32 - 1, request_page_U32+1);
                // Serial.println("Resp Get Name File Page Web request!");
                Serial.println(resp);
                sv.send_P(200, "text/html", resp);
                sv.client().flush();
                return;
            }
        }
        
        sv.send(404, "text/html", "Invalid Page Input!");
        sv.client().flush();
    });

    sv.on("/GetFile",GetFile);

    sv.on("/DeleteAllFiles",DeleteAllFiles);

    sv.on("/GetNames",GetName);

    sv.onNotFound(NotFound);

    sv.on("/GetSize",GetSize);
}

void server_start()
{
    sv.stop();
    sv.begin();
    Serial.println("Server Started");
}

void server_stop()
{
    sv.stop();
    Serial.println("Server Stopped");
}

void server_loop()
{
    sv.handleClient();
}