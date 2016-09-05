/*
* OpenBikeLocker is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation, either version 3 of
* the License, or(at your option) any later version.
*
* OpenBikeLocker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with OpenBikeLocker.  If not, see
* <http://www.gnu.org/licenses/>.
*/

/* Connections
*  Autonomo D0 <=> MFRC522 RESET
*  Autonomo D5 <=> MFRC522 MISO
*  Autonomo D6 <=> MFRC522 SS
*  Autonomo D7 <=> MFRC522 MOSI
*  Autonomo D8 <=> MFRC522 CLK
*
*  Autonomo D1 <=> RED LED
*  Autonomo D2 <=> GREEN LED
*  
*  Autonomo D3 <=> SWITCH
*  
*  Autonomo D4 <=> SERVO
*  *
*  As the MFRC library makes hardcoded use of SPI the autonomo SPI port is remapped to sercom4 on autonomo  
*  Defines in variant.h should be as follows
*  #define PERIPH_SPI           sercom4 
*  #define PIN_SPI_MISO         (53u)
*  #define PIN_SPI_SS           (54u)
*  #define PIN_SPI_MOSI         (55u)
*  #define PIN_SPI_SCK          (56u)
*/

#include <Sodaq_RN2483.h>
#include <SPI.h>
#include <MFRC522.h>

// defines for LoraBee/Autonomo
#define debugSerial SerialUSB
#define loraSerial Serial1

// defines for MFRC lib
#define Serial SerialUSB
#define SS_PIN PIN_SPI_SS
#define RST_PIN 0
MFRC522 mfrc522(SS_PIN, RST_PIN);  

// defines for status leds/switches
#define LED_RED 1
#define LED_GREEN 2
#define SWITCH1_PIN 3

// define for servo motor (A0)
#define SERVO_PIN 4
#define RELAY_PIN 12


/* defines for low level lock API (in uplink direction) 
*  
* CMD_GET_SERVER_TIME get current time from server
* command = 1 byte
* response from server: 6 bytes with datetime structure (to be defined)
* 
* CMD_INVOKE_DOWNLINK see if server has new data for us
* command = 1 byte
* response from server: 1 byte with command, x bytes with command parameters (eg. open lock in 1 minute, to be defined)   
* 
* CMD_SET_LOCKER_STATE sets state of the locker
* command = 1 byte
* param1 1 byte (required) state => maps 1:1 to veiligstallen; 0 = vrij, 1 = bezet, 2 = geblokkeerd, 3 = gereserveerd, 4 =  buiten werking 
* param2 10 bytes (optional) mifareid
* 
* CMD_UPDATE_LOCKER_USER_ID sets the mifareid of the locker that is currently used (eg. anonymity expires after 24h)
* command = 1 byte
* param1 10 bytes (required) mifareid
*/
#define CMD_GET_SERVER_TIME 1
#define CMD_INVOKE_DOWNLINK 2
#define CMD_SET_LOCKER_STATE 3
#define CMD_REQUEST_LOCKER_UNBLOCK 4

//Function for simulation of lock (servo based, can alco be a relay)
void openLock()
{
  unsigned long timeout=millis();
    // poor man's PWM, SG90 servo needs about 50Hz pwm
    pinMode(SERVO_PIN, OUTPUT);     
    digitalWrite(VCC_SW, HIGH);     
 
  digitalWrite(RELAY_PIN, HIGH);
    
    for (uint16_t zz=0;zz<20;zz++)
    {  
        digitalWrite(SERVO_PIN,HIGH);
        delay(1);
        digitalWrite(SERVO_PIN,LOW);
        delay(19);
    }

    for (uint16_t zz=0;zz<20;zz++)
    {  
        digitalWrite(SERVO_PIN,HIGH);
        delay(2);
        digitalWrite(SERVO_PIN,LOW);
        delay(18);
    }

    digitalWrite(VCC_SW, LOW);     
    pinMode(SERVO_PIN, INPUT);     

  while((millis()-timeout)<5000)
    delay(100);
 
  digitalWrite(RELAY_PIN, LOW);
}

// Functions, variables for led control
uint8_t ledStates[3];

void ledSet(uint8_t ledId, uint8_t state, uint8_t updateState=1)
{
  digitalWrite(ledId, state);
  if (updateState)
  {
    switch(ledId)
    {
      case LED_BUILTIN:
        ledStates[0]=state;
        break;
      case LED_RED:
        ledStates[1]=state;
        break;
      case LED_GREEN:
        ledStates[2]=state;
        break;
    }
  }
}

