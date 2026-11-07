// ET4047 Week 10 - Secure Access Control (I2C backpack LCD version)
// Task 2: Debounced buttons and code entry
// Each press is counted once. The LCD shows the code typed so far
// and compares raw edges (with bounce) against clean presses.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>   // "LiquidCrystal I2C" by Frank de Brabander

// 16x2 LCD with an I2C backpack. The lab displays are at address 0x20.
// Blank screen? Try 0x27 or 0x3F, and turn the contrast screw.
LiquidCrystal_I2C lcd(0x20, 16, 2);

const byte BUTTON_PINS[3] = {2, 3, 4};
const unsigned long DEBOUNCE_MS = 40;    // try 0 and compare the counts

// ---------- Debounced button ----------
struct Button {
  byte pin;
  bool stableState;            // last accepted state
  bool lastReading;            // last raw reading
  unsigned long lastChange;    // time of the last raw change
};

Button buttons[3];
unsigned int rawEdges = 0;     // every HIGH->LOW edge, including bounces
unsigned int cleanPresses = 0; // presses accepted after debouncing

// Returns true ONCE per press (on the HIGH -> LOW edge)
bool buttonPressed(byte i) {
  bool reading = digitalRead(buttons[i].pin);

  if (reading != buttons[i].lastReading) {   // input moved: maybe a bounce
    if (reading == LOW) rawEdges++;
    buttons[i].lastReading = reading;
    buttons[i].lastChange = millis();
  }

  // Accept the new state only after it has been steady for DEBOUNCE_MS
  if (millis() - buttons[i].lastChange >= DEBOUNCE_MS &&
      reading != buttons[i].stableState) {
    buttons[i].stableState = reading;
    if (reading == LOW) {
      cleanPresses++;
      return true;
    }
  }
  return false;
}

// Returns 1, 2 or 3 for a new press, or 0 for no press
byte readButtons() {
  for (byte i = 0; i < 3; i++) {
    if (buttonPressed(i)) return i + 1;
  }
  return 0;
}

// ---------- Code entry ----------
const byte CODE_LENGTH = 3;
byte entered[CODE_LENGTH];
byte count = 0;

void clearCode() {
  count = 0;
}

void showScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Code: ");
  for (byte i = 0; i < CODE_LENGTH; i++) {
    if (i < count) lcd.print(entered[i]);
    else           lcd.print('_');
    lcd.print(' ');
  }
  lcd.setCursor(0, 1);
  lcd.print("raw:");
  lcd.print(rawEdges);
  lcd.print(" clean:");
  lcd.print(cleanPresses);
}

void setup() {
  Serial.begin(9600);
  for (byte i = 0; i < 3; i++) {
    buttons[i].pin = BUTTON_PINS[i];
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].stableState = HIGH;
    buttons[i].lastReading = HIGH;
    buttons[i].lastChange = 0;
  }
  lcd.init();
  lcd.backlight();
  showScreen();
}

void loop() {
  byte b = readButtons();
  if (b != 0) {
    if (count == CODE_LENGTH) clearCode();   // 4th press starts a new code
    entered[count++] = b;
    Serial.print("Button ");
    Serial.println(b);
    showScreen();
  }
}
