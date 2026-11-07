// ET4047 Week 10 - Secure Access Control (I2C backpack LCD version)
// Extension: operating modes with three switches (Task 5 + switches)
//   SW1 (D5) ON  = system enabled    OFF = SYSTEM OFF, keypad ignored
//   SW2 (D6) ON  = enhanced security: 5 s to enter the code (not 10 s)
//   SW3 (D7) ON  = maintenance: needs the maintenance code 3-2-1 and
//                  clears the progressive lockout level on success
// A switch is ON when its pin is connected to GND. A jumper wire from
// the pin to GND works as a switch.
// Needs the buttons.h tab.

#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>   // "LiquidCrystal I2C" by Frank de Brabander

// 16x2 LCD with an I2C backpack. The lab displays are at address 0x20.
// Blank screen? Try 0x27 or 0x3F, and turn the contrast screw.
LiquidCrystal_I2C lcd(0x20, 16, 2);

#include "buttons.h"

const byte LED_GREEN = 8;
const byte LED_RED   = 9;

const byte CODE_LENGTH = 3;
const byte PASSWORD[CODE_LENGTH]    = {1, 3, 2};
const byte MAINT_CODE[CODE_LENGTH]  = {3, 2, 1};

const byte SW_SYSTEM   = 5;    // SW1
const byte SW_ENHANCED = 6;    // SW2
const byte SW_MAINT    = 7;    // SW3

const byte MAX_ATTEMPTS = 3;
const unsigned long GRANTED_MS      = 3000;
const unsigned long DENIED_MS       = 2000;
const unsigned long ENTRY_MS        = 10000;
const unsigned long ENHANCED_ENTRY_MS = 5000;
const unsigned long BASE_LOCKOUT_MS = 10000;
const byte MAX_LOCKOUT_LEVEL        = 4;      // 10 s * 2^4 = 160 s

// [4] EEPROM addresses
const int EE_MAGIC    = 0;    // 0xA5 once the EEPROM has been set up
const int EE_FAILED   = 1;    // failed attempts since last success
const int EE_LOCKOUTS = 2;    // how many lockouts in a row
const byte MAGIC = 0xA5;

enum State { IDLE, ENTER_CODE, ACCESS_GRANTED, ACCESS_DENIED, LOCKOUT, SYSTEM_OFF };
State state;
unsigned long stateStart;
unsigned long lockoutMs;

byte entered[CODE_LENGTH];
byte count = 0;
byte failedAttempts = 0;
byte lockoutLevel = 0;
int lastSecondsShown = -1;
bool maintenance = false;      // chosen when the attempt starts

bool switchOn(byte pin) { return digitalRead(pin) == LOW; }

void saveCounters() {
  EEPROM.update(EE_FAILED, failedAttempts);   // update() only writes if changed
  EEPROM.update(EE_LOCKOUTS, lockoutLevel);
}

void loadCounters() {
  if (EEPROM.read(EE_MAGIC) != MAGIC) {       // first run: start clean
    EEPROM.update(EE_MAGIC, MAGIC);
    failedAttempts = 0;
    lockoutLevel = 0;
    saveCounters();
  } else {
    failedAttempts = EEPROM.read(EE_FAILED);
    lockoutLevel = min(EEPROM.read(EE_LOCKOUTS), MAX_LOCKOUT_LEVEL);
  }
}

void showCode() {
  lcd.setCursor(0, 1);
  lcd.print("Code: ");
  for (byte i = 0; i < CODE_LENGTH; i++) {
    lcd.print(i < count ? '*' : '_');          // [1] masked
    lcd.print(' ');
  }
}

// [5] Always compares every digit, so the time taken never depends
//     on how many digits were right
bool codeIsCorrect() {
  const byte *expected = maintenance ? MAINT_CODE : PASSWORD;
  byte differences = 0;
  for (byte i = 0; i < CODE_LENGTH; i++) {
    differences |= entered[i] ^ expected[i];
  }
  return differences == 0;
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
      lcd.print(maintenance ? "MAINTENANCE" : "ENTERING...");
      showCode();
      break;
    case ACCESS_GRANTED:
      digitalWrite(LED_GREEN, HIGH);
      lcd.print("ACCESS GRANTED");
      if (maintenance) {
        lcd.setCursor(0, 1);
        lcd.print("Maintenance OK");
      }
      break;
    case ACCESS_DENIED:
      digitalWrite(LED_RED, HIGH);
      lcd.print("ACCESS DENIED");                // [2] nothing else
      break;
    case LOCKOUT:
      digitalWrite(LED_RED, HIGH);
      lcd.print("SYSTEM LOCKED");
      lockoutMs = BASE_LOCKOUT_MS << lockoutLevel;   // [3] 10,20,40,80,160 s
      lastSecondsShown = -1;
      break;
    case SYSTEM_OFF:
      lcd.print("SYSTEM OFF");
      lcd.setCursor(0, 1);
      lcd.print("Turn SW1 ON");
      break;
  }
}

void failedAttempt() {
  failedAttempts++;
  saveCounters();                 // [4] saved BEFORE any lockout starts
  if (failedAttempts >= MAX_ATTEMPTS) enterState(LOCKOUT);
  else                                enterState(ACCESS_DENIED);
}

void setup() {
  Serial.begin(9600);
  initButtons();
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(SW_SYSTEM, INPUT_PULLUP);
  pinMode(SW_ENHANCED, INPUT_PULLUP);
  pinMode(SW_MAINT, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  loadCounters();

  // [4] Reset during a lockout? Start the lockout again.
  if (failedAttempts >= MAX_ATTEMPTS) enterState(LOCKOUT);
  else                                enterState(IDLE);
}

void loop() {
  byte b = readButtons();
  unsigned long elapsed = millis() - stateStart;

  // SW1 overrides everything except a lockout (turning the system
  // off and on again must not be a way to skip the lockout)
  if (state != LOCKOUT) {
    if (!switchOn(SW_SYSTEM) && state != SYSTEM_OFF) {
      enterState(SYSTEM_OFF);
      return;
    }
    if (switchOn(SW_SYSTEM) && state == SYSTEM_OFF) {
      enterState(IDLE);
      return;
    }
  }

  switch (state) {
    case SYSTEM_OFF:
      break;                                // keypad ignored

    case IDLE:
      if (b != 0) {
        maintenance = switchOn(SW_MAINT);   // mode fixed for this attempt
        entered[count++] = b;
        enterState(ENTER_CODE);
      }
      break;

    case ENTER_CODE:
      if (elapsed >= (switchOn(SW_ENHANCED) ? ENHANCED_ENTRY_MS : ENTRY_MS)) {
        failedAttempt();
      } else if (b != 0) {
        entered[count++] = b;
        showCode();
        if (count == CODE_LENGTH) {
          if (codeIsCorrect()) {
            failedAttempts = 0;
            if (maintenance) lockoutLevel = 0;  // only maintenance clears it
            else if (lockoutLevel > 0) lockoutLevel--;
            saveCounters();
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
      if (elapsed >= lockoutMs) {
        failedAttempts = 0;
        if (lockoutLevel < MAX_LOCKOUT_LEVEL) lockoutLevel++;  // [3] next one is longer
        saveCounters();
        enterState(IDLE);
      } else {
        int secondsLeft = (lockoutMs - elapsed + 999) / 1000;
        if (secondsLeft != lastSecondsShown) {
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
