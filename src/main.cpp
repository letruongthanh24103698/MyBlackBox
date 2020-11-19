#include <Arduino.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "header.h"
#include "wireless.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


#define _GPRMC_SPEED_ORDER_       7
#define _GPRMC_TIME_ORDER_        1
#define _GPRMC_DATE_ORDER_        9

#define _KNOT_2_KMH_              1.852

int32_t RxData_I32;
SPIClass SPI_SD(VSPI);
char Path[100]="/datalog_ESP32_default.txt";
uint8_t Size_U8=0;
uint8_t SentenceSize_U8=0;
uint8_t Buffer_AU8[200];
uint8_t SentenceBuffer_AU8[200];
bool GetTimeDone_B=false;
bool OpenWirelessMode_B=false;
bool CloseWirelessMode_B=false;
bool IsWirelessRunning_B=false;

void writeFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        GPIOWrite(_ONBOARD_LED_PIN_,0);
        return;
    }
    if(file.print(message)){
        // Serial.println("File written");
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
        // Serial.println("Message appended");
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
  char newname[100];
  sprintf(newname,"/datalog_ESP32_%s_GMT0.txt",buf);
  // Serial.println(newname);
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

  uint8_t *datebuf=0;
  if (memcmp(buf,"$GPRMC",6)==0)
  {
    //Get Date
    buf=SentenceBuffer_AU8;
    len=SentenceSize_U8;
    for (uint8_t i=0; i<_GPRMC_DATE_ORDER_; i++)
    {
      pos=GetCharPos(buf,len,',');
      if (pos!=-1)
      {
        buf=buf+pos+1;
        len=len-pos-1;
      }
      else
        break;
    }
    
    nextpos=GetCharPos(buf,len,',');
    if (nextpos!=-1)
    {
      buf[nextpos]=0;
      datebuf=buf;
    }
    
    //Get Speed
    buf=SentenceBuffer_AU8;
    len=SentenceSize_U8;
    for (uint8_t i=0; i<_GPRMC_SPEED_ORDER_; i++)
    {
      pos=GetCharPos(buf,len,',');
      if (pos!=-1)
      {
        buf=buf+pos+1;
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

    //GetTime
    buf=SentenceBuffer_AU8;
    len=SentenceSize_U8;
    for (uint8_t i=0; i<_GPRMC_TIME_ORDER_; i++)
    {
      pos=GetCharPos(buf,len,',');
      if (pos!=-1)
      {
        buf=buf+pos+1;
        len=len-pos-1;
      }
      else
        break;
    }

    nextpos=GetCharPos(buf,len,',');
    if (nextpos!=-1 && (nextpos-pos)>0)
    {
      buf[nextpos]=0;
      char tmp_c[100];
      sprintf(tmp_c,"%s_%s",(char *)datebuf,buf);
      RenameFile((char *)tmp_c);
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
      // Serial.print((char)RxData_I32);
      GPS_Receive_Handle(RxData_I32);
    }
    else
    {
      break;
    }
    
  } while (!OpenWirelessMode_B && !IsWirelessRunning_B);
  
}

void Button_Left_Handle()
{
  OpenWirelessMode_B=true;
}

void Button_Right_Handle()
{
  CloseWirelessMode_B=true;
}

void CreateBlankFile()
{
  if (SD.exists(Path))
  {
    SD.remove(Path);
    File myfile = SD.open(Path, FILE_WRITE);
    myfile.close();
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600);

  SPI_SD.begin(18,19,23,5);
    if(!SD.begin(_SD_CS_PIN_,SPI_SD)){
      Serial.println("Card Mount Failed");
      return;
  }

  CreateBlankFile();

  /*uint8_t temp[4];
  uint64_t size;
  readFile(SD, Path ,temp, &size);*/
  GPIOInit(_GPS_EN_PIN_,OUTPUT);
  GPIOWrite(_GPS_EN_PIN_,1);

  GPIOInit(_ONBOARD_LED_PIN_, OUTPUT);
  GPIOWrite(_ONBOARD_LED_PIN_,1);

  GPIOInit(_GPS_PPS_PIN,INPUT_PULLDOWN);
  GPIOAttachINT(_GPS_PPS_PIN,PPS_INT_Handle,FALLING);

  GPIOInit(_BUTTON_LEFT_PIN_, INPUT);
  GPIOAttachINT(_BUTTON_LEFT_PIN_,Button_Left_Handle,FALLING);

  GPIOInit(_BUTTON_RIGHT_PIN_, INPUT);
  GPIOAttachINT(_BUTTON_RIGHT_PIN_,Button_Right_Handle,FALLING);

  GPIOInit(_LED_LEFT_PIN_, OUTPUT);
  GPIOWrite(_LED_LEFT_PIN_,0);

  GPIOInit(_LED_RIGHT_PIN_, OUTPUT);
  GPIOWrite(_LED_RIGHT_PIN_,0);

  Serial.println("Start...");
  GPIOWrite(_ONBOARD_LED_PIN_,1);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (OpenWirelessMode_B)
  {
    GPIOWrite(_ONBOARD_LED_PIN_,0);
    OpenWirelessMode_B=false;
    IsWirelessRunning_B=true;
    GPIOWrite(_LED_LEFT_PIN_,0);
    GPIOWrite(_LED_RIGHT_PIN_,0);
    wireless_start();
  }

  if (CloseWirelessMode_B)
  {
    GPIOWrite(_ONBOARD_LED_PIN_,1);
    CloseWirelessMode_B=false;
    IsWirelessRunning_B=false;
    GPIOWrite(_LED_LEFT_PIN_,0);
    GPIOWrite(_LED_RIGHT_PIN_,0);
    wireless_stop();
  }

  if (IsWirelessRunning_B)
  {
    wireless_loop();
  }
  else
  {
    PPS_INT_Handle();
  }
  


}