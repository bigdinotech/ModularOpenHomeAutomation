#include <Wire.h>
//#include "RTClib.h"
#include "DHT.h"
#include "U8glib.h"

#define DEBUG 0

//modes
#define OFF      0
#define COOL     1
#define HEAT     2
#define AUTO     3
#define MANUAL   4
#define VACATION 5

//thermostat pins
#define W1 5
#define Y1 6
#define G 3

//DHT
#define DHTPIN 4
#define DHTTYPE DHT22

//buttons
#define upButton 9
#define downButton 10
#define utilButton 11

DHT dht(DHTPIN, DHTTYPE, 3);
//RTC_DS1307 rtc;
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NO_ACK);	// Display which does not send ACK


byte readBuffer[32];
byte cmdPacket[32];
byte dataPacket[32];
byte headerValue = 0b10101010;
byte modID = 10;
byte dataLength = 4;


//DateTime now;
int lastMinute = 0;
int lastTemperature;
int lastMode = 0;
int lastColor = 0;

int mode = 0;
int temporaryMode = mode;
int temporaryTargetTemperature;
int targetTemperature;
int temperatureTolerance = 2;
int coolTemp = 78;
int heatTemp = 68;
int currentTemperature = 0;
int currentHumidity = 0;
int maxHeatValue = 84;
int minCoolValue = 60;
int calibrationFactor = -2;

int heatOnTime = 0;
int coolOnTime = 0;
int heatRestTime = 0;
int coolRestTime = 0;
int coolMaxOnTime = 120;
int heatMaxOnTime = 120;
int restTime = 10;


boolean ACOn = false;
boolean HeatOn = false;
boolean ACRest = false;
boolean HeatRest = false;
boolean stateChanged = true;

unsigned long current= millis();
unsigned long t60000 = millis();
unsigned long t5000 = millis();
unsigned long t500 = millis();
unsigned long t50 = millis();
unsigned long lastTargetChange = millis();


//##########################################################################################################################
//##########################################################################################################################