void ledReset(uint8_t ledId)
{
  switch(ledId)
  {
    case LED_BUILTIN:
      digitalWrite(ledId,ledStates[0]);
      break;
    case LED_RED:
      digitalWrite(ledId,ledStates[1]);
      break;
    case LED_GREEN:
      digitalWrite(ledId,ledStates[2]);
      break;
  }
}

void ledBlink(uint8_t ledId, uint8_t blinks, uint8_t repeat=1)
{
    ledSet(ledId,0,0);
    for (uint8_t r=0;r<repeat;r++)
    {
      if (r)
        delay(600);
      for (uint8_t b=0;b<blinks;b++)
      {
           ledSet(ledId,1,0);
           delay(200);
           ledSet(ledId,0,0);
           delay(200);
      }
    }
    ledReset(ledId);
}

// Functions for switch readout  
uint8_t readSwitch(uint8_t switchId)
{
  // check for pressed switch (debounce for 20ms, active low)
  for (int zz=0;zz<10;zz++)
  {  
      if (digitalRead(switchId))
        return 0;
      delay(2);
  }
  return 1; 
}

// Functions, variables for keeping/setting the state of the locker 
uint8_t lockerInUse=0;
unsigned long lockerInUseTime=0;
const uint8_t uidLength=4;
uint8_t storedUid[uidLength];

void uidCopy(uint8_t *buf, uint8_t offset, uint8_t *uid)
{
  for (int zz=0;zz<uidLength;zz++)
    buf[zz+offset]=uid[zz];
}

void uidStore(uint8_t *uid)
{
  for (int zz=0;zz<uidLength;zz++)
    storedUid[zz]=uid[zz];
}

uint8_t uidCompare(uint8_t *uid)
{
  for (int zz=0;zz<uidLength;zz++)
    if (storedUid[zz]!=uid[zz])
      return 0;
  return 1;
}

void uidShow(uint8_t *uid)
{
  for (int zz=0;zz<uidLength;zz++)
  { 
    if (zz)
      debugSerial.print('.');
    if (uid[zz]<16)
      debugSerial.print('0');
    debugSerial.print(uid[zz],HEX);
  }
}

// Functions, variables for thingsnetwork (ABP assumed)
const byte devAddr[4] = { }; 
const byte nwkSKey[16] = { }; 
const byte appSKey[16] = { }; 

uint8_t sendPayload[32];
uint8_t receivePayload[32];
uint16_t receivedBytes;
uint8_t sendRetries=0;

void rawRN2483Cmd(const uint8_t* command)
{
  uint8_t t=0;
  debugSerial.print("CMD : ");
  while (command[t])
  {
    debugSerial.write(command[t]);
    loraSerial.write(command[t++]);
  }
  
  unsigned long time=millis();;

  while(((millis()-time)<1000)&&(!loraSerial.available()))
    delay(10);

  if (loraSerial.available())
  {
    debugSerial.print("RSP : ");
    while (loraSerial.available()) 
      debugSerial.write(loraSerial.read());
  }
  else
    debugSerial.println("No response!");
}

void clearReceivePayload(void)
{
  for (uint8_t zz;zz<32;zz++)
    receivePayload[zz]=0;
}

