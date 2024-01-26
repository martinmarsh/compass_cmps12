#ifndef Compass_h
#define Compass_h
#include <Arduino.h>
#include <Wire.h>  // This is for i2C

#define CMPS12 0x60  // --- cmp12 Compass at address 0x60

class Compass  {
  
  public:
    Compass();
    void readCompass();
    String status_str();
    float courseError(float desired_heading);
    float relative180(float dif);
    float pitch;
    float roll; 
    float heading;
    float pitch2;
    float temperature;
    int   cmps12_status;
    
  private:
    String bitToStr_(int b);
    float bosh_heading_;
    float pitch1_;
     
};

#endif
