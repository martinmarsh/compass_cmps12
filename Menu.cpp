#include <Arduino.h>
#include "Menu.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


Menu::Menu(char **items, int item_count,states *state_items, Adafruit_SSD1306 *display) {
  this->items = items;
  this->item_count = item_count;
  this->pdisplay_ = display;
  this->selected = 0;
  this->state_items = state_items;
}

states Menu::selectedState(){
  return this->state_items[this->selected];
}

void Menu::display(int dial_position) {
  this->selected = int(this->item_count * dial_position / 360 + 0.5);
  if (this->selected >= this->item_count) this->selected = this->item_count -1;
  if (this->selected < 0) this->selected = 0;
  this->pdisplay_->clearDisplay();
  this->pdisplay_->setTextColor(SSD1306_WHITE);
  this->pdisplay_->setTextSize(1);
  this->pdisplay_->setCursor(0, 0);
  for(int i = 0; i < this->item_count; ++i){
    if(i > 0) this->pdisplay_->print(F(" "));
    if(i == this->selected) this->pdisplay_->print(F("|"));
    else this->pdisplay_->print(this->items[i]);
  }
  this->pdisplay_->setTextSize(2);
  this->pdisplay_->setCursor(0, 16);
  this->pdisplay_->print(this->items[this->selected]);
  this->pdisplay_->display();
}

