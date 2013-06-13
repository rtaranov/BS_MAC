
//2013.03.17 - Roman - Changed output new line \r\n to \n

#include "txrx_simple.h"
#include "I2C.h"
#include "stringFun.h"
#include "mrfi.h"
#include "Sensors.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "board.h"
#include "binary_notation.h"

#define NODE 2 //2:1; 1:2; 3:4; 4:5; 5!:3; 6:ADC:6

#if (NODE)
#define MASTER 0
#else
#define MASTER 1 //3
#endif

#define START_PAYLOAD_INDEX 9

#define F_CPU (BSP_CONFIG_CLOCK_MHZ_SELECT*1000000)

char T;
#define tT 2*T 

#define SERVICE_INFO_LENGTH 1+1+1+2 //service information : type + delimeter + delimeter + len(nextInitNode)
#define MAX_BUFFER_LENGTH 50


#define SOURCE_ADDR_INDEX 1
#define DESTINATION_ADDR_INDEX 5

//--------------------Network buffer vars------------------------
char buf[MAX_BUFFER_LENGTH] = {0};
char networkBuf[MAX_BUFFER_LENGTH-SERVICE_INFO_LENGTH] = {0};
unsigned char currentPacketDestination = 0;
unsigned char currentPacketSource = 0;
//---------------------------------------------------------------

extern char transmissionIsInitialized; //1-Was Transmitted; 0-No Transmission Was

void transmitDataBuf(char source, char destination);
unsigned char returnAmountOfNodesInNet();

void initVars();

char oneTime, oneTime1, nodeToRequest[3], nodeForRequest = 0;
char id;

//-------------------Counters vars-------------------
volatile unsigned int secondsCounter=0;
volatile unsigned char secondsPartCounter=0;
#if !SELF_CLUSTERING
volatile unsigned char secondsWithoutMaster = 0;
#else
volatile unsigned char secondsWhileProbing = 0;
#endif
//---------------------------------------------------------

volatile unsigned char masterCanReadAndTransmitData = 0;

volatile char mainState = NULL_STATE;

//-------------------Initialization vars-------------------
unsigned char initializationSlotNumber = 0;
unsigned char initializationAbleNode = 0;
//---------------------------------------------------------

//-------------------NODES vars-------------------
char currentNodeIsInNetwork = 0;
unsigned char currentNodeSlotNumber = 0;
char currentNodeState = NODE_IS_NONE;
//---------------------------------------------------------


//-------------------Sensors vars--------------------------
float lux, temperature;
unsigned char sensors = 0;
typedef union 
{ 
  unsigned int i;
  float f;
} 
value;

int t_temp, t_hum;
value humi_val,temp_val;
float dew_point, temperatureSHT11, hum;

int acc = 0;

char canMakeNewConversion = 0;
//---------------------------------------------------------
  

//------------------FLAGS----------------------------------
volatile unsigned char _flags;
//---------------------------------------------------------

//-----------------Self-Clustering-------------------------
char logChan = 0;
//---------------------------------------------------------

#if DEBUG
    char outputString[30];
#endif


#define MASTER_SLOT_COUNT 2
#define INITIALIZATION_SLOT_COUNT 1


char maxNodes;

char nodeCount = 0, lastSlot = 0;
char nextNode, clearTimer;  
  
char firstCycleToSelectMaster = 1;

char currentFrameSlotCount = 0;

char a2[3];

int8_t rssi;

void print_rssi(int8_t rssi)
{
  char output[] = {" 000 "};
  if (rssi<0) {output[0]='-';rssi=-rssi;}
  output[1] = '0'+((rssi/100)%10);
  output[2] = '0'+((rssi/10)%10);
  output[3] = '0'+ (rssi%10);
  TXString(output, (sizeof output)-1);
}


// Timer B0 interrupt service routine

