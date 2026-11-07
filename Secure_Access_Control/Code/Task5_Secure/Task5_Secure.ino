// ET4047 Week 10 - Secure Access Control
// Task 5: Security improvements to the Task 4 lock
//   [1] Code is masked with '*' (no shoulder surfing)
//   [2] Denied screen gives no hints (no "tries left")
//   [3] Progressive lockout: 10 s, 20 s, 40 s ... up to 160 s
//   [4] Failed attempts are kept in EEPROM, so RESET / power off
//       does not clear a lockout
//   [5] Every digit is always checked (no early exit)
// Needs the lab_lcd.h and buttons.h tabs.

#include <EEPROM.h>
#include "lab_lcd.h"
#include "buttons.h"

LabLCD lcd(16, 2);

const byte LED_GREEN = 8;
const byte LED_RED   = 9;

const byte CODE_LENGTH = 3;
const byte PASSWORD[CODE_LENGTH] = {1, 3, 2};

const byte MAX_ATTEMPTS = 3;
const unsigned long GRANTED_MS      = 3000;
const unsigned long DENIED_MS       = 2000;
const unsigned long ENTRY_MS        = 10000;
const unsigned long BASE_LOCKOUT_MS = 10000;
const byte MAX_LOCKOUT_LEVEL        = 4;      // 10 s * 2^4 = 160 s

const byte RED = 0x1, GREEN = 0x2, WHITE = 0x7;

// [4] EEPROM addresses
const int EE_MAGIC    = 0;    // 0xA5 once the EEPROM has been set up
const int EE_FAILED   = 1;    // failed attempts since last success
const int EE_LOCKOUTS = 2;    // how many lockouts in a row
const byte MAGIC = 0xA5;

enum State { IDLE, ENTER_CODE, ACCESS_GRANTED, ACCESS_DENIED, LOCKOUT };
State state;
unsigned long stateStart;
unsigned long lockoutMs;

byte entered[CODE_LENGTH];
byte count = 0;
byte failedAttempts = 0;
byte lockoutLevel = 0;
int lastSecondsShown = -1;

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
  byte differences = 0;
  for (byte i = 0; i < CODE_LENGTH; i++) {
    differences |= entered[i] ^ PASSWORD[i];
  }
  return differences == 0;
}

void enterState(State s) {
  state = s;
  stateStart = millis();
  lcd.clear();
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  lcd.setBacklight(WHITE);

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
      lcd.setBacklight(GREEN);
      lcd.print("ACCESS GRANTED");
      break;
    case ACCESS_DENIED:
      digitalWrite(LED_RED, HIGH);
      lcd.setBacklight(RED);
      lcd.print("ACCESS DENIED");                // [2] nothing else
      break;
    case LOCKOUT:
      digitalWrite(LED_RED, HIGH);
      lcd.setBacklight(RED);
      lcd.print("SYSTEM LOCKED");
      lockoutMs = BASE_LOCKOUT_MS << lockoutLevel;   // [3] 10,20,40,80,160 s
      lastSecondsShown = -1;
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

  switch (state) {
    case IDLE:
      if (b != 0) {
        entered[count++] = b;
        enterState(ENTER_CODE);
      }
      break;

    case ENTER_CODE:
      if (elapsed >= ENTRY_MS) {
        failedAttempt();
      } else if (b != 0) {
        entered[count++] = b;
        showCode();
        if (count == CODE_LENGTH) {
          if (codeIsCorrect()) {
            failedAttempts = 0;
            lockoutLevel = 0;
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
