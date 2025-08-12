#include <Arduino.h>
#include "Dial.h"
#include <Wire.h>  // This is for i2C

Dial::Dial() {
  this->angle_ = 0;
  this->rotations_ = 0;
  this->button_pushed_= false;
  this->AS5600Setup_ = false;
  this->turns_modulus_ = 4096;
  this->scale_turns_ = 0.087890625;
  this->offset_= 0;
  this->base_angle = 0;
  this->base_offset = 0;
  this->button_release_count_ = 0;
  this->last_angle_read_ = 0;
}


bool Dial::wasButtonPushed() {
  bool ret = this->button_pushed_ ;
  this->button_pushed_ = false;
  return ret;
} 


bool Dial::readPushButton() {
  bool state = (digitalRead(PUSH_BUTTON) == LOW);
        
  if (this->button_release_count_ <= 0 &&  state == true){
    this->button_pushed_= true;
    this->button_release_count_ = 10;
    Serial.printf("button down %i\n", this->button_release_count_ );
  } else if (state == true){
    this->button_release_count_ = 10;
  } else {
    --this->button_release_count_;
  }
} 


bool Dial::hasDialChanged(){
  bool ret = false;
  if (this->angle_ != this->last_angle_ || this->rotations_ != this->last_rotations_) ret = true;
  this->last_angle_  = this->angle_;
  this->last_rotations_ = this->rotations_ ;
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

void Dial::setBase(int turns, float offset_degrees) {
  // sets number of full turns/rotations for 360
  // sets the zero point in degrees.
  // Can be changes any time before getRoation
  float rotation;
  if (turns > 0) {
    this->turns_modulus_ = 4096 * turns;
    this->scale_turns_ = 0.087890625 / turns;
    this->offset_= 0;   //zero offset for getRotation to find true rotation
    if (this->rotations_ > 0 ){
       this->rotations_ = 0;
    } else if (this->rotations_ < 0 ){
        this->rotations_ = -1;
    }
    rotation = this->getRotation();
    this->base_angle = this->angle_;
    this->base_offset = offset_degrees;
    this->offset_ = rotation - offset_degrees;
    Serial.printf("Dial base turns= %i, total offset: %.2f, rotation:  %.2f, set offset:  %.2f, angle: %i\n",turns,
     this->offset_, rotation, offset_degrees, this->base_angle);
  }
}

float Dial::withinCircle(float x){
  // Not so eligant as:
  // x= i%360 ;
  // x += x<0?360:0;
  // which should work for modern C++ implementations
  // but does not rely on undefined behaviour of negatives in mod
  while (x > 360.0) x -= 360.0;
  while (x < 0) x += 360.0;
  return x;
}

float Dial::LeftRight(float x){
  // Returns Left or Right deflection -180 to +180
  if (x > 180.0){
      x = x - 360.0;
  }
  return x;
}

float Dial::getRotation() {
  //returns 0 to 360 with finer resolution due to multi-turn scalling. The offset applied will push outside a circle so
  //this is corrected by within circle function.
  //The offset is used to fix a menu position value or return a preset value and to compensate for starting wheel position
 
  return this->withinCircle(((abs(this->angle_ + this->rotations_ * 4096) % this->turns_modulus_) * this->scale_turns_) - this->offset_);
}

float Dial::getLeftRightRotation(float min, float max) {
  // turns degrees scaled by multi-turns  
  float value = ((this->angle_ - this->base_angle) + this->rotations_ * 4096) * this->scale_turns_ + this->base_offset;
  if (value < min){
    value = min;
  }
  if (value > max){
    value = max;
  }
  return value;
}

float Dial::getRightLeftRotation(float min, float max) {
  // turns degrees scaled by multi-turns  
  float value = (( this->base_angle - this->angle_) - this->rotations_ * 4096) * this->scale_turns_ + this->base_offset;
  if (value < min){
    value = min;
  }
  if (value > max){
    value = max;
  }
  return value;
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