#pragma vector=TIMERB0_VECTOR
__interrupt void Timer_B (void)   
{
  secondsPartCounter++;
  if(secondsPartCounter >= 16/8)
  {
    //secCount=0;
  
//    secondsPartCounter = 0;
    
//    if(secondsPartCounter >= (SECOND_PART/3))//!!! Double speed?
    {
      secondsPartCounter = 0;
      secondsCounter++;

#if !SELF_CLUSTERING      
      secondsWithoutMaster++;
#else
      if(mainState == INITIALIZATION_CHECK)
        secondsWhileProbing++;
#endif
      
      
#if DEBUG
      snprintf(outputString, sizeof(outputString), "Sec:%i\n", secondsCounter);
      TXString(outputString, strlen(outputString));
#endif        
    }
     
    if(mainState == M_SEND_DATA)
    {
      if((secondsCounter == 2) && (secondsPartCounter == 0))
      {//Send Data from Master
        masterCanReadAndTransmitData = 1;
      }
    }   
    
    if(mainState == N_CAN_SEND)    
    {
      if((secondsCounter == currentNodeSlotNumber) && (secondsPartCounter == 0))
      {//Send Data from Node
        _flags |= (1<<NODE_CAN_READ_TRANSMIT_DATA);
      }      
    }

#if !SELF_CLUSTERING    
    if(currentNodeState == NODE_IS_NODE)
    {
      if(secondsWithoutMaster >= 3*T)
      {
        mainState = NULL_STATE;
        currentNodeState = NODE_IS_NONE;
      }
    }
#endif
  }
//TBCCR0 += 50000;
}

/*
#pragma vector=TIMERB0_VECTOR
__interrupt void Timer_B (void)   
{
  secCount++;
  if(secCount >= (160/SECOND_PART))
  {
    secCount=0;
  
    secondsPartCounter++;
    
    if(secondsPartCounter >= (SECOND_PART/3))//!!! Double speed?
    {
      secondsPartCounter = 0;
      secondsCounter++;
      secondsWithoutMaster++;
    
#if DEBUG
      snprintf(outputString, sizeof(outputString), "Sec:%i\n", secondsCounter);
      TXString(outputString, strlen(outputString));
#endif        
    }
     
    if(mainState == M_SEND_DATA)
    {
      if((secondsCounter == 2) && (secondsPartCounter == 0))
      {//Send Data from Master
        masterCanReadAndTransmitData = 1;
      }
    }   
    
    if(mainState == N_CAN_SEND)    
    {
      if((secondsCounter == currentNodeSlotNumber) && (secondsPartCounter == 0))
      {//Send Data from Node
        _flags |= (1<<NODE_CAN_READ_TRANSMIT_DATA);
      }      
    }
    
    if(currentNodeState == NODE_IS_NODE)
      if(secondsWithoutMaster >= 3*T)
      {
        mainState = NULL_STATE;
        currentNodeState = NODE_IS_NONE;
      }
  }
//TBCCR0 += 50000;
}
*/

void MRFI_RxCompleteISR()
{
  uint8_t i;
  mrfiPacket_t packet;

  if(!(_flags & (1<<NODE_INITIALIZATION_PACKET_ARRIVED)))
  {  
    if(currentNodeState == NODE_IS_NODE)
      toggleRedLed();
    else
      toggleGreenLed();
    
    MRFI_Receive(&packet);
    currentPacketSource = packet.frame[SOURCE_ADDR_INDEX];// = source;
    currentPacketDestination = packet.frame[DESTINATION_ADDR_INDEX];// = destination;  
    //for (i=9;i<29;i++)
    for (i=START_PAYLOAD_INDEX;packet.frame[i]!=0;i++)
    {
      if((i-8) <= MAX_BUFFER_LENGTH)
      {//Kontrolj, 4tobi massiv ne perepolnilsja
        buf[i-9]=packet.frame[i];
        buf[i-8]='\0';
      }
    }
  
#if DEBUG  
    TXString("Rx:", sizeof("Rx:"));
    TXString(buf, strlen(buf));
    sprintf(a2, "|%c", currentPacketSource+48);
    TXString(a2, strlen(a2));    
    TXString("\n", sizeof("\n")); 
    //rssi=MRFI_Rssi();
    //print_rssi(rssi);    
#endif  
  
    processReceivedBuffer();
  }
}

void setNextChanInLoop()
{
  MRFI_SetLogicalChannel(++logChan);  
  
  if(logChan >= MRFI_NUM_LOGICAL_CHANS-1)
    logChan = 0;
}

