#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <led.h>



slacker::Led channels[3] = {
  slacker::Led(D0),
  slacker::Led(D1),
  slacker::Led(D3)
};

unsigned int localPort = 8888;
WiFiUDP UDP;
boolean udpConnected = false;
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

  if(!packetSize || packetSize != 2){
    // Serial.println("No length");
    return ;
  }

  IPAddress remote = UDP.remoteIP();
  for (int i =0; i < 4; i++){
    Serial.print(remote[i], DEC);
    if (i < 3) {
      Serial.print(".");
    }
  }
  Serial.print(": ");
  // read the packet into packetBufffer
  UDP.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
  int ledNumber = packetBuffer[0] - 0xF0;
  int ledPower = packetBuffer[1];

  setChannel(ledNumber, ledPower);
  /*Serial.print(ledNumber);
  Serial.println(ledPower);*/
  return ;
} // End of void listenUdp();

void setup(){
  Serial.begin(9600);
  Serial.println("Booted 1");

  // Network settings
  WiFiManager wifiManager;
  // wifiManager.autoConnect("AP-NAME", "AP-PASSWORD");
  wifiManager.autoConnect();
  Serial.println("Booted and connected");

  if(connectUDP()) {
    Serial.println("Listening UDP");
    listenUdp();
  }
}

void loop(){
  listenUdp();
}
