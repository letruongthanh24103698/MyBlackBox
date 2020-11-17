#include <Arduino.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define GPIOInit(_PIN_,_MODE_)              pinMode(_PIN_,_MODE_)
#define GPIOWrite(_PIN_,_STATE_)            digitalWrite(_PIN_,_STATE_)
#define GPIORead(_PIN_)                     digitalRead(_PIN_)
#define GPIOAttachINT(_PIN_,_FUNC_,_MODE_)  attachInterrupt(_PIN_,_FUNC_,_MODE_)

#define _GPS_EN_PIN_              4 //GPIO4
#define _SD_CS_PIN_               5 //GPIO5
#define _GPS_PPS_PIN              2 //GPIO2

int32_t RxData_I32;
SPIClass SPI_SD(VSPI);
char* Path="/datalog_ESP32.txt";
int64_t Size_I64=0;
int64_t SentenceSize_I64=0;

void writeFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void readFile(fs::FS &fs, const char * path, uint8_t *buffer_AU8, uint64_t *size){
  // Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
      Serial.println("Failed to open file for reading");
      return;
  }

  Serial.print("Read from file: ");\
  
  *size=0;
  while(file.available())
      buffer_AU8[(*size)++]=(uint8_t)(file.read());

  file.close();
}

void GPS_Receive_Handle(uint8_t ch)
{
  if (ch=='\r' || ch=='\n')
  {

  }
}

void PPS_INT_Handle()
{
  do
  {
    RxData_I32=Serial2.read();
    if (RxData_I32!=-1)
    {

    }
    else
    {
      break;
    }
    
  } while (-1);
  
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
  /*writeFile(SD, Path, "aaa");
  uint8_t temp[4];
  uint64_t size;
  readFile(SD, Path ,temp, &size);
  temp[3]=0;
  Serial.println(temp[1]);
  Serial.println(temp[2]);
  Serial.println(temp[0]);*/
  GPIOInit(_GPS_EN_PIN_,OUTPUT);
  GPIOWrite(_GPS_EN_PIN_,1);

  GPIOInit(_GPS_PPS_PIN,INPUT_PULLDOWN);
  GPIOAttachINT(_GPS_PPS_PIN,PPS_INT_Handle,FALLING);

  Serial.println("Start...");
}

void loop() {
  // put your main code here, to run repeatedly:
}