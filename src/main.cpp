#include <Arduino.h>

#include <WiFi.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "header.h"
#include "wireless.h"
#include "Server.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define _GPRMC_SPEED_ORDER_       7
#define _GPRMC_TIME_ORDER_        1
#define _GPRMC_DATE_ORDER_        9

#define _KNOT_2_KMH_              1.852

#define _SPEED_THRESH_HOLD_       40.0
#define _SPEED_THRESH_HOLD_DELTA_ 1.0

int32_t RxData_I32;
SPIClass SPI_SD(VSPI);
char Path[100]="/datalog_ESP32_default.txt";
uint8_t Size_U8=0;
uint8_t SentenceSize_U8=0;
uint8_t GPRMCSize_U8=0;
uint8_t Buffer_AU8[200];
uint8_t SentenceBuffer_AU8[200];
uint8_t GPRMCBuffer_AU8[200];
bool GetTimeDone_B=false;
bool ChangeModeWirelessWIFI_B=false;
bool ChangeModeWirelessBLE_B=false;
bool IsWirelessRunning_B=false;
bool IsWirelessWIFIRunning_B=false;
bool IsWirelessBLERunning_B=false;
bool DecodeTime_B=false;
bool SaveTime_B=false;

TaskHandle_t Task1;

void writeFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        if (!IsWirelessRunning_B)
          GPIOWrite(_ONBOARD_LED_PIN_,0);
        return;
    }
    if(file.print(message)){
        if (!IsWirelessRunning_B)
        {
          Serial.println("File written");
          GPIOWrite(_ONBOARD_LED_PIN_,1);
        }
    } else {
        Serial.println("Write failed");
        if (!IsWirelessRunning_B)
          GPIOWrite(_ONBOARD_LED_PIN_,0);
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        if (!IsWirelessRunning_B)
          GPIOWrite(_ONBOARD_LED_PIN_,0);
        return;
    }
    if(file.print(message)){
        if (!IsWirelessRunning_B)
        {
          Serial.println("File appended");
          GPIOWrite(_ONBOARD_LED_PIN_,1);
        }
    } else {
        Serial.println("Append failed");
        if (!IsWirelessRunning_B)
          GPIOWrite(_ONBOARD_LED_PIN_,0);
    }
    file.close();
}

void readFile(fs::FS &fs, const char * path, uint8_t *buffer_AU8, uint64_t *size){
  // Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    if (!IsWirelessRunning_B)
        GPIOWrite(_ONBOARD_LED_PIN_,0);
    return;
  }

  if (!IsWirelessRunning_B) 
  {
    GPIOWrite(_ONBOARD_LED_PIN_,1);
    Serial.print("Read from file: ");
  }
  
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

  if (speed>=(_SPEED_THRESH_HOLD_ - _SPEED_THRESH_HOLD_DELTA_) && speed<=(_SPEED_THRESH_HOLD_ + _SPEED_THRESH_HOLD_DELTA_))
  {
    GPIOWrite(_LED_LEFT_PIN_, 1);
    GPIOWrite(_LED_RIGHT_PIN_, 0);
  }
  else if (speed > (_SPEED_THRESH_HOLD_ + _SPEED_THRESH_HOLD_DELTA_))
  {
    GPIOWrite(_LED_LEFT_PIN_, 1);
    GPIOWrite(_LED_RIGHT_PIN_, 1);
  }
  else
  {
    GPIOWrite(_LED_LEFT_PIN_, 0);
    GPIOWrite(_LED_RIGHT_PIN_, 0);
  }
}

void RenameFile(char *buf)
{
  vTaskDelay(10);
  char newname[100]="/datalog_ESP32_";
  // sprintf(newname,"/datalog_ESP32_1234567890123456_GMT0.txt",buf);
  memcpy(newname+15, buf, 16);
  memcpy(newname+31, "_GMT0.txt", 9);
  Serial.println(newname);
  SD.rename(Path, newname);
  memcpy(Path,newname, strlen(newname)+1);
}

void Save_Sentence()
{
    appendFile(SD, Path, (char *)SentenceBuffer_AU8);
    Serial.print((char *)SentenceBuffer_AU8);
    SaveTime_B=false;
}

void Decode_Sentence()
{
  DecodeTime_B=false;
  uint8_t *buf=GPRMCBuffer_AU8;
  uint8_t len=GPRMCSize_U8;
  buf[len]=0;
  int16_t pos=0;
  int16_t nextpos=0;
  float speed=-1;

  if (len<=6)
    return;

  uint8_t *datebuf=0;
  if (memcmp(buf,"$GPRMC",6)==0)
  {
    //Get Date
    if (!GetTimeDone_B)
    {
      buf=GPRMCBuffer_AU8;
      len=GPRMCSize_U8;
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
      if (nextpos!=-1 && (nextpos-pos)>0)
      {
        buf[nextpos]=0;
        datebuf=buf;
      }
    }
    
    //Get Speed
    buf=GPRMCBuffer_AU8;
    len=GPRMCSize_U8;
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
    if (!GetTimeDone_B)
    {
      buf=GPRMCBuffer_AU8;
      len=GPRMCSize_U8;
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
        memcpy(tmp_c,datebuf,6);
        tmp_c[6]='_';
        memcpy(tmp_c+7,buf,9);
        tmp_c[16]=0;
        RenameFile((char *)tmp_c);
        GetTimeDone_B=true;
      }
    }
  }
}

