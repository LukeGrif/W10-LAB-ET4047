// ET4047 Week 10 - Secure Access Control (Adafruit RGB LCD shield version)
// Task 1: Hardware test
// Shows the state of every button and switch on the LCD.
// Button 1 lights the green LED, Button 2 lights the red LED.

#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>   // "Adafruit RGB LCD Shield Library"

// Adafruit RGB LCD shield (I2C, address 0x20)
Adafruit_RGBLCDShield lcd;

const byte BUTTON_PINS[3] = {2, 3, 4};   // B1, B2, B3
const byte SWITCH_PINS[3] = {5, 6, 7};   // SW1, SW2, SW3 (Extension only)
const byte LED_GREEN = 8;
const byte LED_RED   = 9;

// INPUT_PULLUP: the pin reads LOW when the button is pressed
bool isPressed(byte pin) {
  return digitalRead(pin) == LOW;
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < 3; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(SWITCH_PINS[i], INPUT_PULLUP);
  }
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  lcd.begin(16, 2);
  lcd.setBacklight(0x7);   // white backlight
}

void loop() {
  // Row 0: buttons (1 = pressed)
  lcd.setCursor(0, 0);
  lcd.print("Button: ");
  for (byte i = 0; i < 3; i++) {
    lcd.print(isPressed(BUTTON_PINS[i]) ? '1' : '0');
    lcd.print(' ');
  }

  // Row 1: switches (1 = ON, connected to GND)
  lcd.setCursor(0, 1);
  lcd.print("Switch: ");
  for (byte i = 0; i < 3; i++) {
    lcd.print(isPressed(SWITCH_PINS[i]) ? '1' : '0');
    lcd.print(' ');
  }

  // LEDs follow Button 1 and Button 2
  digitalWrite(LED_GREEN, isPressed(BUTTON_PINS[0]) ? HIGH : LOW);
  digitalWrite(LED_RED,   isPressed(BUTTON_PINS[1]) ? HIGH : LOW);

  delay(50);   // a short delay is fine in a test sketch
}
