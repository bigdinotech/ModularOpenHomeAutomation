#include <pthread.h>
#include <deque> 
#include <stdio.h>
#include <ctime>

// includes for the UDP connections
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>

//XBEE Mmesh
#include <XBee.h>

struct moduleData
{
  byte ID;
  byte dataLength;
  std::deque<byte> data;
  byte checksum;
};


// debugging
int DEBUG  = 1;                // 1 to see the debug messages in the serial console, or 0 to disable

#define BUFFERSIZE 512          // UDP is limited and must be very short. 512 bytes is more than enough
#define SKETCH_UDP_PORT 2000    // this port is used to receive message events from Node.js
#define WEBSERVER_UDP_PORT 2010 // this port is used to send message events to Node.js
#define SENSOR_READ_INTERVAL 1 // number of seconds to read sensors and report to website


// for the UDP server
struct sockaddr_in my_addr, cli_addr;
int sockfd, i; 
socklen_t slen=sizeof(cli_addr);
char msg_buffer[BUFFERSIZE];
fd_set readfds;
struct timeval tv;
int rv,n;

//buffers
byte sBuff;  //temporary buffer for serial data
std::deque<byte> dataBuffer;
std::deque<moduleData> packets;
std::deque<moduleData> validPackets;
std::deque<moduleData> udpPackets;
std::deque<String> commandQueue;
int packetCount = 0; //count of valid packets in buffer

//variables for modules and sensors
byte headerValue = 170;  //0b10101010
boolean validData = false;

//
pthread_mutex_t dataBufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t packetMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t validPacketMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t udpPacketMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t commandQueueMutex = PTHREAD_MUTEX_INITIALIZER;

XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle 
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();


// SH + SL Address of receiving XBee
XBeeAddress64 addr64 = XBeeAddress64(0x00000000, 0x0000ffff);

ZBTxRequest zbTx;// = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

struct moduleAddress
{
  XBeeAddress64 addr64 = XBeeAddress64(0x00000000, 0x0000ffff);
  uint16_t addr16 = 0xFFFE;
};

moduleAddress moduleAddressList[256];
    
//#####################################################################################################    
void setup() 
{
  Serial.begin(115200); //Debug
  Serial1.begin(38400); //XBEE MESH network
  xbee.begin(Serial1);
  if(DEBUG == 1)
  {
    Serial.println("Starting Home Automation");
  }
  
  //launch workers
  serialWorker();  //starts thread for reading Serial port
  dataFusionWorker();  //starts dataFusion thread
  packetWorker();  //starts packet thread
  moduleWorker();
  udpWorker();
  commandWorker();
  // init variables for UDP server  
  populateUDPServer();
}

void loop() 
{
  
 delay(10000);
  
}

//#####################################################################################################  



//#####################################################################################################  
void printError(char *str)
{
    Serial.print("ERROR: ");
    Serial.println(str);
}
//#####################################################################################################  

//#####################################################################################################
//#####################################################################################################

