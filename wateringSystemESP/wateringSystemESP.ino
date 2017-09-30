/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/
   
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DEBUG
#include <mydebug.h>

#include "secret.h" 
// include secret.h or define ssid password mqttserver mqttserverpassword
/*
const char* ssid = ""
const char* password = ""
const char* mqtt_server = ""
const char* pimqttpass = ""
*/


// Update these with values suitable for your network.

const char* statusTopic = "plantWaterSystem/status";
const char* readSensorsTopic = "plantWaterSystem/readSensors";
const char* pumpTopic = "plantWaterSystem/pump";
const char* onlineTopic = "plantWaterSystem/online";

char* inputString = "";         // a string to hold incoming data
int inputStringLength=0;
boolean stringComplete = false;  // whether the string is complete 

WiFiClient espClient;
PubSubClient client(espClient);
bool autoReadEnabled=true;
long autoReadFreq=5000; // ms
long futureMsgTimestamp=0;

void resetSerialInput()
{
  strcpy(inputString,"");
  stringComplete = false;
  inputStringLength = 0;
}

void handleReply()
{
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  char buffer[255];
  const char *pch;
  if(stringComplete)
  {
    debug("Received string:");
    debug(inputString);

    pch = strtok(inputString," ");
    root["TempStatus"]  = pch;

    const char* temperature = strtok(NULL," ");
    double temp = strtod(temperature,NULL);
    root["Temp"] =  temp;
    
    const char* humidity = strtok(NULL," ");
    double hum = strtod(humidity,NULL);
    root["Hum"] = hum;
    
    const char* waterlevel = strtok(NULL," ");
    int wlvl = atoi(waterlevel);
    root["WaterLvl"] = wlvl;
    
    const char* lightlevel = strtok(NULL," ");
    int llvl = atoi(lightlevel);
    root["Light"] = llvl;
    
    const char* soilhumidity = strtok(NULL," ");    
    double shum = strtod(soilhumidity,NULL);
    root["SoilHum"] = shum;
    
    const char* pumpstatus = strtok(NULL,"\r");
    root["PumpStatus"] = pumpstatus;
    
    root.printTo(buffer);    
    debug("Publishing");
    debug(statusTopic);
    debug(buffer);
    client.publish(statusTopic,buffer);
    resetSerialInput();
  }
}

void handleOnlineStatus()
{
  static long lastSent = 0;
  long now = millis();  
  if(now-lastSent > 5000)
 {
    debug("Published online status");
    client.publish(onlineTopic,"online");
    lastSent = now;
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
    if (inChar == '\n' || inChar == 0) {
      stringComplete = true;
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output  
  Serial.begin(115200);
  Serial1.begin(115200);
  setDebugPort(&Serial1);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  
  debug("Connecting to ");
  debug(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial1.print(".");
  }
  
  debug("WiFi connected");
  debug("IP address: ");
  char ipbuf[20];
  WiFi.localIP().toString().toCharArray(ipbuf,20);
  debug(ipbuf);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial1.print("Message arrived [");
  Serial1.print(topic);
  Serial1.println("] ");
  for (int i = 0; i < length; i++) {
    debug((char)payload[i]);
  }

  // Switch on the LED if an 1 was received as first character
  if(strcmp(topic,readSensorsTopic)==0)
  {
    debug("Sent 'r' to ARD");
    Serial.println("r");
  }
  if(strcmp(topic,pumpTopic)==0)
  {
    debug("Sent 'p' to ARD");
    Serial.print("p");
    Serial.println((char)payload[0]);
  }

}

void handleAutoRead()
{
  if(autoReadEnabled)
  {
    auto now=millis();
    if(now > futureMsgTimestamp)
    {
      futureMsgTimestamp = now+autoReadFreq;
      debug("Sent 'r' to ARD");
      Serial.println("r");
    }
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    debug("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("wateringSystemESP","pi",pimqttpass,onlineTopic,0,1,"offline")) {
      debug("connected");
      // Once connected, publish an announcement...
      client.publish(onlineTopic, "online",true);
      // ... and resubscribe
      client.subscribe(readSensorsTopic);
      client.subscribe(pumpTopic);
    } else {
      debug("failed, rc=");
      debug(client.state());
      debug(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  serialStringHandler();   
  handleOnlineStatus();
  handleReply();
  handleAutoRead();
}
