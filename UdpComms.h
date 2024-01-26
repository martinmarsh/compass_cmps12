#ifndef UdpComms_h
#define UdpComms_h
#include <Arduino.h>
#include "WiFi.h"
#include "AsyncUDP.h"

enum wifi_states {
  WIFI_CONNECTING_STATE,
  WIFI_JUST_CONNECTED_STATE,
  WIFI_CONNECTED_STATE,
  WIFI_NOT_CONNECTED_ERROR
};

enum wifi_listen_states {
  WIFI_WAIT_CONNECTION_STATE,
  WIFI_TRY_LISTENING_STATE,
  WIFI_LISTENING_PORT_READY_STATE,
  WIFI_LISTENING_STATE,
  WIFI_NOT_LITENING_ERROR
};

enum wifi_networks {
  A,
  B
};


class UdpComms {
  
  public:
    UdpComms(char* ssid, char* password, char* ssid2, char* password2, int broadcastPort, int listenPort, int retry_attempts);
    String connectStatusStr();
    void stateMachine();
    void broadcast(String message);
    String localIP();
    String listenPort();
    String broadcastPort();
    int number_of_wifi_retries;
    int wifi_status;
    int wifi_listen_status;
    int wifi_network;
   
  private:
    AsyncUDP udp_;
    void connectWiFi_();
    int max_retry_attempts_;
    char* ssid_ ;
    char* password_;
    char* ssid2_;
    char* password2_;
    int broadcastPort_;
    int listenPort_;  
};

#endif