void GPS_Receive_Handle(uint8_t ch)
{
  Buffer_AU8[Size_U8++]=ch;
  if (ch=='\n')
  {
    Buffer_AU8[Size_U8]=0;
    SentenceSize_U8 = Size_U8;
    memcpy(SentenceBuffer_AU8, Buffer_AU8, SentenceSize_U8+1);
    SaveTime_B=true;
    if (memcmp(Buffer_AU8,"$GPRMC",6)==0)
    {
      GPRMCSize_U8 = Size_U8;
      memcpy(GPRMCBuffer_AU8, Buffer_AU8, GPRMCSize_U8+1);
      // Decode_Sentence();
      DecodeTime_B=true;
    }
    Size_U8=0;
  }  
}

void PPS_INT_Handle()
{
  if (IsWirelessRunning_B)
  {
    Serial.flush();
    return;
  }

  while (!IsWirelessRunning_B && !SaveTime_B)
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
    
  }
}

void Button_Left_Handle()
{
  ChangeModeWirelessWIFI_B=true;
}

void Button_Right_Handle()
{
  ChangeModeWirelessBLE_B=true;
}

void CreateBlankFile()
{
  if (SD.exists(Path))
  {
    SD.remove(Path);
    File myfile = SD.open(Path, FILE_WRITE);
    myfile.close();
  }
  else
  {
    File myfile = SD.open(Path, FILE_WRITE);
    myfile.close();
  }

}

void Task1code( void * pvParameters )
{
  for(;;)
  {
    if (SaveTime_B)
      Save_Sentence();
    if (DecodeTime_B)
      Decode_Sentence();
    vTaskDelay(10);
    yield();
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
  /*File root=SD.open("/");
  uint32_t cnt=0;
  while (1)
  {
    File entry=root.openNextFile();
    if (!entry)
      break;
    SD.remove(entry.name());
    Serial.println(cnt++);
  }
  Serial.println("Delete Done");*/
  CreateBlankFile();

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

  server_setup();

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */      

  Serial.println("Start...");
  GPIOWrite(_ONBOARD_LED_PIN_,1);
  Serial.print("1Ram Left:");
  Serial.println(esp_get_free_heap_size());
  // OpenWirelessMode_B=true;

  wireless_setup();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (ChangeModeWirelessWIFI_B)
  {
    ChangeModeWirelessWIFI_B=false;
    IsWirelessWIFIRunning_B=!IsWirelessWIFIRunning_B;
    if (IsWirelessWIFIRunning_B)
    {
      GPIOWrite(_LED_LEFT_PIN_,0);
      GPIOWrite(_LED_RIGHT_PIN_,0);
      wirelessWIFI_start();
      Serial.print("2Ram Left:");
      Serial.println(esp_get_free_heap_size());
    }
    else
    {
      GPIOWrite(_LED_LEFT_PIN_,0);
      GPIOWrite(_LED_RIGHT_PIN_,0);
      wirelessWIFI_stop();
      Serial.print("3Ram Left:");
      Serial.println(esp_get_free_heap_size());
    }
  }

  if (ChangeModeWirelessBLE_B)
  {
    ChangeModeWirelessBLE_B=false;
    IsWirelessBLERunning_B=!IsWirelessBLERunning_B;
    if (IsWirelessBLERunning_B)
    {
      GPIOWrite(_LED_LEFT_PIN_,0);
      GPIOWrite(_LED_RIGHT_PIN_,0);
      wirelessBLE_start();
      Serial.print("4Ram Left:");
      Serial.println(esp_get_free_heap_size());
    }
    else
    {
      GPIOWrite(_LED_LEFT_PIN_,0);
      GPIOWrite(_LED_RIGHT_PIN_,0);
      wirelessBLE_stop();
      Serial.print("5Ram Left:");
      Serial.println(esp_get_free_heap_size());
    }
  }

  IsWirelessRunning_B=IsWirelessBLERunning_B | IsWirelessWIFIRunning_B;
  if (IsWirelessRunning_B)
    GPIOWrite(_ONBOARD_LED_PIN_,0);
  else
    GPIOWrite(_ONBOARD_LED_PIN_,1);

  if (IsWirelessWIFIRunning_B)
  {
    wirelessWIFI_loop();
    server_loop();
  }

  if (IsWirelessBLERunning_B)
  {
    wirelessBLE_loop();
  }

  PPS_INT_Handle();
  yield();
}