#ifndef Menu_h
#define Menu_h
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


enum states { COMPASS_STATE,
              AUTO_START_STATE,
              AUTO_STATE,
              AUTO_RETURN_STATE,
              IP_STATE,
              OFF_MENU_STATE,
              ON_MENU_STATE,
              GAIN_STATE,
              PI_STATE,
              PD_STATE,
              CONFIRM_PORT_TACK_STATE,
              CONFIRM_STAR_TACK_STATE,
              CONFIRM_WHICH_TACK_STATE,
              TACK_STATE,
              PORT_TACK_STATE,
              STAR_TACK_STATE };

class Menu  {
  
  public:
    Menu(char** items, int count, states *state_items, Adafruit_SSD1306 *display);
    void display(int dial_position);
    states selectedState();
    char** items;
    int item_count;
    int selected;
    states *state_items;
    states last_state;
   
  private:
    Adafruit_SSD1306 *pdisplay_;
   
};

#endif
