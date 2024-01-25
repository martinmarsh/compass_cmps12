#include <Arduino.h>

#include <Wire.h>  // This is for i2C

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdlib.h>

#include "Dial.h"
#include "Compass.h"
#include "UdpComms.h"


//The wifi passwords are stored in net_config.h which is not commited to public repo
//for security of wifi passwords etc
//Your must rename example_net_config.h to net_config.h when your passwords are set
//and comment in next line and comment out the example_net_config.h
#include "net_config.h"
//#include "example_net_config.h"


#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Dial dial;
Compass compass;

int g_loop_counter = 0;
int g_loop_cyles = 0;

float g_battery_volts;



enum states { COMPASS_STATE,
              AUTO_STATE,
              OFF_MENU_STATE,
              ON_MENU_STATE,
              };

enum actions { NO_ACTION,
               BUTTON_PUSHED_ACTION,
              DIAL_ACTION};

int g_state = COMPASS_STATE;


float g_desired_heading = 0;

int g_led_state = HIGH;

UdpComms udpComms(ssid, password, ssid2, password2, broadcastPort, listenPort, RETRY_PASSWORD);

void setup() {
  uint8_t* bufferpt;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);

  
  Serial.begin(115200);
  Serial.println("Starting");
  g_led_state = HIGH;
  digitalWrite(LED_BUILTIN, g_led_state);

  delay(1000);  // Pause for 1 seconds
  Serial.println("Setting Display Voltage");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  g_led_state = LOW;
  digitalWrite(LED_BUILTIN, g_led_state);

  display.display();

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  // display.display();
  //delay(2000);  // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  Wire.begin();            // start i2C
  Wire.setClock(400000L);  // fast clock

  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  display.clearDisplay();
  g_loop_counter = 0;
  g_loop_cyles = 0;

}


void loop() {
  g_led_state = LOW;
  digitalWrite(LED_BUILTIN, g_led_state);

  dial.readPushButton();
  for (int d = 0; d < 7; d++) {
    delay(10);
    dial.readPushButton();
  }
  g_led_state = HIGH;
  digitalWrite(LED_BUILTIN, g_led_state);
  ++g_loop_counter;
  if (g_loop_counter == 8) {
    g_loop_counter = 0;
    ++g_loop_cyles;
    fast_update();
    update();

    if (g_loop_cyles == 4) {
      g_loop_cyles = 0;
      slow_update();
    }
  }
  fast_update();
}


void update() {
  int action;

  if (dial.wasButtonPushed()) {
    action = BUTTON_PUSHED_ACTION;
    Serial.printf("Read Button pushed %i \n", action);
  } else if (dial.hasDialChanged()) {
    action = DIAL_ACTION;
  } else {
    action = NO_ACTION;
  }
  
  switch (g_state) {
    case COMPASS_STATE:
      switch(action){
        case BUTTON_PUSHED_ACTION:
          Serial.printf("Push action compass state %i \n", action);
          displayOffMenu();
          break;
        default:
          displayCompass(2);
        break;
      }
      break;

    case AUTO_STATE:

      switch(action){
        case BUTTON_PUSHED_ACTION:
          displayOnMenu();
          g_state = ON_MENU_STATE;
          break;
        case DIAL_ACTION:
          break;
        default:
          displayCompass(2);
        break;
      }
      break;

    case ON_MENU_STATE:

     switch(action){
        case BUTTON_PUSHED_ACTION:
          
          break;
        case DIAL_ACTION:
          nextMenuItem();
          break;
        default:
          displayOnMenu();
        break;
      }
      break;

    case OFF_MENU_STATE:

     switch(action){
        case BUTTON_PUSHED_ACTION:
         
          break;
        case DIAL_ACTION:
          nextMenuItem();
          break;
        default:
           displayOffMenu();
      }
      break;


    default:
      displayCompass(1);
      break;
  }
  
}



