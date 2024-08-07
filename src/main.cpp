#define NBREG   5    
#define LED_PIN 2

#include <Arduino.h>

#include <SDM.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include "soc/rtc_wdt.h"

//-------- WIFI setting ---------//
const char* ssid                  = ""; 
const char* password              = ""; 

//-------- MQTT setting ---------//
const char* mqttUser              = "";
const char* mqttPassword          = "";
const char* mqtt_server           = "";
const int   mqttPort              = 1883;
String      _mqtt_clientID        = ""; //must be unique per device (Same as deviceID)
const char* mqtt_clientID         = _mqtt_clientID.c_str();


String      _mqtt_publish_topic    = "/MEA/RDD/TLM/" + String(mqtt_clientID) +"/readings";
String      _mqtt_subscribe_topic  = "/MEA/RDD/TLM/" + String(mqtt_clientID) +"/commands";
const char* mqtt_publish_topic     = _mqtt_publish_topic.c_str();
const char* mqtt_subscribe_topic   = _mqtt_subscribe_topic.c_str();

//-------- Modbus SDM -------//
typedef volatile struct {
  volatile float regvalarr;
  const uint16_t regarr;
} sdm_struct;

volatile sdm_struct sdmarr[NBREG] = {
  {0.00, SDM_PHASE_1_VOLTAGE},                                                  
  {0.00, SDM_PHASE_1_CURRENT},                                                 
  {0.00, SDM_PHASE_1_POWER}, 
  {0.00, SDM_PHASE_1_REACTIVE_POWER},
  {0.00, SDM_IMPORT_ACTIVE_ENERGY}                                                                                                   
};

//-------- Json buffer -------//
char bufferJsonPayload[256];  

//-------- Define function -------//
void setup_wifi();
void callback(char* topic, byte* message, unsigned int length); // Function callback incase of message arrive in subscribe
void reconnect();
void createJsonPayload (float v1, float c1, float p1, float q1, float e1);
void readSDM120CT();
void sdmRead();

//-------- Timing ----------------//
unsigned long startMillis     = 0;
const unsigned long wd_Period = 60000; // millisecond
const int wd_CounterLimit     = 240;       // How many time watchdog can feed untill reset
int wd_Counter                = 0;

WiFiClient espClient;
PubSubClient client(espClient);
SDM sdm(Serial2, 2400, NOT_A_PIN, SERIAL_8N1, 16, 17);


void setup() {

  rtc_wdt_protect_off();
	rtc_wdt_enable();
	rtc_wdt_feed();
	rtc_wdt_set_time(RTC_WDT_STAGE0, 120000);

	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, 0);

  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial2.begin(2400);
  // node.begin(1, Serial2);
  sdm.begin();

  setup_wifi();
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

	startMillis = millis();

}

void loop() {
    if (!client.connected()) {
    	reconnect();
  	}
  client.loop();
  // put your main code here, to run repeatedly:
  if((millis() - startMillis >= wd_Period) && (wd_Counter < wd_CounterLimit)){
		startMillis = millis();
		wd_Counter++;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    rtc_wdt_feed();

    // Read SDM120CT
    sdmRead();
    
    // Create payload
    createJsonPayload(sdmarr[0].regvalarr, sdmarr[1].regvalarr, sdmarr[2].regvalarr, sdmarr[3].regvalarr, sdmarr[4].regvalarr);
    // Publish
    client.publish(mqtt_publish_topic, bufferJsonPayload);
	}
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void createJsonPayload (float v1, float c1, float p1, float q1, float e1){
  StaticJsonDocument<256> doc;
	// Add values in the document
  //------------------- Code here -------------
  JsonObject obj = doc.createNestedObject("payload");
	obj["id"] = mqtt_clientID;
  obj["voltage_L1_Ins"] = v1;
  obj["current_L1_Ins"] = c1;

  obj["activePower_L1_Ins"] = p1;
  obj["reactivePower_L1_Ins"] = q1;
  obj["activeEnergy_Imp"] = e1 * 1000;
  //--------------------------------------------

  //change json back to string
  String temp = doc["payload"].as<String>();
  // create {"payload": stringJson}
  StaticJsonDocument<256> doc2; 
  doc2["payload"] = temp;
  serializeJson(doc2, Serial);
  serializeJson(doc2, bufferJsonPayload);
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.println("");
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_clientID, mqttUser, mqttPassword )) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(mqtt_subscribe_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void readSDM120CT(){
  float voltage = sdm.readVal(SDM_PHASE_1_VOLTAGE);
  Serial.print("Voltage:   ");
  Serial.print(voltage);                         //display voltage
  Serial.println("V");
}

void sdmRead() {
  float tmpval = NAN;

  for (uint8_t i = 0; i < NBREG; i++) {
    tmpval = sdm.readVal(sdmarr[i].regarr);

    if (isnan(tmpval))
      sdmarr[i].regvalarr = 0.00;
    else
      sdmarr[i].regvalarr = tmpval;

    yield();
  }
}