char temp[20];

 int main(void)
{
#if SELF_CLUSTERING  
   char o = -1;   
    
  P2DIR |= 0x08;//set Output pin 1
  P2DIR &= ~0x04;//set Input pin 0

  P2OUT |= 0x08;//set Output to 1
    
  P2REN |= 0x04;//enable pullup/pulldown
  P2OUT |= 0x04;//select pull-up 
  
  P2OUT &= ~0x08;//set Output to 0
  
  __delay_cycles (100);  
  
  if(P2IN & 0x04)
    o = 0;//Not a master
  else
    o = 1;//is a master
     
#endif
 
 /************Main State Machine*************/      
  
  while(1)
  {
// -----------------------------NONE MODE---------------------------------------
    if(mainState == NULL_STATE)
    {
      TBCCTL0 &= ~CCIE;                           // TBCCR0 interrupt disable :)
      initBoard();
      radioOff();
      initVars();      
      
#if DEBUG      
      TXString("NULL_STATE\n", sizeof("NULL_STATE\n"));
#endif
      
      mainState = MASTER_DETECTION;
      currentNodeState = NODE_IS_NONE;
      
//Vklju4aem radio, 4to bi prinimatj paketi.
//Vmeste s Taimerom, potomu 4to otdeljno vklju4enie radio ne rabotaet. :(      
      
     radioOn(); 
     initTimer(); 

#if SELF_CLUSTERING 

     if(o == 1)
     {
       mainState =  INITIALIZATION_CHECK;
//       mainState =  M_SEND_NET_INFO;
//       snprintf(networkBuf, sizeof(networkBuf), "%02i_%02i_%02i|", id, 1, 2);
     }
     else
     {
       mainState =  N_INIT_TO_NET;
     }
#endif 


     
#if RAPID_DEVELOPMENT
     
#if MASTER
  mainState =  M_SEND_NET_INFO;
#endif

#if MASTER        
  snprintf(networkBuf, sizeof(networkBuf), "%02i_%02i_%02i|", id, 1, 2);
#endif
  
#if NODE  
  mainState =  N_INIT_TO_NET;
#endif
  
#endif  

      oneTime1 = 1; oneTime = 1;
    }
    
#if SELF_CLUSTERING  
    
    if(mainState == INITIALIZATION_CHECK)
    {            
      if(oneTime)
      {
#if DEBUG        
        TXString("INITIALIZATION_CHECK\n", sizeof("INITIALIZATION_CHECK\n"));
#endif        
        oneTime = 0;
      }
      
      //startMasterProbing = 1;
            
      //while(!(_flags & (1<<NODE_CAN_READ_TRANSMIT_DATA))
      while(1)
      {
        if(_flags & (1<<MASTER_PACKET_ARRIVED))
        {
#if DEBUG        
          snprintf(outputString, sizeof(outputString), "LogChan:% is full\n", logChan);          
          TXString(outputString, strlen(outputString));
#endif           
          _flags &= ~(1<<MASTER_PACKET_ARRIVED);
          setNextChanInLoop();
          
          secondsWhileProbing = 0;
        }
        
        if(secondsWhileProbing >= 2*tT)
        {//channel is free.
#if DEBUG        
          snprintf(outputString, sizeof(outputString), "LogChan:% is free\n", logChan);          
          TXString(outputString, strlen(outputString));
#endif    
          mainState =  M_SEND_NET_INFO;                  
          snprintf(networkBuf, sizeof(networkBuf), "%02i_%02i_%02i|", id, 1, 2);

          secondsWhileProbing = 0;
          break;          
        }
      }
            
      oneTime1 = 1; oneTime = 1;
    }
    
#endif

//---------------------NODE STATES----------------------------------------------------------------    
    
    if(mainState == N_INIT_TO_NET)
    {
      if(oneTime)
      {
#if DEBUG        
        TXString("N_INIT_TO_NET\n", sizeof("N_INIT_TO_NET\n"));
#endif        
        oneTime = 0;
      }
      
      setResetGreenLed(SET_);
      currentNodeState = NODE_IS_NODE;

      if(!checkIfNodeIsInNet())
      {
        //Opredelim Kol-vo uzlov v seti
        currentFrameSlotCount = returnAmountOfNodesInNet()-1 + MASTER_SLOT_COUNT+INITIALIZATION_SLOT_COUNT;
        
        if((secondsCounter == currentFrameSlotCount) && 
           (initializationAbleNode == id) &&
           (secondsPartCounter == 0))
        {//Send Initialization request

          if(oneTime1)
          {
#if DEBUG        
            TXString("Init\n", sizeof("Init\n"));
#endif        
            oneTime1 = 0;
          } 
      
          snprintf(buf, (sizeof(buf)), "%i;0", NODE_INITIALIZATION_PACKET_TYPE);      
          transmitDataBuf(id, 0);

          mainState = N_IF_I_IN_NET;
          oneTime1 = 1; oneTime = 1;
        }
      }
      else
      {
        returnSlotNumberForNode();
        mainState = N_CAN_SEND;
        oneTime1 = 1; oneTime = 1;         
      }     
    }
    
    if(mainState == N_IF_I_IN_NET)
    {
      if(oneTime)
      {
#if DEBUG        
        TXString("N_IF_I_IN_NET\n", sizeof("N_IF_I_IN_NET\n"));
#endif        
        oneTime = 0;
      }       
      //Vsja proverka prohodit v funkcii processReceivedBuffer()
    }    
    
    if(mainState == N_CAN_SEND)
    {
      if(oneTime)
      {
#if DEBUG        
        TXString("N_CAN_SEND\n", sizeof("N_CAN_SEND\n"));
#endif        
        oneTime = 0;
      }   
      
      if((_flags & (1<<NODE_CAN_READ_TRANSMIT_DATA)) && 
         (_flags & (1<<MASTER_PACKET_ARRIVED)))
      {//Mo#no posilatj dannie, esli toljko nastupilo vremja 
       //i bil polu4en MS              

        _flags &= ~(1<<NODE_CAN_READ_TRANSMIT_DATA);
        _flags &= ~(1<<MASTER_PACKET_ARRIVED);        
        
        if(sensors & (1<<LUX_SENSOR))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);
            takeLuxes();
          }

          if(secondsCounter == currentNodeSlotNumber)
          {
            snprintf(buf, (sizeof(buf)), "%i;L=%i", DATA_PACKET_TYPE, (int)(lux*1000));
          }            
        }
        

        
        if(sensors & (1<<TEMP_SENSOR))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);
            takeTemperature();
          }
          
          if(secondsCounter == currentNodeSlotNumber)
          {          
            snprintf(buf, (sizeof(buf)), "%i;T=%i", DATA_PACKET_TYPE, (int)temperature);
          }
        }
        
        if(sensors & (1<<HUM_TEMP_SENSOR))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);         
            takeHumiAnfTemperature();
          }

          if(secondsCounter == currentNodeSlotNumber)
          {          
            snprintf(buf, (sizeof(buf)), "%i;H=%i", DATA_PACKET_TYPE, (int)hum);
          }
        }
        
        if(sensors & (1<<ACCELEROMETER))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);         
            takeAccelerometer();
          }

          if(secondsCounter == currentNodeSlotNumber)
          {          
            snprintf(buf, (sizeof(buf)), "%i;A=%ui", DATA_PACKET_TYPE, acc);
          }
        }        

        if(!sensors)
        {
          snprintf(buf, (sizeof(buf)), "%i;No sensors", DATA_PACKET_TYPE);          
        }

        if(secondsCounter == currentNodeSlotNumber)
        {              
          transmitDataBuf(id, 0);
#if DEBUG 
          TXString("Data:", strlen("Data:"));
          TXString(buf, strlen(buf));
          sprintf(a2, "|%c", id+48);
          TXString(a2, strlen(a2));        
          TXString("\n", strlen("\n"));      
#endif    
          _flags |= (1<<CAN_MAKE_NEW_CONVERSION);
        }      
      }            
    }    