uint8_t ttnSend(const uint8_t* payload, uint8_t size, uint8_t reqAck=0, uint8_t retries=0)   
{  
  uint8_t resp, ret=0;

/*  rawRN2483Cmd((uint8_t*)"radio get freq\r\n");
  rawRN2483Cmd((uint8_t*)"radio get sync\r\n");
  rawRN2483Cmd((uint8_t*)"radio get cr\r\n");
  rawRN2483Cmd((uint8_t*)"radio get sf\r\n");
  rawRN2483Cmd((uint8_t*)"radio get crc\r\n");
  rawRN2483Cmd((uint8_t*)"radio get prlen\r\n");
*/
  if (reqAck)
    resp=LoRaBee.sendReqAck(1,payload,size,retries);
  else
    resp=LoRaBee.send(1,payload,size);  

  switch (resp)
  {
    case NotConnected:
      debugSerial.println("No network connection. Halting!");
      while(1) 
        ledBlink(LED_BUILTIN,3,3);
      break;
    case PayloadSizeError:
      debugSerial.println("Maximum payload size exceeded. Halting!");
      while(1)
        ledBlink(LED_BUILTIN,4,3);
      break;
    case InternalError:
      debugSerial.println("Internal error. Halting!");
      while (1) 
        ledBlink(LED_BUILTIN,5,3);
      break;
    case NetworkFatalError:
      debugSerial.println("Fatal network error. Halting!");
      while (1) 
        ledBlink(LED_BUILTIN,6,3);
      break;
    case NoAcknowledgment:
      debugSerial.println("No acknowledge received!");
      ledBlink(LED_RED,3,3);
      break;
    case Busy:
      debugSerial.println("RN2483 busy, sleep 5 seconds.");
      ledBlink(LED_RED,4,3);
      delay(3000);
      break;
    case NoResponse:
      debugSerial.println("No response from RN2483.");
      ledBlink(LED_RED,5,3);
      break;
    case Timeout:
      debugSerial.println("Connection timeout.");
      ledBlink(LED_RED,6,3);
      break;
    case NoError:
      debugSerial.println("Packet transmission OK.");
      ret=1;
      break;
    default:
      debugSerial.println("Unknow result after transmission. Halting!");
      while (1) 
        ledBlink(LED_BUILTIN,7,3);
      break;
  }
  return ret;
}  

void setup()
{
  // initialise debug serial
  while ((!debugSerial) && (millis() < 10000));
  debugSerial.begin(57600);

  // initialise servo
  pinMode(VCC_SW, OUTPUT);     
  digitalWrite(VCC_SW, LOW);     

  // initialise relay
  pinMode(RELAY_PIN, OUTPUT);
  
  // initialise status leds
  pinMode(LED_BUILTIN, OUTPUT);     
  pinMode(LED_RED, OUTPUT);     
  pinMode(LED_GREEN, OUTPUT);     
  
  ledSet(LED_BUILTIN,0);
  ledSet(LED_RED,0);
  ledSet(LED_GREEN,1);

  // initialise switch
  pinMode(SWITCH1_PIN, INPUT);     
  
  // initialise LoraBee
  pinMode(BEE_VCC, OUTPUT);     
  digitalWrite(BEE_VCC, HIGH);

  loraSerial.begin(LoRaBee.getDefaultBaudRate());
  LoRaBee.setDiag(debugSerial); 
  if (LoRaBee.initABP(loraSerial, devAddr, appSKey, nwkSKey, false))
  {
    debugSerial.println("Connected to network.");
    ledSet(LED_BUILTIN,1);
  }
  else
  {  
    debugSerial.println("Connection to network failed. Halting!");
    while (1)
      ledBlink(LED_BUILTIN,2,3);
  }

  // initialise card reader
  SPI.begin();      
  mfrc522.PCD_Init(); 
}

