# UDP Broadcast Tilt Adjust Compass using ESP 32 FireBeetle

This is code for portable Tilt Adjusted Compass based on CMPS12 compass board with ability to broadcast via UDP at approx. 10Hz direction updates
including desired heading, Pitch, Roll and chip temperature.

A mini ASSD1306 based 1280x32 mini display is supported so that data can be read off the device as well as read from network devices.

A push button and rotary dial allows the desired heading to be set and for off course error to be displayed.

The portable unit should be housed in a water proof box so that it can be used in a marine environment and therefore instead of using a potentiometer to
adjust the desired course a magnetic rotary encoder is used.  Rotating magnet part is outside the box so no water can ingress. Also very accurate settings
can be made as there are 8 turns per 360 degrees of setting.  The magnet sensor is based on AS5600 chip encoding a value of 4096 per turn. 