//---------------------MASTER STATES----------------------------------------------------------------
    
    if(mainState == MASTER_DETECTION)
    {
      if(oneTime)
      {
#if DEBUG        
        TXString("MASTER_DETECTION\n", sizeof("MASTER_DETECTION\n"));
#endif        
        oneTime = 0;
      }
      
      if (secondsCounter >= 3*T)
      {//Mogu bitj masterom
        if(oneTime1)
        {        
#if DEBUG        
          TXString("Master is not detected\n", sizeof("Master is not detected\n"));
          oneTime1 = 0;
#endif    
          oneTime1 = 1; oneTime = 1;
          secondsCounter = 0;secondsPartCounter = 0;
          mainState = MASTER_ELLECTION;
        }        
      }
    }
    
    if(mainState == MASTER_ELLECTION)
    {
      if(oneTime)
      {
#if DEBUG        
        TXString("MASTER_ELLECTION\n", sizeof("MASTER_ELLECTION\n"));
        oneTime = 0;
#endif        
       
      }
      
      if (secondsCounter >= 2*T+id)
      {//Mogu bitj masterom
        if(firstCycleToSelectMaster)
        {//Esli eto pervij raz, kogda mogu bitj masterom, to es4o razok
          firstCycleToSelectMaster = 0;
          if(oneTime1)
          {        
#if DEBUG        
            TXString("Let's do it one time\n", sizeof("Let's do it one time\n"));
            oneTime1 = 0;
#endif    
          }
          
          secondsCounter = 0;secondsPartCounter = 0;
          oneTime1 = 1; oneTime = 1;
          mainState = MASTER_ELLECTION;
          continue;          
        }
        else
        {
          if(oneTime1)
          {        
#if DEBUG        
            TXString("I can be master\n", sizeof("I can be master\n"));
            oneTime1 = 0;
#endif    
            firstCycleToSelectMaster = 1;
            //Obnuljaem kol-vo uzlov
            //Stroim novij bufer seti (u4astnikov)
            nodeCount = 1;

#if !RAPID_DEVELOPMENT          
            snprintf(networkBuf, sizeof(networkBuf), "%02i_%02i_%02i|", id, 1, 2);          
#endif
            oneTime1 = 1; oneTime = 1;
            mainState = M_SEND_NET_INFO;
          }        
        }
      }
    }
    
    if(mainState == M_SEND_NET_INFO)
    {
      if(oneTime)
      {
        
#if DEBUG        
        //TXString("M_SEND_NET_INFO\n", sizeof("M_SEND_NET_INFO\n"));
#endif        
        oneTime = 0;
      }
      
      setResetRedLed(SET_);
      currentNodeState = NODE_IS_MASTER;
      
      nextNode++;         
      if(nextNode > maxNodes)
        nextNode = 1;            
      
      //Opredelim Kol-vo uzlov v seti
      currentFrameSlotCount = returnAmountOfNodesInNet()-1 + MASTER_SLOT_COUNT+INITIALIZATION_SLOT_COUNT;      
      
      snprintf(buf, (sizeof(buf)), "%i;%s;%02i", MASTER_SLOT_PACKET_TYPE, networkBuf, nextNode);
      transmitDataBuf(id, 0);

#if DEBUG      
      TXString(buf, strlen(buf));
      TXString("\n", sizeof("\n"));      
#endif
      
      oneTime1 = 1; oneTime = 1;
      secondsCounter = 1;secondsPartCounter = 0;
      mainState = M_SEND_DATA;
    }
    
    if(mainState == M_SEND_DATA)
    {
      if(oneTime)
      {
#if DEBUG        
        TXString("M_SEND_DATA\n", sizeof("M_SEND_DATA\n"));
#endif        
        oneTime = 0;                
      }
      
      if(masterCanReadAndTransmitData)//Ustanavlivaetsja v obrabot4ike prerivanija tajmera
      { 
#if DEBUG         
        //TXString("Starting data transmission!\n", strlen("Starting data transmission!\n"));
#endif        
        masterCanReadAndTransmitData = 0;
        
        if(sensors & (1<<LUX_SENSOR))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);          
            takeLuxes();
          }

          if(secondsCounter == 2)
          {          
            snprintf(buf, (sizeof(buf)), "%i;L=%i", DATA_PACKET_TYPE, (int)(lux*1000));
          }
        }
        
        
       
        
        if(sensors & (1<<TEMP_SENSOR))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);           
            takeTemperature();
          }
          
          if(secondsCounter == 2)
          {  
            snprintf(buf, (sizeof(buf)), "%i;T=%i", DATA_PACKET_TYPE, (int)temperature);
          }
        }

        if(sensors & (1<<HUM_TEMP_SENSOR))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);           
            takeHumiAnfTemperature();
          }

          if(secondsCounter == 2)
          {           
            snprintf(buf, (sizeof(buf)), "%i;H=%i", DATA_PACKET_TYPE, (int)hum);
          }
        }
        
        if(sensors & (1<<ACCELEROMETER))
        {
          if(_flags & (1<<CAN_MAKE_NEW_CONVERSION))
          {
            _flags &= ~(1<<CAN_MAKE_NEW_CONVERSION);         
            takeAccelerometer();
          }

          if(secondsCounter == 2)
          {          
            snprintf(buf, (sizeof(buf)), "%i;A=%i", DATA_PACKET_TYPE, acc);
          }
        }         
         
        if(!sensors)
        {
          snprintf(buf, (sizeof(buf)), "%i;No sensors", DATA_PACKET_TYPE);          
        }


        if(secondsCounter == 2)
        {                  
          transmitDataBuf(id, 0);
         
#if DEBUG 
          TXString("Data:", strlen("Data:"));
          TXString(buf, strlen(buf));
          sprintf(a2, "|%c", id+48);
          TXString(a2, strlen(a2));
          TXString("\n", strlen("\n"));      
#endif  
          _flags |= (1<<CAN_MAKE_NEW_CONVERSION);                  
        }
        mainState = M_WAIT;
        oneTime1 = 1; oneTime = 1;               
      }
    }
    
    if(mainState == M_WAIT)
    {
      if(oneTime)
      {
#if DEBUG        
        TXString("M_WAIT\n", sizeof("M_WAIT\n"));
#endif        
        oneTime = 0;                
      }
      
      
      if(secondsCounter == currentFrameSlotCount)
      {//Obrabotka zaprosa na inicializaciju
        
        if(_flags & (1<<NODE_INITIALIZATION_PACKET_ARRIVED))
        {          

#if DEBUG        
          TXString("NInit\n", sizeof("NInit\n"));
#endif            
          //char temp[20];
          snprintf(temp, sizeof(temp), "%02i_%02i|", currentPacketSource, 2+returnAmountOfNodesInNet());
          strncat(networkBuf, temp ,sizeof(networkBuf)); 
          temp[0] = 0;
          _flags &= ~(1<<NODE_INITIALIZATION_PACKET_ARRIVED);
        }
      }
          
      //Mo#no realizovatj obrabotku paketov ot drugih uzlov
      
      //if(secondsCounter >= T)
      if(secondsCounter >= currentFrameSlotCount+1)
      {//Na4atj frame zanovo
        if(oneTime1)
        {
#if DEBUG        
          //TXString("F\n", sizeof("F\n"));
#endif        
          oneTime1 = 0;                
        }
        
        mainState = M_SEND_NET_INFO;
        oneTime1 = 1; oneTime = 1;         
      }      
    }
  }
}             


