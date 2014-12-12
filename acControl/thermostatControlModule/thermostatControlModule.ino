#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"
#include "DHT.h"



//thermostat pins
#define Y1 A0
#define Y2 -1
#define W1 A1
#define W2 -1
#define G 3

//modes
#define OFF      0
#define COOL     1
#define HEAT     2
#define AUTO     3
#define MANUAL   4
#define VACATION 5

//display
#define TFT_CS   10
#define TFT_DC   8
//DHT
#define DHTPIN 4
#define DHTTYPE DHT22
//SD
#define SD_CS 7


DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 rtc;


struct thermostatSchedule
{
 int hour;
 int minute;
 int mode;
 int targetHIGHTemp;
 int targetLOWTemp;
};

byte cmdPacket[32];
byte dataPacket[32];
byte headerValue = 0b10101010;
byte modID = 10;
byte dataLength = 4;

//allow 4 shceadules per day
thermostatSchedule weeklySchedule[7][4];

DateTime now;
int lastMinute = 0;
int lastTemperature;
int lastMode = 0;
int lastColor = 0;

int mode = 3;
int temporaryMode = mode;
int highTemp = 78;
int lowTemp = 68;
int targetTemperature = 78;
int temporaryTargetTemperature = targetTemperature;
int temperatureTolerance = 2;
int currentTemperature = 74;
int currentHumidity = 0;


boolean ACOn = false;
boolean HeatOn = false;
boolean stateChanged = true;


unsigned long current= millis();
unsigned long t30000 = millis();
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
  Serial.begin(38400);
  
  Wire.begin();
  rtc.begin();
  
  dht.begin();
  
  //make initial readings
  //readSchedule();
  checkMode();  
}

void loop()
{
  current = millis();
  if((current - t50) > 50)
  {
    //dp every 50ms
    t50 = millis();
    
  }
  if((current - t500) > 500)
  {
    t500 = millis();
    if(temporaryTargetTemperature != targetTemperature)
    {
    }
  }
  if((current - t5000) > 5000)
  {
    //do every 5 sec
    t5000 = millis();
    checkSchedule();
    checkMode();

    //sendData();
    if((current - lastTargetChange)> 30000)
    {
      temporaryTargetTemperature = targetTemperature;
    }
  }
  
  if((current - t30000) > 30000)
  {
    t30000 = millis();
  }
  
  if(Serial.available()>5)
  {
    readCommand();
  }
  
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
  currentTemperature = f + 0.5;
  
  /**
  Serial.print("t: ");
  Serial.println(currentTemperature);
  **/
}
//##########################################################################################################################
//##########################################################################################################################
void turnACOn1()
{
    turnHeatOff1();
    ACOn = true;
    digitalWrite(Y1, HIGH);

}

void turnACOff1()
{
  ACOn = false;
  digitalWrite(Y1, LOW);
}

void turnHeatOn1()
{
  HeatOn = true;
  turnACOff1();
  digitalWrite(W1, HIGH);
}

void turnHeatOff1()
{
  HeatOn = false;
  digitalWrite(W1, LOW);
}

//##########################################################################################################################
//##########################################################################################################################
void readSchedule()
{

}

void writeSchedule(int day, int index)
{
}

void checkSchedule()
{
  
  //run every minute or every time schedule is changed
  getTime();
  if(mode!=4)
  {
    int dayOfWeek = now.dayOfWeek();
    //Serial.print("DOW: ");
    //Serial.println(dayOfWeek);
    for(int i = 0; i < 4; i++)
    {
      if(weeklySchedule[dayOfWeek][i].mode !=99)
      {
        Serial.print("mode: ");
        Serial.println(weeklySchedule[dayOfWeek][i].mode);
        //compare time
        if(weeklySchedule[dayOfWeek][i].hour == now.hour())
        {
          if(weeklySchedule[dayOfWeek][i].minute == now.minute())
          {
            //set new values
            applySchedule(weeklySchedule[dayOfWeek][i]);
            //delay(120000);
          }
        }
      }
    }
    //check for scheaduled events
    //set mode
    //set temperature targets
  }
  
}

void applySchedule(struct thermostatSchedule sched)
{
 
  highTemp = sched.targetHIGHTemp;
  lowTemp = sched.targetLOWTemp;
  mode = sched.mode;

}

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
  //readDHT();
  if(currentTemperature >= (highTemp + temperatureTolerance))
  {
    turnACOn1();
    targetTemperature = highTemp;
  }
  else if((currentTemperature <= (highTemp - temperatureTolerance)) && (currentTemperature >= (lowTemp + temperatureTolerance)))
  {
    turnACOff1();
    turnHeatOff1();
  }
  else if(currentTemperature <= (lowTemp - temperatureTolerance))
  {
    turnHeatOn1();
    targetTemperature = lowTemp;
  }
  else if(currentTemperature >= (lowTemp + temperatureTolerance))
  {
    turnHeatOff1();
  }
  
}

void coolMode()
{
  //readDHT();
  turnHeatOff1();
  if(currentTemperature >= (targetTemperature + temperatureTolerance))
  {
    turnACOn1();
  }
  else if(currentTemperature < (targetTemperature - temperatureTolerance))
  {
    turnACOff1();
  }
}

void heatMode()
{
  //readDHT();
  turnACOff1();
  if(currentTemperature <= (targetTemperature - temperatureTolerance))
  {
    turnHeatOn1();
  }
  else if(currentTemperature > (targetTemperature + temperatureTolerance))
  {
    turnHeatOff1();
  }
}

void offMode()
{
  turnACOff1();
  turnHeatOff1();
}

void vacationMode()
{
  //power saving mode
  //disable schedules until removed from this mode
  
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
      //do nothing
      break;    
  }
  
}

//################################################################################################
//################################################################################################
void readCommand()
{
  if(Serial.available())
  {
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
  dataChecksum += (byte)currentTemperature;
  dataChecksum += (byte)currentHumidity;
  dataChecksum += (byte)mode;
  dataChecksum += (byte)targetTemperature;
  
  Serial.write(headerValue);
  Serial.write(modID);
  Serial.write(dataLength);
  Serial.write((byte)currentTemperature);
  Serial.write((byte)currentHumidity);
  Serial.write((byte)mode);
  Serial.write((byte)targetTemperature);
  Serial.write(dataChecksum);
  Serial.flush();
}

