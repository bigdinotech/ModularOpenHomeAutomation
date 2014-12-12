#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include "DHT.h"
#include <Adafruit_GFX.h>
#include "Adafruit_HX8357.h"
#include "TouchScreen.h"

//basic colors
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define BG       0xEF3D

//modes
#define OFF      0
#define COOL     1
#define HEAT     2
#define AUTO     3
#define MANUAL   4
#define VACATION 5

//thermostat pins
#define Y1 A0
#define Y2 -1
#define W1 A1
#define W2 -1
#define G 3

///touch
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 5   // can be a digital pin
#define XP 6   // can be a digital pin

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
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 280);


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
int modeColor = GREEN;
int currentTemperature = 74;
int currentHumidity = 0;


boolean ACOn = false;
boolean HeatOn = false;
boolean stateChanged = true;

boolean sdInitialized = false;

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
  Serial.begin(57600);
  
  Wire.begin();
  rtc.begin();
  
  dht.begin();
  tft.begin(HX8357D);
  tft.fillScreen(BG);
  
  
  
  tft.setRotation(3);
  tft.setCursor(0, 0);
  tft.setTextColor(HX8357_WHITE);  
  tft.setTextSize(5);

  if (!SD.begin(SD_CS)) 
  {
    sdInitialized = false;
  }
  else
  {
    sdInitialized = true;
  }
  
  //make initial readings
  readSchedule();
  checkMode();  
  
  //draw GUI
  displayControlButtons();
  displayTime();
  displayMode();
  displayCurrentTemperature(); 
}

