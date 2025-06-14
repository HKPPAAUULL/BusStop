// ButtonController.cpp
#include "ButtonController.h"

ButtonController::ButtonController(int pin, unsigned long longPressDuration)
  : _pin(pin), _longPressDuration(longPressDuration) {}

void ButtonController::begin() {
  pinMode(_pin, INPUT_PULLUP);
}

void ButtonController::update() {
  bool pressed = digitalRead(_pin) == LOW;

  if (pressed && !_wasPressed) {
    _pressStart = millis();
    _wasPressed = true;
  }

  if (!pressed && _wasPressed) {
    unsigned long duration = millis() - _pressStart;
    _wasPressed = false;

    if (duration >= _longPressDuration) {
      if (_longPressCallback) _longPressCallback();
    } else {
      if (_shortPressCallback) _shortPressCallback();
    }
  }
}

void ButtonController::onShortPress(std::function<void()> callback) {
  _shortPressCallback = callback;
}

void ButtonController::onLongPress(std::function<void()> callback) {
  _longPressCallback = callback;
}
