#include "stringFun.h"
#include <string.h>
#include "mrfi.h"
#include "txrx_simple.h"
#include <stdlib.h>
#include "board.h"

extern char buf[];
extern char networkBuf[];
extern volatile char mainState;
extern char oneTime;
extern char oneTime1;
extern unsigned char initializationAbleNode;
//extern unsigned char networkDevices [];
extern char id;
extern volatile unsigned int secondsCounter;
extern volatile unsigned char secondsPartCounter;
//extern volatile unsigned char secCount;

#if !SELF_CLUSTERING
extern volatile unsigned char secondsWithoutMaster;
#endif

extern volatile unsigned char _flags;

extern char currentNodeState;

extern unsigned char currentNodeSlotNumber;

void processReceivedBuffer(void)
{//Struktura paketa:
  
  if((buf[PACKET_TYPE_IDX]-48) == DATA_PACKET_TYPE)
  {//Standard (data) packet
    
    //TXString("D\r\n", sizeof("D\r\n"));
    //TXString(buf, strlen(buf));
    //TXString("\r\n", sizeof("\r\n"));     
  }
  
  if((buf[PACKET_TYPE_IDX]-48) == MASTER_SLOT_PACKET_TYPE)
  {//Master packet //Need to syncronize timers
    
    if(mainState == INITIALIZATION_CHECK)
    {
      _flags |= (1<<MASTER_PACKET_ARRIVED);      
    }
    else
    {
      if(currentNodeState == NODE_IS_MASTER)
      {
        mainState = NULL_STATE;
        currentNodeState = NODE_IS_NONE;
      }
      else
      {
        secondsPartCounter = 0;
       //secondsPartCounter = SECOND_PART/2;
        //secCount = 0;
        secondsCounter = 1;

#if !SELF_CLUSTERING    
        secondsWithoutMaster = 0;
#endif
      
        _flags |= (1<<MASTER_PACKET_ARRIVED);
       
        saveNetInfo();//!!!!Proveritj funkciju!
    
        if(!checkIfNodeIsInNet())
        {
          mainState = N_INIT_TO_NET;
        }
    
        if((mainState == MASTER_ELLECTION) || (mainState == MASTER_DETECTION))
        {//Esli vibiraem mastera, i sej4as polu4eno soobs4enie o mastere, to perejti v sostojanie NODE
          mainState = N_INIT_TO_NET;
          oneTime = 1; oneTime1 = 1;
        }
    
        if(mainState == N_IF_I_IN_NET)
        {//Proverka, estj li dannij uzel v seti...
          if(checkIfNodeIsInNet())
          {
            returnSlotNumberForNode();
            mainState = N_CAN_SEND;     
         }
           else
          {
            mainState = N_INIT_TO_NET;  
          }
      
          oneTime = 1; oneTime1 = 1; 
        }              
      }
    }
  }
  
  //if(buf[PACKET_TYPE_IDX]=='4')
  if((buf[PACKET_TYPE_IDX]-48) == MASTER_ELECTION_PACKET_TYPE)
  {//Master Request packet
    
    saveNetInfo();//!!!!Proveritj funkciju!
    
    if((mainState == MASTER_ELLECTION) || (mainState == MASTER_DETECTION))
      mainState = N_INIT_TO_NET;        
  }
  
  
  if((buf[PACKET_TYPE_IDX]-48) == NODE_INITIALIZATION_PACKET_TYPE)
  {//Node initialization request
    if(currentNodeState == NODE_IS_MASTER)    
      _flags |= (1<<NODE_INITIALIZATION_PACKET_ARRIVED);         
  }  
}

void saveNetInfo()
{//Packet sample:1;03_01_02|;01
  int i=0;
  int ii=0;
  char infoType = 0;//;
  char initNodeNum[3];
  initNodeNum[0] = 0;
  
  while(buf[i])
  {
    if(buf[i] == ';')
    {
      infoType ++;
      i++;
      ii = 0;
      continue;
    }
    
    if(infoType == 1)
    {//Network information (nodes with slots)
      networkBuf[ii] = buf[i];
      ii++;
      networkBuf[ii] = '\0';            
    }
    
    if(infoType == 2)
    {//Initialization able node's ID
      if(ii < sizeof(initNodeNum)-1)
      {  
        initNodeNum[ii] = buf[i];
        ii++;
        initNodeNum[ii] = '\0';
      }
    }    
    
    i++;
  }
  
  initializationAbleNode = atoi(initNodeNum);
  
  //determineInitSlotNodeSlot(); //??? - Za4em eta funkcija?
}

/*
void determineInitSlotNodeSlot(void)
{//Packet sample:1;03_01_02|;01
  
  //TODO: This code!
  
}
*/

void returnSlotNumberForNode ()
{
  //buf:1;03_01_02|02_03|;0
  //net:03_01_02|02_03|
  
  char currentSlot[3] = {"0"};
  char cc=0, firstEntryNow = 1, stop = 0;
    
  for(int i=0; i<strlen((char*)networkBuf); i++)
  {
    if(firstEntryNow)
    {//First entry is for Master only!
      if(networkBuf[i] == '|')
      {
        firstEntryNow = 0;
        i--;
      }
    }
    else
    {
      if(networkBuf[i] == '|')
      {
        stop = 0;
        
        if(atoi(currentSlot) == id)
        {//Found current node's id
          //TODO:Multy slot support add here
          currentSlot[0] = networkBuf[i-2];
          currentSlot[1] = networkBuf[i-1];          
          currentSlot[2] = 0;
          
          currentNodeSlotNumber = atoi(currentSlot);
          
          break;
        }
        currentSlot[0] = 0;
        cc = 0;
      }
      else
      {
        if(networkBuf[i] == '_')
          stop = 1;
        
        if(!stop)
        {
          currentSlot[cc] = networkBuf[i];
          cc++;
          currentSlot[cc] = 0;
        }
      }
    }
  } 
}

unsigned char i_=0;
char nodesId[3];
char breakWas = 0, firstUsage = 1;
  
char checkIfNodeIsInNet()
{//Estj li NODE s ID=id v opisanii seti
 // 1 - Estj takoj; 0 - net takogo
  //unsigned char i=0;
  //char nodesId[3];
  //char breakWas = 0, firstUsage = 1;

  i_=0;
  breakWas = 0;
  firstUsage = 1;

  while(networkBuf[i_])
  {
    if((networkBuf[i_] == '|') || (firstUsage))
    {
      if(!firstUsage)
        i_++;

      firstUsage = 0;
        
      if((networkBuf[i_]))
      {
        nodesId[0] = networkBuf[i_];
        nodesId[1] = networkBuf[i_+1];  
        nodesId[2] = '\0';
        if(atoi(nodesId) == id)
        {
          breakWas = 1;
          break;
        }
      }
      else
      {
        breakWas = 0;
        break;
      }
    }

    i_++;
  }
  
  if(breakWas)
    return 1;
  else
    return 0;  
}

unsigned char num = 0;

unsigned char returnAmountOfNodesInNet()
{
  //int i=0;
  //unsigned char num = 0;
  
  while(networkBuf[i_])
  {
    if(networkBuf[i_] == '|')
      num++;
    i_++;
  }
 
  return num;
}