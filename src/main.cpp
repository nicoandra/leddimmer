#include "Arduino.h"
#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

char mqtt_server[40] = "192.168.1.106";
char mqtt_port[6] = "1883";
char device_name[32] = "ESP Dimmer";


bool shouldSaveConfig = false;  //flag for saving data

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
WiFiManagerParameter custom_device_name("device_name", "device name", device_name, 32);

long lastMsg = 0;
char msg[2048];
int value = 0;

DynamicJsonBuffer jsonBufferPub;
DynamicJsonBuffer jsonBufferSub;

void resetOnDemand(){

  if(!digitalRead(D0)){
    return ;
  }

  Serial.println("Resetting everything");
  wifiManager.resetSettings();
  SPIFFS.format();
  Serial.println("Reboot in 2 seconds");
  delay(2000);
  ESP.restart();
}

bool doReadConfig(){
  /* if (!SPIFFS.begin()) {
    Serial.println("failed to mount FS in doReadConfig");
    return false;
  }*/


  if (!SPIFFS.exists("/config.json")) {
    // If Config file does not exist Force the Config page
    Serial.println("doReadConfig: config file does not exist");
    return false;
  }

  Serial.println("Reading config file");
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to load json config file");
    // Force the Config page
    return false;
  }

  Serial.println("Opened config file");
  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  json.printTo(Serial);
  Serial.println(" ...");
  if (!json.success()) {
    Serial.println("Failed to parse json config file");
    // Force the Config page
    return false;
  }

  strcpy(mqtt_server, json["mqtt_server"]);
  strcpy(mqtt_port, json["mqtt_port"]);
  strcpy(device_name, json["device_name"]);
  Serial.println("Json parsed and configuration loaded from file");

}

void doSaveConfig(){
  //save the custom parameters to FS
  if (shouldSaveConfig) {

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(device_name, custom_device_name.getValue());

    Serial.println("Saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["device_name"] = device_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
      return ;
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
    Serial.println("Config Saved! Wait 5 seconds");
    delay(5000);
    ESP.restart();
  }
}


//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
  doSaveConfig();
}


void mqttMessageCallback(char* topic, byte* payload, unsigned int length) {
  JsonObject& jsonPayload = jsonBufferSub.parseObject(payload);

  jsonPayload.printTo(Serial);

  for(int i = 1; i < 8; i++){
    char lightCharArray[15];
    sprintf(lightCharArray, "light%01d", i);

    JsonVariant lightName = jsonPayload[lightCharArray];
    if(!lightName.success()){
      Serial.print("No data for light ");
      Serial.println(i);
      continue;
    }

    Serial.print(i);
    Serial.print(" : ");
    String value = lightName.as<char*>();
    Serial.println(value);
  }


  return ;

  String value = jsonPayload[String("hola")];
  Serial.println(value);
  jsonPayload.printTo(Serial);

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("Received "  + String((char)payload[0]));

  switch((char)payload[0]){
    case '0':
      digitalWrite(D2, LOW);   // Turn the LED on (Note that LOW is the voltage level
      return;
      break;
    case '1':
      digitalWrite(D2, LOW);   // Turn the LED on (Note that LOW is the voltage level
      return;
      break;
    case 'X':
        int power = (int)payload[1] * 1000 + (int)payload[2] * 100 + (int)payload[3] * 10 + (int)payload[4] - (1111 * 48);   // Turn the LED on (Note that LOW is the voltage level
        analogWrite(D2, power);   // Turn the LED on (Note that LOW is the voltage level
        char buf[2];
        sprintf(buf, "%d", power);
        Serial.println("Setting power to " + String(buf));
        client.publish("outTopic", buf, 2);
        return;
        break;
  }



  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {

    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(D2, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("Serial UP!");

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount FS in Setup");
    delay(10000);
    ESP.restart();
    return ;
  }

  Serial.println("Mounted file system.");

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_device_name);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setAPCallback(configModeCallback);

  pinMode(D2, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(D2, LOW);
  pinMode(D0, INPUT);
  digitalWrite(D0, LOW);

  resetOnDemand();

  while(!wifiManager.autoConnect("AutoConnectAP", "password")){
    Serial.println("Can not connect to WiFi.");
    delay(2000);
    // return ;
  }

  Serial.println("Wifi up. Try to load device settings from JSON");

  if(!doReadConfig()){
    Serial.println("Either can not read config, or can not connect. Loading portal!");
    wifiManager.startConfigPortal("OnDemandAP");
    Serial.println("Portal loaded");
    return;
    // If config can not be read, resetOnDemand!
    // SPIFFS.format();
    // wifiManager.resetSettings();

    // wifiManager.resetSettings();

  }


  Serial.println("");
  Serial.println(WiFi.macAddress());
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttMessageCallback);
}

void reconnect() {
  // Loop until we're reconnected
  String topic = "/lights/" + WiFi.macAddress();
  char topicBuffer[255];
  topic.toCharArray(topicBuffer, 255);

  int tryes = 0;
  while (!client.connected() && tryes < 10) {
    Serial.print("Attempting MQTT connection...");
    tryes++;
    // Attempt to connect
    if (client.connect("RandomName")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
      client.subscribe(topicBuffer);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try #" + String(tryes) + " again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1500);
    }
  }
}

void announce(){
  char mac_address[20];
  char message[2048];
  DynamicJsonBuffer jsonBufferPub;

  JsonObject& json = jsonBufferPub.createObject();
  Serial.println(WiFi.macAddress());
  WiFi.macAddress().toCharArray(mac_address, 20);
  json["mac_address"] = mac_address;
  json["device_name"] = device_name;
  json.printTo(message);
  // snprintf (msg, 75, macAddress);
  Serial.print("Publish message: ");
  // msg = msg + device_name;
  Serial.println(message);
  // sprintf(msg, "{'device_name' : '%s', mac_address: '%s'}", device_name, msg);
  client.publish("/device/announcement", message);


}

void loop() {
  resetOnDemand();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    // ++value;
    /* snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);*/
    announce();
  }

}
