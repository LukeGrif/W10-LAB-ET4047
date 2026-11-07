// ET4047 Week 10 - Secure Access Control
// Task 3: Finite State Machine
// IDLE -> ENTER_CODE -> ACCESS_GRANTED / ACCESS_DENIED -> IDLE
// Needs the lab_lcd.h and buttons.h tabs.

#include "lab_lcd.h"
#include "buttons.h"

LabLCD lcd(16, 2);

const byte LED_GREEN = 8;
const byte LED_RED   = 9;

const byte CODE_LENGTH = 3;
const byte PASSWORD[CODE_LENGTH] = {1, 3, 2};

const unsigned long GRANTED_MS = 3000;
const unsigned long DENIED_MS  = 2000;

// RGB shield backlight colours (any colour = "on" for the backpack LCD)
const byte RED = 0x1, GREEN = 0x2, WHITE = 0x7;

// ---------- FSM ----------
enum State { IDLE, ENTER_CODE, ACCESS_GRANTED, ACCESS_DENIED };
State state;
unsigned long stateStart;        // millis() when we entered the state

byte entered[CODE_LENGTH];
byte count = 0;

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

// Every state change goes through here: set outputs ONCE on entry
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
      Serial.println("IDLE");
      break;
    case ENTER_CODE:
      lcd.print("ENTERING...");
      showCode();
      Serial.println("ENTER_CODE");
      break;
    case ACCESS_GRANTED:
      digitalWrite(LED_GREEN, HIGH);
      lcd.setBacklight(GREEN);
      lcd.print("ACCESS GRANTED");
      Serial.println("ACCESS_GRANTED");
      break;
    case ACCESS_DENIED:
      digitalWrite(LED_RED, HIGH);
      lcd.setBacklight(RED);
      lcd.print("ACCESS DENIED");
      Serial.println("ACCESS_DENIED");
      break;
  }
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
  byte b = readButtons();          // 0 = no new press
  unsigned long now = millis();

  switch (state) {
    case IDLE:
      if (b != 0) {                // first digit starts a new attempt
        entered[count++] = b;
        enterState(ENTER_CODE);
      }
      break;

    case ENTER_CODE:
      if (b != 0) {
        entered[count++] = b;
        showCode();
        if (count == CODE_LENGTH) {
          enterState(codeIsCorrect() ? ACCESS_GRANTED : ACCESS_DENIED);
        }
      }
      break;

    case ACCESS_GRANTED:
      if (now - stateStart >= GRANTED_MS) enterState(IDLE);
      break;

    case ACCESS_DENIED:
      if (now - stateStart >= DENIED_MS) enterState(IDLE);
      break;
  }
}