//currentFrameSlotCount
//Nodes Function

void transmitDataBuf(char source, char destination)
{
  mrfiPacket_t packet;
 
  radioOn();
  
  packet.frame[SOURCE_ADDR_INDEX] = source;
  packet.frame[DESTINATION_ADDR_INDEX] = destination;
    
  for (char i=0; i<strlen(buf); i++)
  {
    packet.frame[START_PAYLOAD_INDEX+i] = buf[i];
    packet.frame[START_PAYLOAD_INDEX+i+1] = '\0';
  }
   
  packet.frame[0]=38;
  //P4OUT = 0x08;

  MRFI_Transmit(&packet, MRFI_TX_TYPE_FORCED);
  //P4OUT = 0x10;  
}


#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
  P1IFG &= ~0x04;
  //reset();
}

void initVars()
{  
/*********************/  
  _flags = 0;
  
  initializationSlotNumber = 0;
  initializationAbleNode = 0;

  currentNodeIsInNetwork = 0;
  currentNodeSlotNumber = 0;
  currentNodeState = NODE_IS_NONE;

  oneTime = 0; 
  oneTime1 = 0; 
  memset(nodeToRequest, 0, sizeof(nodeToRequest)); 
  nodeForRequest = 0;
  id = 0;

  secondsCounter=0;
  secondsPartCounter=0;
  
#if !SELF_CLUSTERING
  secondsWithoutMaster = 0;
#endif
  
  masterCanReadAndTransmitData = 0;

  transmissionIsInitialized = 0;

  memset(buf,0,sizeof(buf));
  memset(networkBuf,0,sizeof(networkBuf));
  currentPacketDestination = 0;
  currentPacketSource = 0;

  maxNodes = 0;

  nodeCount = 0; lastSlot = 0;
  nextNode = 0; clearTimer = 0;  
  
  firstCycleToSelectMaster = 1;

  currentFrameSlotCount = 0;

  memset(a2,0,sizeof(a2));
  memset(temp,0,sizeof(temp));
  
  /*************************************/
  
  /*********Initialization***************/  

#if MASTER
  id = 3;
#endif 
  
#if NODE
  #if (NODE == 1)  
    id = 2;
  #endif
  #if (NODE == 2)
    id = 1;
  #endif  
  #if (NODE == 3)
    id = 4;
  #endif     
  #if (NODE == 4)
    id = 5;
  #endif     
  #if (NODE == 5)
    id = 3;
  #endif   
  #if (NODE == 6)
    id = 6;
  #endif     
#endif    
  
  //maxNodes = 5;
  maxNodes = 4;
  
  nextNode = 0;
  nodeCount = 0;
  lastSlot = 0;
  //networkDevices[0] = '\0';    
  oneTime = 1;
  oneTime1 = 1;

  clearTimer = 0;  
   
  mainState = NULL_STATE;  
  
  T = maxNodes+(MASTER_SLOT_COUNT)+INITIALIZATION_SLOT_COUNT;
  
  logChan = 0;
}