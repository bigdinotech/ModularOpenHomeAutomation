//################################################################################################
void serialWorker()
{
  pthread_t serialReaderThread;
  
  int iret = pthread_create(&serialReaderThread, NULL, readSerial, (void*)NULL);
  pthread_detach(serialReaderThread);
  
}
//################################################################################################
//################################################################################################
void *readSerial(void *arg)
{
  std::deque<byte> packetBuffer; //data buffer before pushing into queue
  while(true)
  {
    if(DEBUG == 2)
    {
      Serial.println("Serial loop");
    }
    /**
    while(Serial1.available())
    {
      packetBuffer.push_back(Serial1.read());
      //Serial.println(packetBuffer[index-1]);
    }
    **/
    if(Serial1.available() >= 23)
    {
      rx = ZBRxResponse();
      xbee.readPacket();
      if (xbee.getResponse().isAvailable()) 
      {
        if(xbee.getResponse().getApiId() == ZB_RX_RESPONSE) 
        {
          xbee.getResponse().getZBRxResponse(rx);
          int l = rx.getDataLength();
          if(DEBUG == 2)
          {
            Serial.print("data length: ");
            Serial.println(l);
          }
          for(int i = 0; i < l; i++)
          {
            packetBuffer.push_back((byte)rx.getData(i));
            if(DEBUG ==2)
            {
              Serial.print(rx.getData(i), HEX);
            Serial.print(" ");
            }
          }
          //if(moduleAddressList[rx.getData(i)].addr16 ==0)
          {
            if(DEBUG == 2)
            {
              Serial.print("header: ");
              Serial.println(rx.getData(0));
              Serial.print("id: ");
              Serial.println(rx.getData(1));
            }
            moduleAddressList[rx.getData(1)].addr64 = rx.getRemoteAddress64();
            moduleAddressList[rx.getData(1)].addr16 = rx.getRemoteAddress16();
            /**
            Serial.print("Addr16: ");
            Serial.println(rx.getRemoteAddress16(), HEX);
            
            Serial.print("MSB: ");
            Serial.println(moduleAddressList[rx.getData(1)].addr64.getMsb(), HEX);
            Serial.print("LSB: ");
            Serial.println(moduleAddressList[rx.getData(1)].addr64.getLsb(), HEX);
            
            
            Serial.print("Addr16: ");
            Serial.println(moduleAddressList[rx.getData(1)].addr16, HEX);
            **/
          }
        }
        
        //push whatever was read into queue
        if(!packetBuffer.empty())
        {
          //data was read
          pthread_mutex_lock(&dataBufferMutex);
          while(!packetBuffer.empty())
          {
            dataBuffer.push_back (packetBuffer.front());
            packetBuffer.pop_front();
          }
          pthread_mutex_unlock(&dataBufferMutex);
        }
        else
        {
          if(DEBUG == 2)
          {
            Serial.println("no data");
          }
        } 
      }
    }
    else
    {
      if(DEBUG == 2)
      {
        Serial.println("not enough data: ");
        Serial.println(Serial1.available());
      }
    }
    delay(100);  //if no data sleep for 100ms
  }
}
//################################################################################################
//################################################################################################
void sendData(byte output[], int modID)
{
  /**
  Serial.println("***TX***");
  Serial.print("MSB: ");
  Serial.println(moduleAddressList[modID].addr64.getMsb(), HEX);
  Serial.print("LSB: ");
  Serial.println(moduleAddressList[modID].addr64.getLsb(), HEX);
            
  Serial.print("Addr16: ");
  Serial.println(moduleAddressList[modID].addr16), HEX;
  **/
  addr64 = moduleAddressList[modID].addr64;
  
  zbTx = ZBTxRequest(addr64, output, sizeof(output));
  zbTx.setAddress16(moduleAddressList[modID].addr16);
  xbee.send(zbTx);
}
//################################################################################################