void loop()
{
  if (readSwitch(SWITCH1_PIN)) // if pressed we need to communicate with server 
  {
    debugSerial.println("Switch pressed, invoking downlink");
    sendPayload[0]=CMD_INVOKE_DOWNLINK;
    if (ttnSend(sendPayload,1,1))
    {
      debugSerial.println("Acknowledge packet received!");
      
      clearReceivePayload();
      receivedBytes=LoRaBee.receive(receivePayload,32);
      if(receivedBytes)
      {  
      	debugSerial.print("I received ");
      	debugSerial.print(receivedBytes);
      	debugSerial.print(" byte(s) : ");
      	for (uint8_t zz=0;zz<receivedBytes;zz++)
          debugSerial.println(receivePayload[zz],HEX);
      	
      	if ((receivePayload[0]==1)&&(receivePayload[1]==0xff))
      	{
      	  debugSerial.println("Open lock command received from downlink, locker is no longer in use!");
          ledSet(LED_RED,0);
          ledSet(LED_GREEN,1);

          lockerInUse=0;
          lockerInUseTime=0;
          openLock();

          sendPayload[0]=CMD_SET_LOCKER_STATE;
          sendPayload[1]=0; // vrij
          if (ttnSend(sendPayload,2,1))
            ledBlink(LED_GREEN,3);
      	}
        else
          ledBlink(LED_GREEN,1);
      }
      else    
      	debugSerial.println("No return data received!");
    }
  }
 
  if (mfrc522.PICC_IsNewCardPresent()) 
  {
    if (!mfrc522.PICC_ReadCardSerial()) 
    {
      debugSerial.println("Could not read info of detected card!");
      ledBlink(LED_RED,5);
    }
    else
    {  
      debugSerial.print("Card detected, uid = ");
      uidShow((uint8_t *)mfrc522.uid.uidByte);
      debugSerial.println("");  
      //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

      switch(lockerInUse)
      {
        case 0: // locker vrij
        debugSerial.println("Locker is now in use!");
        uidStore((uint8_t *)mfrc522.uid.uidByte);
        ledSet(LED_RED,1);
        ledSet(LED_GREEN,0);

        lockerInUse=1;
          lockerInUseTime=millis();
        openLock();

        sendPayload[0]=CMD_SET_LOCKER_STATE;
        sendPayload[1]=1; // bezet
          //uidCopy(sendPayload,2,(uint8_t *)mfrc522.uid.uidByte);
          ttnSend(sendPayload,2,1);
  
          delay(2000); // guard time
          break;
        case 1: // locker bezet
          if (uidCompare((uint8_t *)mfrc522.uid.uidByte))
          {
            debugSerial.println("Locker is no longer in use!");

            ledSet(LED_RED,0);
            ledSet(LED_GREEN,1);
  
            lockerInUse=0;
            lockerInUseTime=0;
            openLock();
  
            sendPayload[0]=CMD_SET_LOCKER_STATE;
            sendPayload[1]=0; // vrij
            //uidCopy(sendPayload,2,(uint8_t *)mfrc522.uid.uidByte);
            ttnSend(sendPayload,2,1);

        delay(2000); // guard time
      }
      else
      {
            debugSerial.print("Safe is in use by another card (");
            uidShow((uint8_t *)storedUid);
            debugSerial.println(")!");
            
            ledBlink(LED_RED,5);
          }
          break;
        case 2: // locker geblokkeerd
        if (uidCompare((uint8_t *)mfrc522.uid.uidByte))
        {
            debugSerial.println("Locker is blocked, requesting open lock from server!");
            sendPayload[0]=CMD_REQUEST_LOCKER_UNBLOCK;
            uidCopy(sendPayload,1,(uint8_t *)mfrc522.uid.uidByte);
            if (ttnSend(sendPayload,1+uidLength,1))
            {
              debugSerial.println("Acknowledge packet received!");
      
              clearReceivePayload();
              receivedBytes=LoRaBee.receive(receivePayload,32);
              if(receivedBytes)
              {  
                debugSerial.print("I received ");
                debugSerial.print(receivedBytes);
                debugSerial.print(" byte(s) : ");
                for (uint8_t zz=0;zz<receivedBytes;zz++)
                  debugSerial.println(receivePayload[zz]);
                
                if ((receivePayload[0]==1)&&(receivePayload[1]==0xff))
                {
                   debugSerial.println("Unblock clearance received from downlink, locker is no longer in use!");
  
          ledSet(LED_RED,0);
          ledSet(LED_GREEN,1);

          lockerInUse=0;
                    lockerInUseTime=0;
          openLock();

                    lockerInUse=0;
                    lockerInUseTime=0;
          sendPayload[0]=CMD_SET_LOCKER_STATE;
          sendPayload[1]=0; // vrij
                    if (ttnSend(sendPayload,2,1))
                      ledBlink(LED_GREEN,3);
                  }
                  else
                    ledBlink(LED_GREEN,5);
                }
                else    
                  debugSerial.println("No return data received!");
              }
        }
        else
        {
          debugSerial.print("Safe is in use by another card (");
          uidShow((uint8_t *)storedUid);
          debugSerial.println(")!");
            
          ledBlink(LED_RED,5);
        }
            break;
          default:
            // do nothing
            break;
      }
    }
    delay(500);
  }
  
  if (lockerInUse==1)
  {
    if ((millis()-lockerInUseTime)>120000) // block the safe after 2 minutes (24 hours)
    {
      debugSerial.println("Locker is now blocked!");
      lockerInUse=2;
      lockerInUseTime=0;
      
      ledSet(LED_RED,1);
      ledSet(LED_GREEN,1);

      sendPayload[0]=CMD_SET_LOCKER_STATE;
      sendPayload[1]=2; // geblokkeerd
      uidCopy(sendPayload,2,(uint8_t *)mfrc522.uid.uidByte);
      if (ttnSend(sendPayload,2+uidLength,1))
  ledBlink(LED_GREEN,3);

    }
  }   
}

