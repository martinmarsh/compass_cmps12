#include <Arduino.h>
#include "Compass.h"
#include <Wire.h>  // This is for i2C


Compass::Compass() {
  this->cmps12_status = 0;
}

// cms12 compass module 
void Compass::readCompass() {
  Wire.beginTransmission(CMPS12);
  Wire.write(0X02);
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS12, 4);
  this->heading = (int16_t)(Wire.read() << 8 | Wire.read()) / 10.00;  //Two bytes Yaw in range of (0 to 359 degrees)
  this->pitch1_ = ((int8_t)Wire.read());                              // One byte pitch in range of (-90 to 90 degrees)
  this->roll = ((int8_t)Wire.read());                                 // One byte roll in range of (-90 to 90 degrees)

  Wire.beginTransmission(CMPS12);
  Wire.write(0x18);
  Wire.endTransmission(false);
  Wire.requestFrom(CMPS12, 7);
  this->temperature = (int16_t)(Wire.read() << 8 | Wire.read());        // Temperature
  this->bosh_heading_ = (int16_t)(Wire.read() << 8 | Wire.read()) / 16;  // Boss Bearing
  this->pitch = (int16_t)(Wire.read() << 8 | Wire.read());             // Two bytes pitch in range of (-180 to 180 degrees)
  this->cmps12_status = ((uint8_t)Wire.read());
}

//returns 4 status components MAGS in form of a string "0000" to "3333" 
//MAGS = Magnetic, Acceleration, Gyro, System status each 0 poor to 3 good 
String Compass::status_str(){
  String status_str;
  int b = this->cmps12_status;
  status_str = this->bitToStr_(b);
  b = b >> 2;
  status_str += this->bitToStr_(b);
  b = b >> 2;
  status_str += this->bitToStr_(b);
  b = b >> 2;
  status_str += this->bitToStr_(b);
  return status_str;

}

float Compass::courseError(float desired_heading){
  return this->relative180(this->heading - desired_heading);
}

String Compass::bitToStr_(int b) {
  return String(b & 0x03);
}

float Compass::relative180(float dif) {
  if (dif < -180.0) {
    dif += 360.0;
  }
  if (dif > 180.0) {
    dif -= 360.0;
  }
  return dif;
}
