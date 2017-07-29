#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <led.h>


slacker::Led channels[3] = {
  slacker::Led(D4),
  slacker::Led(D1),
  slacker::Led(D3)
};

unsigned int localPort = 8888;
WiFiUDP UDP;
boolean udpConnected = false;
byte mac[6];                     // the MAC address of your Wifi shield
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
// char ReplyBuffer[] = "Acknowledged";

boolean connectUDP(){
  boolean state = false;
  Serial.println("");
  Serial.println("Connecting to UDP");

  if(UDP.begin(localPort) == 1){
    Serial.println("Connection successful");
    state = true;
  } else {
    Serial.println("Connection failed");
  }

  return state;
}


void setChannel(int id, int value){
  Serial.println(String("Led") + String(id) + " Power:" + String(value));
  //channels[id-1].On();
  //channels[id-1].SetBrightness(value);

}


void listenUdp(){
  // if there’s data available, read a packet
  int packetSize = UDP.parsePacket();

  if(packetSize == 0){
    Serial.print(".");
    delay(20);
    return ;
  }

  if(packetSize != 2){
    Serial.print("x");
    delay(20);
    return ;
  }

  Serial.print(packetSize);
  UDP.read(packetBuffer,packetSize);

  IPAddress remote = UDP.remoteIP();
  for (int i =0; i < 4; i++){
    Serial.print(remote[i], DEC);
    if (i < 3) {
      Serial.print(".");
    }
  }

  int ledNumber = packetBuffer[0];
  if(ledNumber < 0xF0){
    Serial.println("Reject packet.");
    return ;
  }

  ledNumber = ledNumber - 0xF0;
  int ledPower = packetBuffer[1];

  setChannel(ledNumber, ledPower);
  Serial.print(ledNumber);
  Serial.println(ledPower);
  return ;
} // End of void listenUdp();

void setup(){
  Serial.begin(115200);
  Serial.println("Booted 1");

  // Network settings
  WiFiManager wifiManager;
  // wifiManager.autoConnect("AP-NAME", "AP-PASSWORD");
  wifiManager.autoConnect();
  Serial.println("Booted and connected");

  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);

  if(connectUDP()) {
    Serial.println("Listening UDP");
  }
}

void loop(){
  listenUdp();
}
