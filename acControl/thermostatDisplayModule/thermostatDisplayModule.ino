#include <SPI.h>
#include <SD.h>
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
///touch
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 5   // can be a digital pin
#define XP 6   // can be a digital pin

//display
#define TFT_CS   10
#define TFT_DC   8

//SD
#define SD_CS 7

Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 280);

int lastMinute = 0;
int lastTemperature;
int lastMode = 0;
int lastColor = 0;
unsigned long lastTargetChange = millis();

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

File dataFile;
boolean sdInitialized = false;

//##########################################################################################################################
//##########################################################################################################################

void setup()
{
  Serial.begin(38400);

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
   
  //draw GUI
  displayControlButtons();
  displayMode(mode);
  displayCurrentTemperature(); 
}

void loop()
{
  if(Serial.available())
  {
    readCommand();
  }
}
//##########################################################################################################################
//##########################################################################################################################
void readCommand()
{
}
//##########################################################################################################################
//##########################################################################################################################
void readSchedule()
{
  
  //read schedule from sd card
  if(sdInitialized)
  {
    File dataFile = SD.open("schedule.txt");
    String buff = "";
    char c;
    while((c != '\n') && dataFile.available())
    {
      c = dataFile.read();
      Serial.print(c);
    }
  }
  
}

void writeSchedule(int day, int index)
{
}


//##########################################################################################################################
//##########################################################################################################################
//UI and UX
void displayTime(int hour, int minute)
{
  //tft.fillScreen(HX8357_BLACK);
  //if(now.minute() != lastMinute)
  {
    tft.drawRoundRect(380, 0, 100, 30, 3, BLUE);
    tft.fillRoundRect(380, 0, 100, 30, 3, BLUE);
    tft.setCursor(385, 5);
    tft.setTextColor(HX8357_WHITE);  
    tft.setTextSize(3);
    if(hour >=10)
    {
      tft.print(hour, DEC);
    }
    else
    {
      tft.print(0);
      tft.print(hour, DEC);
    }
    tft.print(':');
    if(minute >=10)
    {
      tft.print(minute, DEC);
    }
    else
    {
      tft.print(0);
      tft.print(minute, DEC);
    }
    tft.println();
  }
}

void displayCurrentTemperature()
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

void displayTargetTemperature(int highTemp, int lowTemp, int mode)
{
  int tColor = WHITE;
  int tTemp = highTemp;
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

void displayMode(int mode)
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
        //OK Button
        targetTemperature = temporaryTargetTemperature;
        displayTargetTemperature(targetTemperature, targetTemperature, mode);
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
        displayMode(mode);
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
