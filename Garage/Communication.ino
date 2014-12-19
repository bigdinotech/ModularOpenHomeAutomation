//################################################################################################
void readCommand()
{
  int i = 0;
  int cmdID;
  sBuff = 0;
  byte dataLength = 0;
  unsigned long startingTime = millis();
  unsigned long currentTime;
  if(Serial.available()>=5)
  {
    //read the header byte
    sBuff = Serial.read();
    while((sBuff != headerValue) && (Serial.available()>=5)) 
    {
      if(DEBUG)
      {
        Serial.print("Module header error!");
      }
      sBuff = Serial.read(); //read a byte
    }
    if(sBuff == headerValue)
    {
      sBuff = Serial.read();   //read the ID byte
      packetBuffer[i] = sBuff;  //place module ID into packet buffer
      i++;
      dataLength = Serial.read();   //read the dataLength byte
      packetBuffer[i] = dataLength;  //place data length into packet buffer
      i++;
      startingTime = millis();
      //read the rest of the packet
      while(i<(dataLength + 3))
      {
        if(Serial.available())
        {
          packetBuffer[i] = Serial.read();
          i++;
        }
        else
        {
          //wait for next byte
          currentTime = millis();
          if((currentTime - startingTime) > 50)  //50 ms timeout
          {
            if(DEBUG)
            {
              Serial.print("Data Timeout!");
            }
          }
        }
      }
      cmdID = processData(packetBuffer);
      if(cmdID == modID)
      {
          byte cmd = packetBuffer[2];
          if(cmd == 'g')
          {
            toggleGarageDoor();
          }
          else if(cmd == 'l')
          {
            toggleGarageLight();
          }
          else
          {
            if(DEBUG)
            {
              Serial.println("Invalid command");
            }
          }
      }
    } 
    else
    {
      if(DEBUG)
      {
        Serial.println("Packet Error");
      }
    }
  }
  else
  {
    if(DEBUG == 2)
      {
        Serial.println("Not Enough Data");
      }
  }
}
//################################################################################################
//################################################################################################
int processData(byte packet[])
{
  byte check = 0;
  //byte header = packet[0];
  byte ID = packet[0];
  check += ID;
  byte length = packet[1];
  check += length;
  for(int x=0; x<length; x++)
  {
    check += packet[2+x];
  }
  if(packet[2+length] == check)
  {
    return ID;
  }
  else
  {
    return 0;
  }
  return 0;
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
