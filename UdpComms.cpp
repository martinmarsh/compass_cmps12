#include <Arduino.h>
#include "UdpComms.h"
#include <Wire.h>  // This is for i2C
#include "WiFi.h"


//globals are required to access be in scope within Async call back
uint8_t g_recBuf[61];
int g_recLen;
bool g_recLocked;           

UdpComms::UdpComms(char* ssid, char* password, char* ssid2, char* password2, int broadcastPort, int listenPort, int retry_attempts) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    this->max_retry_attempts_ = retry_attempts;
    this->number_of_wifi_retries = retry_attempts;
    this->wifi_status = WIFI_CONNECTING_STATE;
    this->wifi_listen_status = WIFI_WAIT_CONNECTION_STATE;
    this->wifi_network = A;
    this->ssid_ = ssid;
    this->password_ = password;
    this->ssid2_ = ssid2;
    this->password2_ = password2;
    this->broadcastPort_ = broadcastPort;
    this->listenPort_ = listenPort;
    this->receivedMessage = "";
    this->messageReady_  = false;
    g_recLocked = false;
}

bool UdpComms::messageAvailable(){
  if (g_recLocked == true && this->messageReady_ == false){
      this->receivedMessage = (char *) g_recBuf;
      this->messageReady_ = true;
      Serial.printf("Set message available:  %u byte buffer: %s message: %s\n", g_recLen, g_recBuf, this->receivedMessage);
  } 
  return this->messageReady_;
}


void UdpComms::nextMessage(){
  this->messageReady_ = false;
  g_recLocked = false;
}

String UdpComms::localIP(){
  return WiFi.localIP().toString();
}

String UdpComms::listenPort(){
  return String(this->listenPort_);

}
String UdpComms::broadcastPort(){
   return String(this->broadcastPort_);
}

void UdpComms::connectWiFi_() {

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    if (this->wifi_network == A) {
      Serial.println("Try WiFi - password A");
      WiFi.begin(this->ssid_, this->password_);
    } else {
      Serial.println("Try WiFi -  password B");
      WiFi.begin(this->ssid2_, this->password2_);
    }
    if (--this->number_of_wifi_retries <= 0) {
      Serial.println("Switch network to try other password");
      if (this->wifi_network == A) {
        this->wifi_network = B;
      } else {
        this->wifi_network = A;
      }
      this->number_of_wifi_retries = this->max_retry_attempts_;
    }

  } else {
    this->wifi_status = WIFI_JUST_CONNECTED_STATE;
  }
}


String UdpComms::connectStatusStr(){
  String wifi_status_str;
  if (this->wifi_network == A) {
    wifi_status_str = "A";
  } else {
    wifi_status_str = "B";
  }
  if (this->wifi_status == WIFI_CONNECTING_STATE) {
    wifi_status_str = "xX";
  } else if (this->wifi_listen_status == WIFI_LISTENING_STATE) {
    wifi_status_str = " " + wifi_status_str;
  } else {
    wifi_status_str = "x" + wifi_status_str; 
  }
  return wifi_status_str;
}

void UdpComms::stateMachine(){
  
  switch (this->wifi_status) {
    case WIFI_CONNECTING_STATE:
      this->connectWiFi_();
      break;
    case WIFI_JUST_CONNECTED_STATE:
      this->wifi_status = WIFI_CONNECTED_STATE;
      this->wifi_listen_status = WIFI_TRY_LISTENING_STATE;
      break;
  }

  switch (this->wifi_listen_status) {
    case WIFI_WAIT_CONNECTION_STATE:
      break;
    case WIFI_TRY_LISTENING_STATE:
      if (this->udp_.listen(this->listenPort_)) this->wifi_listen_status = WIFI_LISTENING_PORT_READY_STATE;
      break;
    case WIFI_LISTENING_PORT_READY_STATE:
      Serial.println("Port ready Listening starting");
      this->udp_.onPacket([](AsyncUDPPacket packet) {
        if ( !g_recLocked){ 
          //only supports one packet length message
          g_recLocked = true;
          size_t g_recLen = packet.read(g_recBuf, 60);
          if (g_recLen > 0 && g_recLen <= 61) {
              g_recBuf[g_recLen] = 0;
              Serial.printf("Got message in call back:  %u byte %s\n", g_recLen, g_recBuf);
              packet.printf("Got %u bytes\n", g_recLen);
          } else{
            // discard packet
            g_recLocked = true;
          }
        }
      });
        
      this->wifi_listen_status = WIFI_LISTENING_STATE;
      break;
  }
  
}

void UdpComms::broadcast(String message){
  if (WIFI_CONNECTED_STATE == this->wifi_status) {
    this->udp_.broadcastTo(message.c_str(), this->broadcastPort_);
  }
}
