#include "WiFi.h"
#include "AsyncUDP.h"
#include <Wire.h>  // This is for i2C

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdlib.h>

//The wifi passwords are stored in net_config.h which is not commited to public repo
//for security of wifi passwords etc
//Your must rename example_net_config.h to net_config.h when your passwords are set
//and comment in next line and comment out the example_net_config.h
#include "net_config.h"
//#include "example_net_config.h"

#define AS5600 0x36  //Comment out if AS5600 not used otherwise set value of the status register (MD, ML, MH)
#define PUSH_BUTTON D2

#ifdef AS5600
// ----- AS5600 Magnetic sensor -- used to set heading
int magnetStatus = 0;  //value of the status register (MD, ML, MH)
int lowbyte;           //raw angle bits[7:0]
word highbyte;         //raw angle bits[11:8]
int rawAngle;          //final raw angle bits in degrees (360/4096 * [value between 0-4095])
int lastAngle;
bool AS5600Setup = true;
int rotations = 0;
#endif


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

int loopCounter = 0;
int loops = 0;
bool wifiConnected = false;
bool wifiConnecting = true;
bool wifiListening = false;
bool try1stPassword = true;
int number_of_wifi_retries = RETRY_PASSWORD;
float batVoltage;
bool buttonPushed = false;


// --- cmp12 Compass
int CMPS12 = 0x60;
float Pitch, Roll, Heading, Pitch2, BossHeading, Temp;
int cmps12Status;
String headingStr, statusStr, helm_string, displayStr, statStr, desiredHeadingStr, courseErrorStr;
char buff[15];
float desiredHeading = -1;
float courseError;

int toggle = HIGH;
AsyncUDP udp;
String wifiStatStr;

void setup() {
  uint8_t* bufferpt;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);


  Serial.begin(115200);
  Serial.println("Starting");
  delay(1000);  // Pause for 1 seconds
  Serial.println("Setting Display Voltage");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);  // Don't proceed, loop forever
  }

  display.display();

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  // display.display();
  //delay(2000);  // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  Wire.begin();            // start i2C
  Wire.setClock(400000L);  // fast clock

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  display.clearDisplay();
  loopCounter = 0;
  loops = 0;
  desiredHeading = -1;
  --number_of_wifi_retries;

#ifdef AS5600
  rotations == 0;
  AS5600Setup = false;
#endif
}


void loop() {
  readPushButton();
  for (int d = 0; d < 7; d++){
    delay(10);
    readPushButton();
  }
  ++loopCounter;
  if (loopCounter == 8) {
    loopCounter = 0;
    ++loops;
    fast_update();
    update();

    if (loops == 4) {
      toggle ^= HIGH;
      digitalWrite(LED_BUILTIN, toggle);
      loops = 0;
      slow_update();
    }
    display_update();
  }
  fast_update();
}


void display_update() {
  display.clearDisplay();  // Clear the display buffer

  display.setTextColor(SSD1306_WHITE);

  if (desiredHeading < 0) {
    // standard display
    display.setCursor(32, 0);
    display.setTextSize(2);  // Draw 2X-scale text
    display.print(headingStr);

  } else {
    // set course display
    display.setCursor(0, 0);
    display.setTextSize(1);  // Draw 1X-scale text
    display.println(headingStr);
    display.print(desiredHeadingStr);
    display.setCursor(40, 0);
    display.setTextSize(2);
    display.println(courseErrorStr);
  }

  // Display wifi connected info
  display.setCursor(116, 0);
  display.setTextSize(1);
  if (try1stPassword) {
    wifiStatStr = "A";
  } else {
    wifiStatStr = "B";
  }
  if (wifiConnecting) {
    wifiStatStr = "xX";
  } else {
    if (wifiConnected) {
      if (wifiListening) {
        wifiStatStr = " " + wifiStatStr;
      } else {
        wifiStatStr = "x" + wifiStatStr;
      }
    }
  }
  display.print(wifiStatStr);

  display.setCursor(0, 17);
  display.setTextSize(1);
  display.println(F("MAGS Pitch  Roll  Bat"));
  display.println(displayStr);
  display.display();
}

