#include "DHT.h"

#define garagedoor_pin 4
#define garagelight_pin 3
#define DHTPIN 8 

#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE, 3);

boolean validTemp = false;

byte cmdPacket[10];
byte dataPacket[10];
byte headerValue = 0b10101010;
byte modID = 23;
byte dataLength = 3;

int temperature;
int humidity;
int garageDoorState = 0;

unsigned long t1 = 0;
unsigned long t2 = 0;
//################################################################################################
//################################################################################################
void setup()
{
  Serial.begin(38400);
  pinMode(garagelight_pin, OUTPUT);
  pinMode(garagedoor_pin, OUTPUT);
  digitalWrite(garagelight_pin, LOW);
  digitalWrite(garagedoor_pin, LOW);
  dht.begin();
}

void loop()
{
  switch(readCommand())
  {
    case 'g':
      toggleGarageDoor();
      break;
    case 'l':
      toggleGarageLight();
      break;
    default:
      break;
      //nothing
  }
  t2 = millis();
  if((t2 -t1) > 5000)
  {
    t1 = millis();
    sendGarageData();
  }
  else if(t1 > t2)
  {
    //overflow
    t1 = millis();
    t2 = t1;
  }
  delay(100);
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
void toggleGarageDoor()
{
  digitalWrite(garagedoor_pin, HIGH);
  delay(200);
  digitalWrite(garagedoor_pin, LOW);
}
//################################################################################################
//################################################################################################
void toggleGarageLight()
{
  digitalWrite(garagelight_pin, HIGH);
  delay(200);
  digitalWrite(garagelight_pin, LOW);
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
}
//################################################################################################
//################################################################################################
void determineDoorState()
{
  //determines if garage door is open or not using an ultrasonic proximity sensor
  //0 = closed, 1 = open, 2 = UNK
  
  long duration, inches, cm;
  
  //TRIG
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  delayMicroseconds(2);
  digitalWrite(6, HIGH);
  delayMicroseconds(5);
  digitalWrite(6, LOW);
  
  //ECHO
  pinMode (7, INPUT);
  duration = pulseIn(7, HIGH);
  
  inches = microsecondsToInches(duration);
 
  if(inches > 20)
  {
    garageDoorState =  1;
  }
  else
  {
    garageDoorState =  0;
  }
}

long microsecondsToInches(long microseconds)
{
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}
//################################################################################################
//################################################################################################
void readMeshData()
{
}
//################################################################################################
//################################################################################################
void sendGarageData()
{
  //temperature = 26;
  //humidity = 38;
  readDHT();
  determineDoorState();
  byte dataChecksum = 0;
  dataChecksum += modID;
  dataChecksum += dataLength;
  dataChecksum += (byte)temperature;
  dataChecksum += (byte)humidity;
  dataChecksum += (byte)garageDoorState;
  
  Serial.write(headerValue);
  Serial.write(modID);
  Serial.write(dataLength);
  Serial.write((byte)temperature);
  Serial.write((byte)humidity);
  Serial.write((byte)garageDoorState);
  Serial.write(dataChecksum);
  Serial.flush();
  
}
//################################################################################################
