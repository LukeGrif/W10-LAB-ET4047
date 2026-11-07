#ifndef BUTTONS_H
#define BUTTONS_H

// ------------------------------------------------------------
// Debounced push buttons (the Task 2 code, packaged for reuse)
//
//   initButtons();          // in setup()
//   byte b = readButtons(); // in loop(): 1, 2, 3 = new press, 0 = none
//
// Buttons are wired pin -> button -> GND and use INPUT_PULLUP,
// so a pressed button reads LOW.
// ------------------------------------------------------------

#include <Arduino.h>

const byte BUTTON_PINS[3] = {2, 3, 4};   // B1, B2, B3
const unsigned long DEBOUNCE_MS = 40;

struct Button {
  byte pin;
  bool stableState;
  bool lastReading;
  unsigned long lastChange;
};

Button buttons[3];

void initButtons() {
  for (byte i = 0; i < 3; i++) {
    buttons[i].pin = BUTTON_PINS[i];
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].stableState = HIGH;
    buttons[i].lastReading = HIGH;
    buttons[i].lastChange = 0;
  }
}

// True once per press (HIGH -> LOW after DEBOUNCE_MS of steady input)
bool buttonPressed(byte i) {
  bool reading = digitalRead(buttons[i].pin);
  if (reading != buttons[i].lastReading) {
    buttons[i].lastReading = reading;
    buttons[i].lastChange = millis();
  }
  if (millis() - buttons[i].lastChange >= DEBOUNCE_MS &&
      reading != buttons[i].stableState) {
    buttons[i].stableState = reading;
    return reading == LOW;
  }
  return false;
}

// 1, 2 or 3 for a new press, 0 for none
byte readButtons() {
  for (byte i = 0; i < 3; i++) {
    if (buttonPressed(i)) return i + 1;
  }
  return 0;
}

#endif // BUTTONS_H
