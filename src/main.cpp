#include <Arduino.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <WiFi.h>
#include <HTTPClient.h>

#define GPIOInit(_PIN_,_MODE_)              pinMode(_PIN_,_MODE_)
#define GPIOWrite(_PIN_,_STATE_)            digitalWrite(_PIN_,_STATE_)
#define GPIORead(_PIN_)                     digitalRead(_PIN_)
#define GPIOAttachINT(_PIN_,_FUNC_,_MODE_)  attachInterrupt(_PIN_,_FUNC_,_MODE_)

#define _GPS_EN_PIN_              4 //GPIO4
#define _SD_CS_PIN_               5 //GPIO5
#define _GPS_PPS_PIN              13 //GPIO13
#define _ONBOARD_LED_PIN_         2

#define _GPRMC_SPEED_ORDER_       7
#define _GPRMC_TIME_ORDER_        1

#define _KNOT_2_KMH_              1.852

int32_t RxData_I32;
SPIClass SPI_SD(VSPI);
char* Path="/datalog_ESP32_default.txt";
uint8_t Size_U8=0;
uint8_t SentenceSize_U8=0;
uint8_t Buffer_AU8[200];
uint8_t SentenceBuffer_AU8[200];
bool GetTimeDone_B=false;

esp32web

void writeFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        GPIOWrite(_ONBOARD_LED_PIN_,0);
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
        GPIOWrite(_ONBOARD_LED_PIN_,1);
    } else {
        Serial.println("Write failed");
        GPIOWrite(_ONBOARD_LED_PIN_,0);
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        GPIOWrite(_ONBOARD_LED_PIN_,0);
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
        GPIOWrite(_ONBOARD_LED_PIN_,1);
    } else {
        Serial.println("Append failed");
        GPIOWrite(_ONBOARD_LED_PIN_,0);
    }
    file.close();
}

void readFile(fs::FS &fs, const char * path, uint8_t *buffer_AU8, uint64_t *size){
  // Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
      Serial.println("Failed to open file for reading");
      GPIOWrite(_ONBOARD_LED_PIN_,0);
      return;
  }

  GPIOWrite(_ONBOARD_LED_PIN_,1);
  Serial.print("Read from file: ");\
  
  *size=0;
  while(file.available())
      buffer_AU8[(*size)++]=(uint8_t)(file.read());

  file.close();
}

int16_t GetCharPos(uint8_t *buf, uint8_t len, uint8_t ch)
{
  for (uint8_t i=0; i<len; i++)
  {
    if (buf[i]==ch)
      return i;
  }
  return -1;
}

void New_Speed_Handle(float speed)
{
  Serial.print("Speed :");
  Serial.println(speed);
  //TODO HANDLE SPEED CHANGE
}

void RenameFile(char *buf)
{
  char *newname="";
  sprintf(newname,"/datalog_ESP32_%s.txt",buf);
  SD.rename(Path, newname);
  memcpy(Path,newname, strlen(buf));
}

void Decode_Sentence()
{
  uint8_t *buf=SentenceBuffer_AU8;
  uint8_t len=SentenceSize_U8;
  buf[len]=0;
  appendFile(SD, Path, (char *)buf);
  int16_t pos=0;
  int16_t nextpos=0;
  float speed=-1;
  if (len<=6)
    return;
  if (memcmp(buf,"$GPRMC",6)==0)
  {
    for (uint8_t i=0; i<_GPRMC_SPEED_ORDER_; i++)
    {
      pos=GetCharPos(buf,len,',')+1;
      if (pos!=-1)
      {
        buf=buf+pos;
        len=len-pos-1;
      }
      else
        break;
    }

    nextpos=GetCharPos(buf,len,',');
    if (nextpos!=-1)
    {
      buf[nextpos]=0;
      speed=atof((char *)buf)*_KNOT_2_KMH_;
      New_Speed_Handle(speed);
    }

    for (uint8_t i=0; i<_GPRMC_TIME_ORDER_; i++)
    {
      pos=GetCharPos(buf,len,',')+1;
      if (pos!=-1)
      {
        buf=buf+pos;
        len=len-pos-1;
      }
      else
        break;
    }

    nextpos=GetCharPos(buf,len,',');
    if (nextpos!=-1 && (nextpos-pos)>0)
    {
      buf[nextpos]=0;
      float tmp=atof((char*) buf)+70000.0;
      char tmp_c[20];
      sprintf(tmp_c,"%f",tmp);
      RenameFile((char *)buf);
      GetTimeDone_B=true;
    }
  }
}

void GPS_Receive_Handle(uint8_t ch)
{
  if (ch=='\n')
  {
    SentenceSize_U8 = Size_U8;
    memcpy(SentenceBuffer_AU8, Buffer_AU8, SentenceSize_U8);
    Decode_Sentence();
    SentenceSize_U8=0;
    Size_U8=0;
  }
  else
  {
    Buffer_AU8[Size_U8++]=ch;
  }
  
}

void PPS_INT_Handle()
{
  do
  {
    RxData_I32=Serial2.read();
    if (RxData_I32!=-1)
    {
      Serial.print((char)RxData_I32);
      GPS_Receive_Handle(RxData_I32);
    }
    else
    {
      break;
    }
    
  } while (1);
  
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600);

  SPI_SD.begin(18,19,23,5);
    if(!SD.begin(_SD_CS_PIN_,SPI_SD)){
      Serial.println("Card Mount Failed");
      return;
  }
  writeFile(SD, Path, "");
  /*uint8_t temp[4];
  uint64_t size;
  readFile(SD, Path ,temp, &size);
  temp[3]=0;
  Serial.println(temp[1]);
  Serial.println(temp[2]);
  Serial.println(temp[0]);*/
  GPIOInit(_GPS_EN_PIN_,OUTPUT);
  GPIOWrite(_GPS_EN_PIN_,1);

  GPIOInit(_ONBOARD_LED_PIN_, OUTPUT);
  GPIOWrite(_ONBOARD_LED_PIN_,1);

  GPIOInit(_GPS_PPS_PIN,INPUT_PULLDOWN);
  GPIOAttachINT(_GPS_PPS_PIN,PPS_INT_Handle,FALLING);

  Serial.println("Start...");
  GPIOWrite(_ONBOARD_LED_PIN_,1);
}

void loop() {
  // put your main code here, to run repeatedly:
  PPS_INT_Handle();
}