#include <ctype.h>
#include <dht.h>

//#define DEBUG
//#define DEBUG_MILLISECONDS
#include <mydebug.h>

dht DHT;

struct
{
    uint32_t total;
    uint32_t ok;
    uint32_t crc_error;
    uint32_t time_out;
    uint32_t connect;
    uint32_t ack_l;
    uint32_t ack_h;
    uint32_t unknown;
} stat = { 0,0,0,0,0,0,0,0};


#define SOIL_PIN A0
#define SOIL_EN 4
#define LDR_PIN A1 /* TODO */
#define WATER_LVL_PIN A2 /* TODO */
#define DHT22_PIN 6
#define PUMP_PIN  2

char tempStatus[20] = "";
char temperatureStr[10]="";
char humidityStr[10]="";

unsigned int now = 0;
unsigned int soilmV = 0; 
unsigned int LDRmV = 0; 
unsigned long waterLevelmV = 0; 
unsigned long pumpTimeout=0;
unsigned long messageFreq = 1000;
unsigned long messageTimeout = 0;

bool pumpActive=false;
char pumpStatus[10] = "false";
char inputString[10] = "";         // a string to hold incoming data

int inputStringLength=0;
boolean stringComplete = false;  // whether the string is complete 

struct fkt
{
  void (*fktPtr)();
  unsigned long ciclicity;
  unsigned long lastCalled;
};

void readSoil();
void readWaterLevel();
void readLDR();
void readTempAndHumidity();

fkt functions[] = 
{
  {readSoil,30000,0},
  {readWaterLevel,60000,0},
  {readLDR,1000,0},
  {readTempAndHumidity,10000,0},  
};

#define nr_of_fkt 4

void resetSerialInput()
{
  strcpy(inputString,"");
  stringComplete = false;
  inputStringLength = 0;
}

void readSoil()
{
  debug("Read Soil called");
  digitalWrite(SOIL_EN,HIGH);
  delay(100);
  int sensorValue = analogRead(SOIL_PIN);
  debug(sensorValue);
  digitalWrite(SOIL_EN,LOW);
  soilmV = 100-((unsigned long)sensorValue*100)/1024;
  debug(soilmV);
}

void readWaterLevel()
{
  debug("Read Water Level called");
  int sensorValue = analogRead(WATER_LVL_PIN);  
  waterLevelmV = ((unsigned long)sensorValue*100)/1024;  
}

void readLDR()
{
  debug("Read LDR called");
  int sensorValue = analogRead(LDR_PIN);    
  LDRmV = 100-((unsigned long)sensorValue*100)/1024;    
}

void sendDataToESP()
{
  debug("Send data to ESP called");
  Serial.print(tempStatus);
  Serial.print(" ");
  Serial.print(temperatureStr);
  Serial.print(" ");
  Serial.print(humidityStr);
  Serial.print(" ");
  Serial.print(waterLevelmV);
  Serial.print(" ");
  Serial.print(LDRmV);
  Serial.print(" ");
  Serial.print(soilmV);
  Serial.print(" ");
  Serial.println(pumpStatus);
}

void handlePump()
{
        if(pumpActive)
        {
          digitalWrite(PUMP_PIN,LOW);
          now = millis();
          if(now>pumpTimeout)
          {
            debug("Pump Stopped");
            pumpActive = false;
            digitalWrite(PUMP_PIN,HIGH);
            strcpy(pumpStatus,"false");
            sendDataToESP();         
          }
          if(now>messageTimeout)
          {
            readSoil();
            messageTimeout = now + messageFreq;
            sendDataToESP();
          }
        }
        else
        {
            digitalWrite(PUMP_PIN,HIGH);
        }
}

char ascii[32];
char* tempToAscii(double temp)
{
  int frac;
  frac=(unsigned int)(temp*10)%10;  //get three numbers to the right of the deciaml point
  itoa((int)temp,ascii,10);
  strcat(ascii,".");
  itoa(frac,&ascii[strlen(ascii)],10); //put the frac after the deciaml
  return ascii;
}

void readTempAndHumidity()
{
    debug("Read temp and humidity called");
    uint32_t start = micros();
    int chk = DHT.read22(DHT22_PIN);
    uint32_t stop = micros();

    stat.total++;
    switch (chk)
    {
    case DHTLIB_OK:
        stat.ok++;
        strcpy(tempStatus,"OK ");
        break;
    case DHTLIB_ERROR_CHECKSUM:
        stat.crc_error++;
        strcpy(tempStatus,"CHKERR ");
        break;
    case DHTLIB_ERROR_TIMEOUT:
        stat.time_out++;
        strcpy(tempStatus,"TOERR ");
        break;
    case DHTLIB_ERROR_CONNECT:
        stat.connect++;
        strcpy(tempStatus,"CONERR ");
        break;
    case DHTLIB_ERROR_ACK_L:
        stat.ack_l++;
        strcpy(tempStatus,"ACK1ERR ");
        break;
    case DHTLIB_ERROR_ACK_H:
        stat.ack_h++;
        strcpy(tempStatus,"ACK2ERR ");
        break;
    default:
        stat.unknown++;
        strcpy(tempStatus,"UNKERR ");
        break;
    }
    // DISPLAY DATA   
    strcpy(temperatureStr,tempToAscii(DHT.temperature));
    strcpy(humidityStr, tempToAscii(DHT.humidity));    
}

void handleMessages()
{
if (stringComplete) {
    if(inputString[0]=='p' && isdigit(inputString[1])) 
    {
      debug("Pump started");
      pumpActive = true;
      strcpy(pumpStatus,"true");
      pumpTimeout = now + (inputString[1]-'0') * 5000;
      messageTimeout = now + messageFreq;
      /*debug("PumpTimeout");
      debug(itoa(pumpTimeout));*/
    }
    if(inputString[0]=='r')
    {      
      if(!pumpActive)      
        sendDataToESP();
    }
    resetSerialInput();
   }
}

void handleReadings()
{
   now = millis();
   int i;
   for(i=0;i<nr_of_fkt;i++)
   {
    if(now - functions[i].lastCalled > functions[i].ciclicity)
    {
      functions[i].lastCalled=now;
      functions[i].fktPtr();
    }
   }
}


void serialStringHandler() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString[inputStringLength]= inChar;
    inputStringLength++;
    inputString[inputStringLength]= '\0';
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    
    if (inChar == '\n') {
      debug("String complete");
      debug(inputString);
      stringComplete = true;
    }
  }
}


void setup() {
  // put your setup code here, to run once:
  digitalWrite(PUMP_PIN,HIGH);
  pinMode(PUMP_PIN,OUTPUT);
  digitalWrite(SOIL_EN,LOW);
  pinMode(SOIL_EN,OUTPUT);
  

  readSoil();
  readWaterLevel();
  readLDR();
  readTempAndHumidity();
  
  Serial.begin(115200);  
  debug("Hello from Arduino UNO!");
}

void loop() {
  // put your main code here, to run repeatedly:
   now = millis();
   handlePump();
   handleMessages();
   handleReadings();   
   serialStringHandler();

}