void slow_update() {

  g_battery_volts = analogRead(A0) * 0.001792;

  dial.checkAS5600Setup();  // check the magnet is present)

  udpComms.stateMachine();

  if (udpComms.wifi_status == WIFI_JUST_CONNECTED_STATE){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);  // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.println(F("WiFi Connected"));
    display.print(F("IP: "));
    display.println(WiFi.localIP());
    String listenStr = "UDP Listen Port: " + String(listenPort);
    display.println(listenStr);
    listenStr = "Broadcast Port: " + String(broadcastPort);
    display.println(listenStr);
    display.display();
     
    delay(5000);
  }

}


void fast_update() {
  String helm_str, status_str;
  char buff[15];

  dial.readAngle();
  compass.readCompass();

  helm_str = "H:";

  dtostrf(compass.heading, 1, 1, buff);
  helm_str += buff;

  dtostrf(g_desired_heading, 1, 1, buff);
  helm_str += ",D:" + String(buff);

  status_str = String(compass.cmps12_status);
  helm_str += ",S:" + String(status_str);

  dtostrf(compass.pitch, 1, 0, buff);
  helm_str += ",P:" + String(buff);


  dtostrf(compass.roll, 1, 0, buff);
  helm_str += ",R:" + String(buff);

  dtostrf(compass.temperature, 1, 0, buff);
  helm_str += ",T:" + String(buff);

  udpComms.broadcast(helm_str);
}



/*
-------------   Display Screens --------

*/


void nextMenuItem(){
  

}

void displayOffMenu() {
  display.clearDisplay();  // Clear the display buffer
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(F("On   Gain PI   PD"));
 
}

void displayOnMenu() {
  display.clearDisplay();  // Clear the display buffer
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(F("Off  Gain PI   PD"));
 
}


void displayCompass(int type) {
  char buff[9];
  float course_error;
  float abs_course_error;
  String heading_str, wifi_status_str, display_str, desired_heading_str, course_error_str;

  g_desired_heading = dial.getFineRotation(6);
  course_error = compass.courseError(g_desired_heading);
  abs_course_error = abs(course_error);

  if (abs_course_error < 10) {
    dtostrf(abs_course_error, 4, 1, buff);
    course_error_str = "-->";
    if (course_error > 0) {
      course_error_str = "<--";
    }
  } else {
    dtostrf(abs_course_error, 3, 0, buff);
    if ( course_error >= 100) {
      course_error_str = "<<<";
    } else if (course_error >= 10) {
      course_error_str = "<<-";
    } else if (course_error <= -100) {
      course_error_str = ">>>";
    } else if (course_error <= -10) {
      course_error_str = "->>";
    }
  }
  course_error_str += buff;

  display_str = compass.status_str();

  dtostrf(compass.heading, 3, 0, buff);
  heading_str = buff;

  dtostrf(g_desired_heading, 3, 0, buff);
  desired_heading_str = buff;

  dtostrf(compass.pitch, 6, 0, buff);
  display_str += buff;

  dtostrf(compass.roll, 6, 0, buff);
  display_str += buff;

  dtostrf(g_battery_volts, 5, 1, buff);
  display_str += buff;

  display.clearDisplay();  // Clear the display buffer

  display.setTextColor(SSD1306_WHITE);

  if (type == 1) {
    // standard display
    display.setCursor(32, 0);
    display.setTextSize(2);  // Draw 2X-scale text
    display.print(heading_str);

  } else {
    // set course display
    display.setCursor(0, 0);
    display.setTextSize(1);  // Draw 1X-scale text
    display.println(heading_str);
    display.print(desired_heading_str);
    display.setCursor(28, 0);
    display.setTextSize(2);
    display.println(course_error_str);
  }

  // Display wifi connected info
  display.setCursor(116, 0);
  display.setTextSize(1);
  wifi_status_str = udpComms.connectStatusStr();
  
  display.print(wifi_status_str);

  display.setCursor(0, 17);
  display.setTextSize(1);
  display.println(F("MAGS Pitch  Roll  Bat"));
  display.println(display_str);
  display.display();
}
