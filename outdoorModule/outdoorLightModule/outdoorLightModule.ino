#include "DHT.h"

#define PIRPIN 10
#define lightCtrl 9
#define DHTPIN 11


#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE, 3);

boolean validTemp = false;

byte cmdPacket[10];
byte dataPacket[10];
byte headerValue = 0b10101010;
byte modID = 24;
byte dataLength = 4;

int temperature;
int humidity;

unsigned long t1 = 0;
unsigned long t2 = 0;
unsigned long lastMovementDetected = 0;

boolean movementDetected = false;
boolean lightState = false;
//################################################################################################
//################################################################################################
void setup()
{
  Serial.begin(38400);
  pinMode(lightCtrl, OUTPUT);
  pinMode(PIRPIN, INPUT);
  digitalWrite(lightCtrl, LOW);
  dht.begin();
}
void loop()
{
  t2 = millis();
  if((t2 -t1) > 5000)
  {
    t1 = millis();
    sendData();
  }
  else if(t1 > t2)
  {
    //overflow
    t1 = millis();
    t2 = t1;
  }
  checkPIR();
  if(movementDetected)
  {
    //no movement in last 30 seconds
    turnONLight();
  }
  else
  {
    turnOFFLight();
  }
}
//################################################################################################
//################################################################################################
void sendData()
{
  readDHT();
  checkPIR();
  byte dataChecksum = 0;
  dataChecksum += modID;
  dataChecksum += dataLength;
  dataChecksum += (byte)temperature;
  dataChecksum += (byte)humidity;
  dataChecksum += (byte)movementDetected;
  dataChecksum += (byte)lightState;
  
  Serial.write(headerValue);
  Serial.write(modID);
  Serial.write(dataLength);
  Serial.write((byte)temperature);
  Serial.write((byte)humidity);
  Serial.write((byte)movementDetected);
  Serial.write((byte)lightState);
  Serial.write(dataChecksum);
  Serial.flush();
}
//################################################################################################
//################################################################################################
char readCommand()
{
  if(Serial.available())
  {
    return (char)Serial.read();
  }
  else
  {
    return 'x';
  }
}
//################################################################################################
//################################################################################################
void readDHT()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  temperature = 0;
  humidity = 0;
  validTemp = false;
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) 
  {
    //Serial.println("Failed to read from DHT sensor!");
  }
  else
  {
    validTemp = true;
  }
  if(validTemp)
  {
    temperature = t + 0.5;
    humidity = h + 0.5;
  }
  else
  {
  }
}
//################################################################################################
//################################################################################################
void turnONLight()
{
  digitalWrite(lightCtrl, HIGH);
  lightState = true;
}
//################################################################################################
//################################################################################################
void turnOFFLight()
{
  digitalWrite(lightCtrl, LOW);
  lightState = false;
}
//################################################################################################
void checkPIR()
{
  t2 = millis();
  if(digitalRead(PIRPIN))
  {
    lastMovementDetected = millis();
  }
  if(t2 < lastMovementDetected)
  {
    lastMovementDetected = millis();
    t2 = millis();
  }
  if((t2 - lastMovementDetected) > 30000)
  {
    movementDetected = false;
  }
  else
  {
    movementDetected = true;
  }
}
//################################################################################################
//################################################################################################


