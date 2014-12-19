//################################################################################################
void dataFusionWorker()
{
  pthread_t dataFusionThread;
  int iret = pthread_create(&dataFusionThread, NULL, dataFusionHandler, (void*)NULL);
  pthread_detach(dataFusionThread);
}
//################################################################################################
//################################################################################################
void *dataFusionHandler(void *arg)
{
  while(true)
  {
    if(DEBUG == 2)
    {
      Serial.println("DF loop");
      Serial.println("DF locking dataBuffer");
    }
    pthread_mutex_lock(&dataBufferMutex);  //lock mutex for dataBuffer
    
    if(dataBuffer.size() >= 5)
    {
      byte b = 0;
      
      //pop front until header is found or buffer gets too small;
      while((dataBuffer.size() >= 5) && (b != headerValue))
      {
        b = dataBuffer.front();
        dataBuffer.pop_front();
      }
      
      if(b == headerValue)
      {
        if(DEBUG == 2)
        {
          Serial.println("header found");
        }
        //check if the packet is large enough
        if(dataBuffer.size() > dataBuffer.at(1)+2)
        {
          //pop the rest into a packet
          struct moduleData m;
          
          m.ID = dataBuffer.front();
          dataBuffer.pop_front();
          m.dataLength = dataBuffer.front();
          dataBuffer.pop_front();
          for(int i =0; i < m.dataLength; i++)
          {
            m.data.push_back(dataBuffer.front());
            dataBuffer.pop_front();
          }
          m.checksum = dataBuffer.front();
          dataBuffer.pop_front();
          //m.data = dataBuffer;
          
          
          pthread_mutex_lock(&packetMutex);
          packets.push_back(m);
          pthread_mutex_unlock(&packetMutex);
          
        }
        else
        {
          //not enough data in queue. push the header byte back in front
          Serial.println(dataBuffer.at(1)+2);
          if(DEBUG == 2)
          {
            printError("not enough pushing header back in front");
          }
          dataBuffer.push_front(b);
          
        }
      }
      else
      {
        if(DEBUG ==1)
        {
          Serial.print("#");
        }
        if(DEBUG == 2)
        {
          printError("bad header");
        }
      }
    }
    pthread_mutex_unlock(&dataBufferMutex);  //unlock mutex for dataBuffer
    if(DEBUG == 2)
    {
      Serial.println("DF unlocking dataBuffer");
    }
    delay(100);
  }
}
//################################################################################################
//################################################################################################
void packetWorker()
{
  pthread_t packetThread;
  int iret = pthread_create(&packetThread, NULL, packetHandler, (void*)NULL);
  pthread_detach(packetThread);
}
//################################################################################################
//################################################################################################
void *packetHandler(void *arg)
{
  while(true)
  {
    
    if(DEBUG ==2)
    {
      Serial.println("Packet loop");
     
    }
    int availablePackets = packets.size(); //should be atomic enough to be thread safe
    while(availablePackets > 0)
    {
      if(DEBUG == 2)
      {
        Serial.print("Packets: ");
        Serial.print(availablePackets);
        Serial.println(" need to be processed");
      }
      pthread_mutex_lock(&packetMutex);
      struct moduleData m = packets.front();
      packets.pop_front();
      availablePackets = packets.size();
      pthread_mutex_unlock(&packetMutex);
      byte dataChecksum = 0;
      dataChecksum += m.ID;
      dataChecksum += m.dataLength;
      for(int i = 0; i < m.dataLength; i++)
      {
        dataChecksum += m.data.at(i);
        //dataChecksum += m.data.front();
        //m.data.pop_front();
      }
      if(dataChecksum == m.checksum)
      {
        //valid packet
        if(DEBUG)
        {
          Serial.print("Valid Module Data: ");
          Serial.println(m.ID);
        }
        pthread_mutex_lock(&validPacketMutex);
        validPackets.push_back(m);
        pthread_mutex_unlock(&validPacketMutex);
      }
      else
      {
        if(DEBUG)
        {
          printError("**************Checksum Error!************");
          Serial.print("calculated Checksum: ");
          Serial.println(dataChecksum);
          Serial.print("received Checksum: ");
          Serial.println(m.checksum);
        }
      }
    }
    delay(100);
  }
}
//################################################################################################
//################################################################################################
void udpWorker()
{
  pthread_t udpThread;
  int iret = pthread_create(&udpThread, NULL, udpHandler, (void*)NULL);
  pthread_detach(udpThread);
}
//################################################################################################
void *udpHandler(void *arg)
{
  while(true)
  {
    boolean send = false;
    //Send
    if(DEBUG == 2)
    {
      Serial.println("UDP Loop");
    }
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    
    rv = select(sockfd + 1, &readfds, NULL, NULL, &tv); 
    if(rv==-1)
    {
      if(DEBUG)
      {
          printError("Error in select!");
      }
    }
    if(rv == 0)
    {
      struct moduleData m;
      if(DEBUG == 2)
      {
        Serial.println("locking udp packets");
      }
      pthread_mutex_lock(&udpPacketMutex);
      if(!udpPackets.empty())
      {
        m = udpPackets.front();
        udpPackets.pop_front(); 
        send = true;
      }
      pthread_mutex_unlock(&udpPacketMutex);
      if(DEBUG == 2)
      {
        Serial.println("unlocking udp packets");
      }
      if(send)
      {
        byte *udpPacket;
        int packetSize = m.dataLength+4;
        udpPacket = new byte[packetSize];
        udpPacket[0] = headerValue;
        udpPacket[1] = (byte)m.ID;
        udpPacket[2] = (byte)m.dataLength;
        int i;
        for(i =0; i < (int)m.dataLength;i++)
        {
          udpPacket[3 + i] = m.data.front();
          m.data.pop_front();
        }
        udpPacket[3 + i] = m.checksum;
        sendUDPMessage(udpPacket, packetSize);
        if(DEBUG)
        {
          Serial.println("*****UDP SENT*****");
        }
        delete [] udpPacket;
      }
      //delete [] udpPacket;
    }
    
    
    //receive
    if (FD_ISSET(sockfd, &readfds))
    {
      if (recvfrom(sockfd, msg_buffer, BUFFERSIZE, 0, (struct sockaddr*)&cli_addr, &slen)==-1)
      {
        printError("recvfrom()");
        //return; // let's abort the loop
      }
   
      //parse the received msg buffer
      boolean commandFound = false;
      int i = 0;
      String command = "";
      while((!commandFound) && (i < BUFFERSIZE))
      {
        if(msg_buffer[i] == '*')
        {
          commandFound = true;
        }
        else
        {
          command += msg_buffer[i];
        }
        i++;
      }
      if(DEBUG)
      {  
        Serial.print("raw cmd: ");
        Serial.println(command);
      }
      if (DEBUG)
      { 
        Serial.println("***************************************************");     
        Serial.println("Received packet from %s:%d\nData:");
        Serial.println(inet_ntoa(cli_addr.sin_addr));
        Serial.println(msg_buffer);
        Serial.println("***************************************************");
      }
      pthread_mutex_lock(&commandQueueMutex);
      commandQueue.push_back(command);
      pthread_mutex_unlock(&commandQueueMutex);

    }
    
    delay(100);
    
  }
  
}
//################################################################################################
//################################################################################################
// this function is reponsible to send UDP datagrams
void sendUDPMessage(byte datagram[], int datagramLength)
{
  struct sockaddr_in serv_addr;
  int sockfd, i, slen=sizeof(serv_addr);
  
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
  { 
    printError("socket");
    return;
  }
  
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(WEBSERVER_UDP_PORT);
  
  // considering the sketch and the webserver run into Galileo
  // let's use the loop back address
  if (inet_aton("127.0.0.1", &serv_addr.sin_addr)==0)
  {
      printError("inet_aton() failed\n");
      close(sockfd);
      return;
  }

  if (sendto(sockfd, datagram, datagramLength, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1)
	printError("sendto()");
 
  close(sockfd);
}
//#####################################################################################################
//#####################################################################################################
// this function is responsible to init the UDP datagram server
int populateUDPServer(void)
{
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
      printError("socket");
    else 
      if (DEBUG) Serial.println("Server : Socket() successful\n");

    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(SKETCH_UDP_PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sockfd, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1)
      printError("bind");
    else
      if (DEBUG) Serial.println("Server : bind() successful\n");

    memset(msg_buffer, 0, sizeof(msg_buffer));
 
      

}
//#####################################################################################################
