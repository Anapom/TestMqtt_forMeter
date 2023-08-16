#include <Arduino.h>

#include <PubSubClient.h>
#include <WiFi.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

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


String      _mqtt_publish_topic    = "/MEA/Test/" + String(mqtt_clientID) +"/readings";
String      _mqtt_subscribe_topic  = "/MEA/Test/" + String(mqtt_clientID) +"/commands";
const char* mqtt_publish_topic     = _mqtt_publish_topic.c_str();
const char* mqtt_subscribe_topic   = _mqtt_subscribe_topic.c_str();

char bufferJsonPayload[256];  

//-------- Define function -------//
void setup_wifi();
void callback(char* topic, byte* message, unsigned int length); // Function callback incase of message arrive in subscribe
void reconnect();
void createJsonPayload (int v1, int v2, int v3);

//-------- Timing ----------------//
unsigned long startMillis     = 0;
const unsigned long wd_Period = 15000; // millisecond
const int wd_CounterLimit     = 240;       // How many time watchdog can feed untill reset
int wd_Counter                = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
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

    int _v1 = random(230, 240 + 1);
    int _v2 = random(230, 240 + 1);
    int _v3 = random(230, 240 + 1);
    // Create payload
    createJsonPayload(_v1, _v2, _v3);
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

void createJsonPayload (int v1, int v2, int v3){
  StaticJsonDocument<256> doc;
	// Add values in the document
  //------------------- Code here -------------
  JsonObject obj = doc.createNestedObject("payload");
	obj["id"] = mqtt_clientID;
  obj["voltage_L1_Ins"] = v1;
  obj["voltage_L2_Ins"] = v2;
  obj["voltage_L3_Ins"] = v3;
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