void setup()
{
  
  pinMode(Y1, OUTPUT);
  pinMode(W1, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(upButton, INPUT);
  pinMode(downButton, INPUT);
  pinMode(utilButton, INPUT);
  
  Serial.begin(38400);
  
  Wire.begin();
  //rtc.begin();
  
  dht.begin();

  checkMode();
  
  readDHT();
  temporaryTargetTemperature = targetTemperature;
  lastTemperature = currentTemperature;
  displayScreen(targetTemperature);  
}

void loop()
{
  current = millis();
  if((current - t50) > 50)
  {
    //dp every 50ms
    t50 = millis();
    readButtons();
    
  }
  if((current - t5000) > 5000)
  {
    //do every 5 sec
    t5000 = millis();
    checkMode();
    readDHT();
    sendData();
    if(currentTemperature != lastTemperature)
    {
      displayScreen(targetTemperature);
      lastTemperature = currentTemperature;
    }  
    if(((current - lastTargetChange)> 10000) && (temporaryTargetTemperature != targetTemperature))
    {
      temporaryTargetTemperature = targetTemperature;
      displayScreen(targetTemperature);
    }
    if(DEBUG)
    {
      Serial.print("Mode: ");
      Serial.println(mode);
      Serial.print("cool temp: ");
      Serial.println(coolTemp);
      Serial.print("heat temp: ");
      Serial.println(heatTemp);
    }
  }
  if((current - t60000) > 60000)
  {
    t60000 = millis();
    if(ACOn)
    {
      coolOnTime++;
      if(coolOnTime >= coolMaxOnTime)
      {
        ACRest = true;
        coolRestTime = restTime;
      }
    }
    else
    {
      if(coolRestTime >0)
      {
        coolRestTime--;
      }
      else
      {
        ACRest = false;
      }
    }
    
    if(HeatOn)
    {
      heatOnTime++;
      if(heatOnTime >= heatMaxOnTime)
      {
        HeatRest = true;
        heatRestTime = restTime;
      }
    }
    else
    {
      if(heatRestTime >0)
      {
        heatRestTime--;
      }
      else
      {
        HeatRest = false;
      }
    }
  }
  
  if(Serial.available()>=6)
  {
    readCommand();
  }
  

  //Serial.println(millis());
  //delay(1000);
}
//##########################################################################################################################
//##########################################################################################################################
void readDHT()
{
  float h = dht.readHumidity() + 0.5;
  float t = dht.readTemperature() + 0.5;
  float f = dht.readTemperature(true);
  float hi = dht.computeHeatIndex(f, h);
  
  currentHumidity = h + 0.5;
  currentTemperature = f + 0.5 + calibrationFactor;
  
}
//##########################################################################################################################
//##########################################################################################################################
void turnACOn1()
{
    turnHeatOff1();
    if(!ACRest)
    {
      ACOn = true;
      digitalWrite(Y1, HIGH);
    }
    else
    {
      turnACOff1();
    }
}

void turnACOff1()
{
  ACOn = false;
  digitalWrite(Y1, LOW);
}

void turnHeatOn1()
{
  turnACOff1();
  if(!HeatRest)
  {
    HeatOn = true;
    digitalWrite(W1, HIGH);
  }
  else
  {
    turnHeatOff1();
  }
}

void turnHeatOff1()
{
  HeatOn = false;
  digitalWrite(W1, LOW);
}

//##########################################################################################################################
//##########################################################################################################################

void getTime()
{
  //now = rtc.now();
  
  //display time on LCD
}

void setTime()
{
}


//##########################################################################################################################
//##########################################################################################################################
void autoMode()
{
  if(currentTemperature >= (coolTemp + temperatureTolerance))
  {
    turnACOn1();
  }
  else if((currentTemperature <= (coolTemp - temperatureTolerance)) && (currentTemperature >= (heatTemp + temperatureTolerance)))
  {
    turnACOff1();
    turnHeatOff1();
  }
  else if(currentTemperature <= (heatTemp - temperatureTolerance))
  {
    turnHeatOn1();
    targetTemperature = heatTemp;
  }
  else if(currentTemperature >= (heatTemp + temperatureTolerance))
  {
    turnHeatOff1();
  }
  
}

void coolMode()
{
  turnHeatOff1();
  if(currentTemperature >= (coolTemp + temperatureTolerance))
  {
    turnACOn1();
  }
  else if(currentTemperature <= coolTemp)
  {
    turnACOff1();
  }
  targetTemperature = coolTemp;
}

void heatMode()
{
  turnACOff1();
  if(currentTemperature <= (heatTemp - temperatureTolerance))
  {
    turnHeatOn1();
  }
  else if(currentTemperature >= heatTemp)
  {
    turnHeatOff1();
  }
  targetTemperature = heatTemp;
}

void offMode()
{
  turnACOff1();
  turnHeatOff1();
}


void checkMode()
{
  switch(mode)
  {
    case OFF:
      break;
    case COOL:
      coolMode();
      break;
    case HEAT:
      heatMode();
      break;
    case AUTO:
      //Serial.println("Auto Mode");
      autoMode();
      break;
    case MANUAL:
      //Serial.println("Manual");
      break;
    case VACATION:
      break;
    default:
      break;    
  }
  
  if(coolTemp < minCoolValue)
  {
    coolTemp = minCoolValue;
  }
  else if(heatTemp > maxHeatValue)
  {
    heatTemp = maxHeatValue;
  }
  
}
//##########################################################################################################################
//##########################################################################################################################
//UI and UX
/**
void displayTime()
{
  now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();
  if(now.minute() != lastMinute)
  {
    lastMinute = now.minute();
  }
}
**/
void displayScreen(int targetTemp)
{
  
  u8g.firstPage();
  
  do 
  {
    drawTemperatureValue(currentTemperature);
    drawTargetTemperature(targetTemp);
    displayMode(mode);
  } 
  while( u8g.nextPage() );
}

void drawTemperatureValue(int value)
{
  String temperatureValue = String(value);
  u8g.setFont(u8g_font_gdr30r);
  int length = temperatureValue.length();
  for(int i = 0; i < length; i++)
  {
    switch(temperatureValue[i])
    {
      case '0':
        u8g.drawStr(i*22+42, 50, "0");
        break;
      case '1':
        u8g.drawStr(i*22+42, 50, "1");
        break;
      case '2':
        u8g.drawStr(i*22+42, 50, "2");
        break;
      case '3':
        u8g.drawStr(i*22+42, 50, "3");
        break;
      case '4':
        u8g.drawStr(i*22+42, 50, "4");
        break;
      case '5':
        u8g.drawStr(i*22+42, 50, "5");
        break;
      case '6':
        u8g.drawStr(i*22+42, 50, "6");
        break;
      case '7':
        u8g.drawStr(i*22+42, 50, "7");
        break;
      case '8':
        u8g.drawStr(i*22+42, 50, "8");
        break;
      case '9':
        u8g.drawStr(i*22+42, 50, "9");
        break;
      default:
        break;
    }
  }
}

void drawTargetTemperature(int value)
{
  String temperatureValue = String(value);
  u8g.setFont(u8g_font_9x15);
  int length = temperatureValue.length();
  for(int i = 0; i < length; i++)
  {
    switch(temperatureValue[i])
    {
      case '0':
        u8g.drawStr(i*9+101, 15, "0");
        break;
      case '1':
        u8g.drawStr(i*9+101, 15, "1");
        break;
      case '2':
        u8g.drawStr(i*9+101, 15, "2");
        break;
      case '3':
        u8g.drawStr(i*9+101, 15, "3");
        break;
      case '4':
        u8g.drawStr(i*9+101, 15, "4");
        break;
      case '5':
        u8g.drawStr(i*9+101, 15, "5");
        break;
      case '6':
        u8g.drawStr(i*9+101, 15, "6");
        break;
      case '7':
        u8g.drawStr(i*9+101, 15, "7");
        break;
      case '8':
        u8g.drawStr(i*9+101, 15, "8");
        break;
      case '9':
        u8g.drawStr(i*9+101, 15, "9");
        break;
      default:
        break;
    }
  }
    
}

void displayMode(int mode)
{
  u8g.setFont(u8g_font_9x15);
  switch(mode)
  {
    case OFF:
      u8g.drawStr(92, 64, "OFF");
      break;
    case COOL:
      u8g.drawStr(92, 64, "COOL");
      break;
    case HEAT:
      u8g.drawStr(92, 64, "HEAT");
      break;
    case AUTO:
      u8g.drawStr(92, 64, "AUTO");
      break;
    default:
      u8g.drawStr(92, 64, "UNK");
      break;
  }
}

//################################################################################################
//################################################################################################
void readCommand()
{
  if(Serial.available()>=6)
  {
    if(DEBUG)
    {
      Serial.println("Data available");
    }
    byte readChecksum = 0;
    for(int i = 0; Serial.available(); i++)
    {
      readBuffer[i] = Serial.read();
    }
    if(readBuffer[0] == headerValue)
    {
      if(readBuffer[1] == modID)
      {
        readChecksum += modID;
        int length = readBuffer[2];
        readChecksum += length;
        for(int i =0 ;i < length; i++)
        {
          readChecksum +=readBuffer[3+i];
        }
        if(readChecksum == readBuffer[3+length])
        {
          //valid command
          byte cmd = readBuffer[3];
          switch(cmd)
          {
            case 't':
              {
                //fahrenheit
                int tMode = readBuffer[4];
                if(tMode <= 4)
                {
                  mode = tMode;
                  switch(tMode)
                  {
                    case OFF:
                      break;
                    case COOL:
                      coolTemp = readBuffer[5];
                      break;
                    case HEAT:
                      heatTemp = readBuffer[5];
                      break;
                    case AUTO:
                      heatTemp = readBuffer[5];
                      coolTemp = readBuffer[6];
                      break;
                    default:
                      break;
                  } 
                }
              }
              break;
            case 'm':
              {
                //celsius
                int tMode = readBuffer[4];
                if(tMode <= 4)
                {
                  mode = tMode;
                  switch(tMode)
                  {
                    case OFF:
                      break;
                    case COOL:
                      coolTemp = celsiusToFahrenheit(readBuffer[5]);
                      break;
                    case HEAT:
                      heatTemp = celsiusToFahrenheit(readBuffer[5]);
                      break;
                    case AUTO:
                      heatTemp = celsiusToFahrenheit(readBuffer[5]);
                      coolTemp = celsiusToFahrenheit(readBuffer[6]);
                      break;
                    default:
                      break;
                  }  
                }
              }
              break;
            default:
              break;
          }
        }
        else
        {
          //bad checksum
          if(DEBUG)
          {
            Serial.print("checksum error: ");
            Serial.print(readChecksum, HEX);
            Serial.print("!=");
            Serial.println(readBuffer[3+length], HEX);
          }
        }
      }
      else
      {
        //wrong modules
        if(DEBUG)
        {
          Serial.println("wrong module!");
        }
      }
    }
    else
    {
      //header error
      if(DEBUG)
      {
        Serial.println("header error!");
      }
    }
  }
  else
  {
  }
}

void sendData()
{
  //readDHT();
  byte dataChecksum = 0;
  dataChecksum += modID;
  dataChecksum += dataLength;
  dataChecksum += (byte)fahrenheitToCelsius(currentTemperature);
  dataChecksum += (byte)currentHumidity;
  dataChecksum += (byte)mode;
  dataChecksum += (byte)fahrenheitToCelsius(targetTemperature);
  
  Serial.write(headerValue);
  Serial.write(modID);
  Serial.write(dataLength);
  Serial.write((byte)fahrenheitToCelsius(currentTemperature));
  Serial.write((byte)currentHumidity);
  Serial.write((byte)mode);
  Serial.write((byte)fahrenheitToCelsius(targetTemperature));
  Serial.write(dataChecksum);
  Serial.flush();
}

//################################################################################################
//################################################################################################

void readButtons()
{
  if(!digitalRead(upButton))
  {
    temporaryTargetTemperature++;
    lastTargetChange = millis();
    displayScreen(temporaryTargetTemperature);
  }
  else if(!digitalRead(downButton))
  {
    temporaryTargetTemperature--;
    lastTargetChange = millis();
    displayScreen(temporaryTargetTemperature);
  }
  else if(!digitalRead(utilButton))
  {
    unsigned long t = millis();
    if(((t - lastTargetChange) > 5000) || (temporaryTargetTemperature == targetTemperature))
    {
      mode++;
      if(mode > 3)
      {
        mode = 0;
      }
      checkMode();
      temporaryTargetTemperature = targetTemperature;
      displayScreen(targetTemperature);
      delay(250);
    }
    else
    {
      switch(mode)
      {
        case COOL:
          coolTemp = temporaryTargetTemperature;
          break;
        case HEAT:
          heatTemp = temporaryTargetTemperature;
          break;
        case AUTO:
          coolTemp = temporaryTargetTemperature;
          heatTemp = temporaryTargetTemperature;
          break;
        default:
          break;
        displayScreen(temporaryTargetTemperature);
      }
      delay(500);
    }
  }
}
//################################################################################################
//################################################################################################
int fahrenheitToCelsius(int fahrenheit)
{
  double temp = fahrenheit - 32;
  return (int)((temp * 5.0/9.0) + 0.5);
}

int celsiusToFahrenheit(int celsius)
{
  double temp = celsius * (9.0/5.0);
  return (int)(temp + 32.5);
}