void update() {

  desiredHeading = getDesiredHeading();

  courseError = relative180(Heading - desiredHeading);
  float absCourseError = abs(courseError);
  courseErrorStr = " ";
  if (absCourseError <= 2) {
    dtostrf(courseError, -3, 1, buff);
    courseErrorStr = " >";
    if (courseError > 0) {
      courseErrorStr = " <";
    }
  } else {
    dtostrf(absCourseError, -3, 0, buff);
    if (courseError > 2) {
      courseErrorStr = " <";
    }
    if ((courseError > 5)) {
      courseErrorStr = "<-";
    }
    if ((courseError > 10)) {
      courseErrorStr = "<<";
    }
  }

  if (courseError < -2) {
    courseErrorStr = " >";
    if ((courseError < -5)) {
      courseErrorStr = "->";
    }
    if ((courseError < -10)) {
      courseErrorStr = ">>";
    }
  }
  courseErrorStr += buff;

  int b = cmps12Status;
  statStr = bitToStr(b);
  b = b >> 2;
  statStr += bitToStr(b);
  b = b >> 2;
  statStr += bitToStr(b);
  b = b >> 2;
  statStr += bitToStr(b);

  displayStr = statStr;

  dtostrf(Heading, 5, 0, buff);
  headingStr = buff;

  dtostrf(desiredHeading, 5, 0, buff);
  desiredHeadingStr = buff;

  dtostrf(Pitch, 6, 0, buff);
  displayStr += buff;

  dtostrf(Roll, 6, 0, buff);
  displayStr += buff;

  dtostrf(batVoltage, 5, 1, buff);
  displayStr += buff;
}


void slow_update() {
  int sensorValue = analogRead(A0);
  batVoltage = sensorValue * 0.001792;

  if (wasButtonPushed()) {
    Serial.println("Button was pushed");
  }

  checkAS5600Setup();  // check the magnet is present)
  if (wifiConnecting) {
    connectWiFi();
  } else {
    if (!wifiListening) {
      wifiListen();
      if (!wifiConnected) {
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
        wifiConnected = true;
        delay(5000);
      }
      if (wifiListening) {
        Serial.println("Listening started");
        udp.onPacket([](AsyncUDPPacket packet) {
          uint8_t buf[20];
          size_t l = packet.read(buf, 20);
          Serial.print("Data: ");
          Serial.write(packet.data(), packet.length());
          Serial.println();
          packet.printf("Got %u bytes of data", packet.length());
        });
      } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);  // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.println(F("Eror WiFi Not Listening"));
        String listenStr = "on UDP Port: " + String(listenPort);
        display.println(listenStr);
        display.display();
        delay(4000);
      }
    }
  }
}


void fast_update() {
  readAngle();
  readCompass();
 
  helm_string = "H:";

  dtostrf(Heading, 1, 1, buff);
  helm_string += buff;

  dtostrf(desiredHeading, 1, 1, buff);
  helm_string += ",D:" + String(buff);

  statusStr = String(cmps12Status);
  helm_string += ",S:" + String(statusStr);

  dtostrf(Pitch, 1, 0, buff);
  helm_string += ",P:" + String(buff);


  dtostrf(Roll, 1, 0, buff);
  helm_string += ",R:" + String(buff);

  dtostrf(Temp, 1, 0, buff);
  helm_string += ",T:" + String(buff);


  if (wifiConnected) {
    udp.broadcastTo(helm_string.c_str(), broadcastPort);
  }
}


void readPushButton() {
  if (digitalRead(PUSH_BUTTON) == LOW){
      buttonPushed = true;
   }
}


bool wasButtonPushed(){
  bool ret = buttonPushed;
  buttonPushed = false;
  return ret;

}

String bitToStr(int b) {
  return String(b & 0x03);
}


void connectWiFi() {
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    if (try1stPassword) {
      Serial.println("WiFi Failed - password A");
      WiFi.begin(ssid, password);
    } else {
      Serial.println("WiFi Failed - password B");
      WiFi.begin(ssid2, password2);
    }
    if (--number_of_wifi_retries == 0) {
      try1stPassword = !try1stPassword;
      number_of_wifi_retries = RETRY_PASSWORD;
    }

  } else {
    wifiConnecting = false;
  }
}


void wifiListen() {
  if (udp.listen(listenPort)) {
    wifiListening = true;
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
  }
}


float relative180(float dif) {
  if (dif < -180.0) {
    dif += 360.0;
  }
  if (dif > 180.0) {
    dif -= 360.0;
  }
  return dif;
}


