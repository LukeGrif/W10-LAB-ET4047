// ET4047 Week 10 - Secure Access Control (I2C backpack LCD version)
// Task 4: Authentication with attempt counter, lockout and timeout
// Adds the LOCKOUT state to the Task 3 FSM. All timing uses millis().
// Needs the buttons.h tab.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>   // "LiquidCrystal I2C" by Frank de Brabander

// 16x2 LCD with an I2C backpack. The lab displays are at address 0x20.
// Blank screen? Try 0x27 or 0x3F, and turn the contrast screw.
LiquidCrystal_I2C lcd(0x20, 16, 2);

#include "buttons.h"

const byte LED_GREEN = 8;
const byte LED_RED   = 9;

const byte CODE_LENGTH = 3;
const byte PASSWORD[CODE_LENGTH] = {1, 3, 2};

const byte MAX_ATTEMPTS = 3;
const unsigned long GRANTED_MS = 3000;
const unsigned long DENIED_MS  = 2000;
const unsigned long ENTRY_MS   = 10000;   // finish the code within 10 s
const unsigned long LOCKOUT_MS = 10000;   // locked for 10 s

// ---------- FSM ----------
enum State { IDLE, ENTER_CODE, ACCESS_GRANTED, ACCESS_DENIED, LOCKOUT };
State state;
unsigned long stateStart;

byte entered[CODE_LENGTH];
byte count = 0;
byte failedAttempts = 0;
int lastSecondsShown = -1;       // so the countdown redraws once a second

void showCode() {
  lcd.setCursor(0, 1);
  lcd.print("Code: ");
  for (byte i = 0; i < CODE_LENGTH; i++) {
    if (i < count) lcd.print(entered[i]);
    else           lcd.print('_');
    lcd.print(' ');
  }
}

bool codeIsCorrect() {
  for (byte i = 0; i < CODE_LENGTH; i++) {
    if (entered[i] != PASSWORD[i]) return false;
  }
  return true;
}

void enterState(State s) {
  state = s;
  stateStart = millis();
  lcd.clear();
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  switch (s) {
    case IDLE:
      count = 0;
      lcd.print("ENTER ACCESS");
      showCode();
      break;
    case ENTER_CODE:
      lcd.print("ENTERING...");
      showCode();
      break;
    case ACCESS_GRANTED:
      digitalWrite(LED_GREEN, HIGH);
      lcd.print("ACCESS GRANTED");
      break;
    case ACCESS_DENIED:
      digitalWrite(LED_RED, HIGH);
      lcd.print("ACCESS DENIED");
      lcd.setCursor(0, 1);
      lcd.print("Tries left: ");
      lcd.print(MAX_ATTEMPTS - failedAttempts);
      break;
    case LOCKOUT:
      digitalWrite(LED_RED, HIGH);
      lcd.print("SYSTEM LOCKED");
      lastSecondsShown = -1;
      break;
  }
  Serial.print("State -> ");
  Serial.println(s);
}

// Called when a code is wrong OR the entry time runs out
void failedAttempt() {
  failedAttempts++;
  if (failedAttempts >= MAX_ATTEMPTS) enterState(LOCKOUT);
  else                                enterState(ACCESS_DENIED);
}

void setup() {
  Serial.begin(9600);
  initButtons();
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  lcd.init();
  lcd.backlight();
  enterState(IDLE);
}

void loop() {
  byte b = readButtons();
  unsigned long now = millis();
  unsigned long elapsed = now - stateStart;

  switch (state) {
    case IDLE:
      if (b != 0) {
        entered[count++] = b;
        enterState(ENTER_CODE);
      }
      break;

    case ENTER_CODE:
      if (elapsed >= ENTRY_MS) {          // too slow: counts as a failure
        failedAttempt();
      } else if (b != 0) {
        entered[count++] = b;
        showCode();
        if (count == CODE_LENGTH) {
          if (codeIsCorrect()) {
            failedAttempts = 0;
            enterState(ACCESS_GRANTED);
          } else {
            failedAttempt();
          }
        }
      }
      break;

    case ACCESS_GRANTED:
      if (elapsed >= GRANTED_MS) enterState(IDLE);
      break;

    case ACCESS_DENIED:
      if (elapsed >= DENIED_MS) enterState(IDLE);
      break;

    case LOCKOUT:
      // Buttons are read but ignored: nothing can be entered while locked
      if (elapsed >= LOCKOUT_MS) {
        failedAttempts = 0;
        enterState(IDLE);
      } else {
        int secondsLeft = (LOCKOUT_MS - elapsed + 999) / 1000;
        if (secondsLeft != lastSecondsShown) {      // redraw once a second
          lastSecondsShown = secondsLeft;
          lcd.setCursor(0, 1);
          lcd.print("Wait ");
          lcd.print(secondsLeft);
          lcd.print(" s   ");
        }
      }
      break;
  }
}