void loop()
{
  current = millis();
  if((current - t50) > 50)
  {
    //dp every 50ms
    t50 = millis();
    readTouchScreen();
    
  }
  if((current - t500) > 500)
  {
    t500 = millis();
    if(temporaryTargetTemperature != targetTemperature)
    {
      displayTargetTemperature();
    }
  }
  if((current - t5000) > 5000)
  {
    //do every 5 sec
    t5000 = millis();
    checkSchedule();
    displayTime();
    checkMode();
    //displayMode();
    displayCurrentTemperature();
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
    displayCurrentTemperature();

}

void turnACOff1()
{
  ACOn = false;
  digitalWrite(Y1, LOW);
  displayCurrentTemperature();
}

void turnHeatOn1()
{
  HeatOn = true;
  turnACOff1();
  digitalWrite(W1, HIGH);
  displayCurrentTemperature();

}

void turnHeatOff1()
{
  HeatOn = false;
  digitalWrite(W1, LOW);
  displayCurrentTemperature();
}

//##########################################################################################################################
//##########################################################################################################################
void readSchedule()
{
  if(sdInitialized)
  {
    File dataFile = SD.open("schedule.txt");
    int scheduleCount = 0;
    while(dataFile.available() && (scheduleCount < 28))
    {
      Serial.println("reading card");
      for(int day = 0; day < 7; day++)
      {
        Serial.print("day: ");
        Serial.println(day);
        for(int i = 0; i < 4; i++)
        {
           int values[5];
           for(int j = 0; j < 5; j++)
           {
             values[j] = dataFile.parseInt();
             Serial.println(values[j]);
             if(values[j] == -1)
             {
               //not proper readable format
             }
           }
           weeklySchedule[day][i].hour = values[0];
           weeklySchedule[day][i].minute = values[1];
           weeklySchedule[day][i].mode = values[2];
           weeklySchedule[day][i].targetHIGHTemp = values[3];
           weeklySchedule[day][i].targetLOWTemp = values[4];
           printSchedule(weeklySchedule[day][i]);
           scheduleCount++;
        }
      }
    }
    dataFile.close();
    Serial.println("CLOSING");
  }
}

void printSchedule(struct thermostatSchedule sched)
{
  Serial.println("***************************");
  Serial.print("hour: ");
  Serial.println(sched.hour);
  Serial.print("minute: ");
  Serial.println(sched.minute);
  Serial.print("mode: ");
  Serial.println(sched.mode);
  Serial.print("HIGH: ");
  Serial.println(sched.targetHIGHTemp);
  Serial.print("LOW: ");
  Serial.println(sched.targetLOWTemp);
  Serial.println("***************************");
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
  /**
  Serial.print("new HIGH temp: ");
  Serial.println(highTemp);
  Serial.print("new LOW temp: ");
  Serial.println(lowTemp);
  Serial.print("new mode: ");
  Serial.println(mode);
  **/
}
/**
void initWeeklySchedule()
{
  //set all to invalid
  for(int i =0; i < 7; i++)
  {
    for(int j = 0; j < 4; j++)
    {
      weeklySchedule[i][j].mode = 99;
    }
  }
  //read schedule from sdcard
}
**/
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
//##########################################################################################################################
//##########################################################################################################################
//UI and UX
void displayTime()
{
  //tft.fillScreen(HX8357_BLACK);
  now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();
  if(now.minute() != lastMinute)
  {
    tft.drawRoundRect(380, 0, 100, 30, 3, BLUE);
    tft.fillRoundRect(380, 0, 100, 30, 3, BLUE);
    tft.setCursor(385, 5);
    tft.setTextColor(HX8357_WHITE);  
    tft.setTextSize(3);
    if(hour >=10)
    {
      tft.print(now.hour(), DEC);
    }
    else
    {
      tft.print(0);
      tft.print(now.hour(), DEC);
    }
    tft.print(':');
    if(minute >=10)
    {
      tft.print(now.minute(), DEC);
    }
    else
    {
      tft.print(0);
      tft.print(now.minute(), DEC);
    }
    tft.println();
    lastMinute = now.minute();
  }
  /**
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.println();
  **/
}

void displayCurrentTemperature()
{
  readDHT();
  //currentTemperature = 74;
  currentHumidity = 30;
  modeColor = GREEN;
  if(ACOn)
  {
    modeColor = BLUE;
  }
  else if(HeatOn)
  {
    modeColor = RED;
  }
  else
  {
    modeColor = GREEN;
  }
  if((lastTemperature != currentTemperature) || (lastColor != modeColor) || (mode != lastMode))
  {
    lastColor = modeColor;
    lastTemperature = currentTemperature;
    tft.drawRoundRect(150, 90, 180, 180, 5, modeColor);
    tft.fillRoundRect(150, 90, 180, 180, 5, modeColor);
    tft.setTextColor(HX8357_WHITE);  
    
    //display current temperature
    if(currentTemperature < 100)
    {
      tft.setTextSize(10);
      tft.setCursor(180, 150);
      tft.print(currentTemperature);
    }
    else
    {
      tft.setTextSize(7);
      tft.setCursor(170, 160);
      tft.print(currentTemperature);
    }
  }
  displayTargetTemperature();
}

void displayTargetTemperature()
{
  int tColor = WHITE;
  int tTemp = targetTemperature;
  if(mode == AUTO)
  {
    tft.setTextColor(WHITE); 
    //display highTemp
    tft.drawRoundRect(275, 90, 55, 30, 5, BLUE);
    tft.fillRoundRect(275, 90, 55, 30, 5, BLUE);
    tft.setTextSize(3);
    tft.setCursor(280, 95);
    tft.print(highTemp);
    
    //display lowTemp
    tft.drawRoundRect(275, 120, 55, 30, 5, RED);
    tft.fillRoundRect(275, 120, 55, 30, 5, RED);
    tft.setTextSize(3);
    tft.setCursor(280, 125); 
    tft.print(lowTemp);
  }
  else
  {
    if(temporaryTargetTemperature != targetTemperature)
    {
      tColor = YELLOW;
      tTemp = temporaryTargetTemperature;
    }
    else
    {
    }
    tft.drawRoundRect(275, 90, 55, 30, 5, modeColor);
    tft.fillRoundRect(275, 90, 55, 30, 5, modeColor);
    tft.setTextSize(3);
    tft.setCursor(280, 95);
    tft.setTextColor(tColor); 
    tft.print(tTemp);
  }
}

void displayMode()
{
  int displayModeColor = GREEN;
  String modeText = "";
  switch(mode)
  {
    case OFF:
      displayModeColor = BLACK;
      modeText = "OFF";
      break;
    case COOL:
      displayModeColor = BLUE;
      modeText = "COOL";
      break;
    case HEAT:
      displayModeColor = RED;
      modeText = "HEAT";
      break;
    case AUTO:
      displayModeColor = GREEN;
      modeText = "AUTO";
      break;
    case MANUAL:
      displayModeColor = GREEN;
      modeText = "MAN";
      break;
    default:
      displayModeColor = GREEN;
      modeText = "MODE";
      break;
  }
  if(mode != lastMode)
  {
    stateChanged = true;
    checkMode();
    displayCurrentTemperature();
    lastMode = mode;
    tft.drawRoundRect(380, 290, 100, 30, 3, displayModeColor);
    tft.fillRoundRect(380, 290, 100, 30, 3, displayModeColor);
    tft.setCursor(385, 295);
    tft.setTextColor(HX8357_WHITE);  
    tft.setTextSize(3);
    tft.print(modeText);
  }
}

void displayControlButtons()
{
  //draw raise temperature arrow
  tft.drawTriangle(390, 100, 430, 50, 470, 100, RED);
  tft.fillTriangle(390, 100, 430, 50, 470, 100, RED);
  
  //draw lower temperature arrow
  tft.drawTriangle(390, 120, 430, 170, 470, 120, BLUE);
  tft.fillTriangle(390, 120, 430, 170, 470, 120, BLUE);
  
  //draw OK button
  tft.drawRoundRect(380, 190, 100, 30, 3, CYAN);
  tft.fillRoundRect(380, 190, 100, 30, 3, CYAN);
  tft.setCursor(380, 195);
  tft.setTextColor(HX8357_WHITE);  
  tft.setTextSize(3);
  tft.print("  OK");
  
  //draw cancel button
  tft.drawRoundRect(380, 240, 100, 30, 3, CYAN);
  tft.fillRoundRect(380, 240, 100, 30, 3, CYAN);
  tft.setCursor(385, 247);
  tft.setTextColor(HX8357_WHITE);  
  tft.setTextSize(2);
  tft.print(" CANCEL");
}

void displayMenuButton()
{
}

void displaySchedules()
{
}

int displayYesNoPrompt()
{
  return 1;
}

void readTouchScreen()
{
  TSPoint p = ts.getPoint();

  //map touchscreen to pixels
  if (p.z > ts.pressureThreshhold)
  {
    int xValue = map(p.y, 930, 90, 0, 480);
    int yValue = map(p.x, 150, 900, 0, 320);

    Serial.print("X: ");
    Serial.print(xValue);
    Serial.print("\tY: ");
    Serial.println(yValue);

    //determine which button is pressed
    if(xValue > 380)
    {
      if((yValue >= 50) &&(yValue <= 100))
      {
        if(mode != AUTO)
        {
          temporaryTargetTemperature++;
          lastTargetChange = millis();
        }
      }
      else if((yValue >= 120) &&(yValue <= 170))
      {
        if(mode != AUTO)
        {
          temporaryTargetTemperature--;
          lastTargetChange = millis();
        }
      }
       else if((yValue >= 190) &&(yValue <= 220))
      {
        targetTemperature = temporaryTargetTemperature;
        displayTargetTemperature();
        //delay(50);
      }
      else if(yValue >= 290)
      {
        //mode button
        mode++;
        
        if(mode >= 5)
        {
          mode = 0;
        }
        displayMode();
        delay(500);
      }
      
    }
   else if((xValue > 150) && (xValue < 330))
   {
     if((yValue >=90) && yValue <= 270)
     {
       //BIG
     } 
   } 
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

