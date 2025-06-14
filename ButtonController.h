// ButtonController.h
#ifndef BUTTON_CONTROLLER_H
#define BUTTON_CONTROLLER_H

#include <Arduino.h>
#include <Preferences.h>

class ButtonController {
  public:
    ButtonController(int pin, unsigned long longPressDuration);
    void begin();
    void update();
    void onShortPress(std::function<void()> callback);
    void onLongPress(std::function<void()> callback);

  private:
    int _pin;
    unsigned long _longPressDuration;
    unsigned long _pressStart = 0;
    bool _wasPressed = false;
    std::function<void()> _shortPressCallback = nullptr;
    std::function<void()> _longPressCallback = nullptr;
};

#endif