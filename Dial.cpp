#include <Arduino.h>
#include "Dial.h"
#include <Wire.h>  // This is for i2C

Dial::Dial() {
  this->angle_ = 0;
  this->rotations_ = 0;
  this->button_pushed_= false;
  this->AS5600Setup_ = false;
}


bool Dial::wasButtonPushed() {
  bool ret = this->button_pushed_ ;
  this->button_pushed_ = false;
  return ret;
}


bool Dial::readPushButton() {
  if (digitalRead(PUSH_BUTTON) == LOW) {
    this->button_pushed_= true;
  }
}


bool Dial::hasDialChanged(){
  bool ret = false;
  if (this->angle_ != this->last_angle_ || this->rotations_ != this->last_rotations_) ret = true;
  return ret;  
}


void Dial::readAngle() {
  if (this->AS5600Setup_) {
    this->readRawAngle_();
    this->computeRotations_();
  }
}

void Dial::checkAS5600Setup() {
  if (!this->AS5600Setup_) {
    this->checkMagnetPresence_();
  }
}

float Dial::getFineRotation(int turns) {
  // turns is number of roations for 360
  return abs((this->angle_ + this->rotations_ * 4096) % (4096 * turns)) * 0.087890625 / turns;
}

// AS5600 Rotation sensor  Code
// ----------------------

void Dial::checkMagnetPresence_() {
#ifdef AS5600
  //Status register output: 0 0 MD ML MH 0 0 0
  //MH: Too strong magnet  -  101000  - dec 40 - hex 28
  //ML: Too weak magnet -     110000  - dec 48 - hex 30
  //MD: OK magnet -           100000  - dec 32 - hex 20

  // ---- Check MD status bit (magnet detected)
  Serial.print("Magnet Status  ");
  Serial.println("Checking Magnetic Status:");

  if ((this->magnet_status_ & B00100000) != 32) {  // detect correctly positioned magnet
    this->magnet_status_  = 0;

    Wire.beginTransmission(AS5600);
    Wire.write(0x0B);  // register map: Status: MD ML MH
    Wire.endTransmission();
    Wire.requestFrom(AS5600, 1);

    Serial.println("Waiting:");
    while (Wire.available() == 0)
      ;  //wait until available

    this->magnet_status_  = Wire.read() & B00111000;
    this->AS5600Setup_ = true;
  }
  Serial.printf("Magnet Status  %02x \n",this->magnet_status_ );
 
#endif
}


void Dial::readRawAngle_() {
#ifdef AS5600  
  int low_byte;    //raw angle bits[7:0]
  word high_byte;  //raw angle bits[11:8]

  //----- read low-order bits 7:0
  Wire.beginTransmission(AS5600);  //connect to the sensor
  Wire.write(0x0D);                //figure 21 - register map: Raw angle (7:0)
  Wire.endTransmission();          //end transmission
  Wire.requestFrom(AS5600, 1);     //request from the sensor
  while (Wire.available() == 0)
    ;                      //wait until it becomes available
  low_byte = Wire.read();  //Reading the data after the request

  // ----- read high-order bits 11:8
  Wire.beginTransmission(AS5600);
  Wire.write(0x0C);  //figure 21 - register map: Raw angle (11:8)
  Wire.endTransmission();
  Wire.requestFrom(AS5600, 1);
  while (Wire.available() == 0)
    ;
  high_byte = Wire.read();

  // ----- combine bytes
  high_byte = high_byte << 8;          // shift highbyte to left
  this->angle_ = high_byte | low_byte;  // combine bytes to get 12-bit value 11:0
#endif
}

void Dial::computeRotations_() {
  
  // Counts the number of rotations clockwise positive
  if (this->angle_ < 1028 && this->last_angle_read_ > 3084) {
    ++this->rotations_;
  } else if (this->angle_  > 3084 &&  this->last_angle_read_ < 1028) {
    --this->rotations_;
  }
  this->last_angle_read_  = this->angle_ ;
}






