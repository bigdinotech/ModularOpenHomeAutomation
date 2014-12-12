//################################################################################################
void moduleWorker()
{
  pthread_t moduleWorkerThread;
  int iret = pthread_create(&moduleWorkerThread, NULL, moduleHandler, (void*)NULL);
  pthread_detach(moduleWorkerThread);
}
//################################################################################################
//################################################################################################
void *moduleHandler(void *arg)
{
  while(true)
  {
    if(DEBUG == 2)
    {
      Serial.println("Module worker loop");
    }
    
    struct moduleData m;
    while(!validPackets.empty())  //read should be atomic enough
    {
      m.ID = -1;
      pthread_mutex_lock(&validPacketMutex);
      if(!validPackets.empty())
      {
        m = validPackets.front();  //this will also set m.ID to a valid value greater than 0
        validPackets.pop_front();
      }
      pthread_mutex_unlock(&validPacketMutex);
      if(m.ID >= 0)
      {
        //add module data to udp packets to be sent
        pthread_mutex_lock(&udpPacketMutex);
        udpPackets.push_back(m);
        pthread_mutex_unlock(&udpPacketMutex);
        
        
        //module specific tasks
        if(DEBUG)
        {
          Serial.print("Processing data from module: ");
          Serial.println(m.ID);
        }
        switch(m.ID)
        {
          case 0:
            break;
          default:
            break;
        }
      }
    }
    delay(100);
  }  
}
//################################################################################################
//################################################################################################
void commandWorker()
{
  pthread_t commandWorkerThread;
  int iret = pthread_create(&commandWorkerThread, NULL, commandHandler, (void*)NULL);
  pthread_detach(commandWorkerThread);
}
//################################################################################################
//################################################################################################
void *commandHandler(void *arg)
{
  while(true)
  {
    if(DEBUG == 2)
    {
      Serial.println("Command Loop");
    }
    String cmd = "";
    pthread_mutex_lock(&commandQueueMutex);
    if(!commandQueue.empty())
    {
      cmd =   commandQueue.front();
      commandQueue.pop_front();
    }
    pthread_mutex_unlock(&commandQueueMutex);
    
    //TODO: command hash table
    if(cmd == "tgdoor")
    {
      byte *dataBuff;
      dataBuff = new byte[1];
      dataBuff[0] = 'g';
      sendData(dataBuff, 23);
      delete [] dataBuff;
      if(DEBUG)
      {
        Serial.println("toggle garage door");
      }
    }
    else if(cmd == "tglight")
    {
      byte *dataBuff;
      dataBuff = new byte[1];
      dataBuff[0] = 'l';
      sendData(dataBuff,23 );
      delete [] dataBuff;
      if(DEBUG)
      {
        Serial.println("toggle garage light");
      }
    }
    else
    {
      //parse Command
      int cmdPacketLength = cmd.length();
      byte* cmdBuffer = new byte[cmdPacketLength];
      for(int i = 0; i < cmdPacketLength; i++)
      {
        cmdBuffer[i] = (byte)(cmd[i]);
      }
    }
    delay(100);
  }
}
//################################################################################################
//################################################################################################
void transferModuleData(struct moduleData modData)
{
}
//################################################################################################


//*******************************************Modules**********************************************
//################################################################################################
void garageModule(byte packet[])
{
}
//################################################################################################