// cms12 compass module   8bit bearing: 0x12
void readCompass() {
  Wire.beginTransmission(CMPS12);
  Wire.write(0X02);
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS12, 4);
  Heading = (int16_t)(Wire.read() << 8 | Wire.read()) / 10.00;  //Two bytes Yaw in range of (0 to 359 degrees)
  Pitch = ((int8_t)Wire.read());                                // One byte Pitch in range of (-90 to 90 degrees)
  Roll = ((int8_t)Wire.read());                                 // One byte Roll in range of (-90 to 90 degrees)

  Wire.beginTransmission(CMPS12);
  Wire.write(0x18);
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS12, 7);
  Temp = (int16_t)(Wire.read() << 8 | Wire.read());              // Temperature
  BossHeading = (int16_t)(Wire.read() << 8 | Wire.read()) / 16;  // Boss Bearing
  Pitch2 = (int16_t)(Wire.read() << 8 | Wire.read());            // Two bytes Pitch in range of (-180 to 180 degrees)
  cmps12Status = ((uint8_t)Wire.read());
}

void readAngle() {
#ifdef AS5600
  if (AS5600Setup) {
    readRawAngle();
    computeRotations();
  }
#endif
}

void checkAS5600Setup() {
#ifdef AS5600
  if (!AS5600Setup) {
    checkMagnetPresence();
  }
#endif
}

float getDesiredHeading() {
  // If present the dial changing the desired heading will only activate heading set mode
  // when turned at least one rotation or when a set heading message is sent.
  // Once set the mode will remain until the unit is powerred off or a set heading message of -1 is
  // sent. Further dial rotations will not change the mode.

  float ret = -1;

#ifdef AS5600
  if (AS5600Setup && rotations != 0) {
    desiredHeading = 0;
    ret = 0;
  }
  if (AS5600Setup && desiredHeading >= 0) {
    int absRot = abs(rotations) % 4;
    ret = (rawAngle + absRot * 4096) * 0.02197265625;
  }
#endif

  if (ret < 0) {
    ret = desiredHeading;
  }
  return ret;
}



#ifdef AS5600
// ----------------------
// AS5600 Rotation sensor  Code
// ----------------------

void checkMagnetPresence() {
  //Status register output: 0 0 MD ML MH 0 0 0
  //MH: Too strong magnet  -  101000  - dec 40
  //ML: Too weak magnet -     110000  - dec 48
  //MD: OK magnet -           100000  - dec 32

  // ---- Check MD status bit (magnet detected)
  Serial.print("Magnet Status  ");
  Serial.println("Checking Magnetic Status:");
  
  if ((magnetStatus & B00100000) != 32) {  // detect correctly positioned magnet
    magnetStatus = 0;

    Wire.beginTransmission(AS5600);  
    Wire.write(0x0B);                // register map: Status: MD ML MH
    Wire.endTransmission();          
    Wire.requestFrom(AS5600, 1);  

    Serial.println("Waiting:");
    while (Wire.available() == 0);   //wait until available

    magnetStatus = Wire.read() & B00111000;
    AS5600Setup = true;
  }
  Serial.print("Magnet Status  ");
  Serial.println(magnetStatus, BIN);
  Serial.println(" ");
}


void readRawAngle() {
  //----- read low-order bits 7:0
  Wire.beginTransmission(AS5600);  //connect to the sensor
  Wire.write(0x0D);                //figure 21 - register map: Raw angle (7:0)
  Wire.endTransmission();          //end transmission
  Wire.requestFrom(AS5600, 1);     //request from the sensor
  while (Wire.available() == 0)
    ;                     //wait until it becomes available
  lowbyte = Wire.read();  //Reading the data after the request

  // ----- read high-order bits 11:8
  Wire.beginTransmission(AS5600);
  Wire.write(0x0C);  //figure 21 - register map: Raw angle (11:8)
  Wire.endTransmission();
  Wire.requestFrom(AS5600, 1);
  while (Wire.available() == 0)
    ;
  highbyte = Wire.read();

  // ----- combine bytes
  highbyte = highbyte << 8;       // shift highbyte to left
  rawAngle = highbyte | lowbyte;  // combine bytes to get 12-bit value 11:0
}


void computeRotations() {
  // Counts the number of rotations clockwise positive
  if (rawAngle < 1028 && lastAngle > 3084) {
    ++rotations;
  } else if (rawAngle > 3084 && lastAngle < 1028) {
    --rotations;
  }
  lastAngle = rawAngle;
}

#endif
