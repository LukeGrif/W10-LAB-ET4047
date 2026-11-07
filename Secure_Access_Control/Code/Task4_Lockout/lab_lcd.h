#ifndef LAB_LCD_H
#define LAB_LCD_H

// ------------------------------------------------------------
// One LCD object for both lab displays
//
//   * 16x2 LCD with an I2C backpack (PCF8574)
//       library: "LiquidCrystal I2C" by Frank de Brabander
//   * Adafruit RGB LCD Shield (MCP23017)
//       library: "Adafruit RGB LCD Shield Library"
//
// Install both libraries. lcd.init() finds whichever display is
// plugged in, so the same sketch works on either one.
//
// Usage (same calls as LiquidCrystal_I2C):
//   LabLCD lcd(16, 2);
//   lcd.init();  lcd.backlight();
//   lcd.setCursor(0, 1);  lcd.print("Hello");  lcd.clear();
//   Serial.println(lcd.name());   // which display was found
//
// How it tells them apart: both can sit at I2C address 0x20, so
// the address alone is not enough. The MCP23017 has registers
// that can be written and read back; the PCF8574 does not. The
// test never sets the backpack's Enable bit (P2), so the LCD on
// a backpack ignores it.
// ------------------------------------------------------------

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_RGBLCDShield.h>

class LabLCD : public Print {
public:
  enum Type : uint8_t { NONE, I2C_BACKPACK, RGB_SHIELD };

  LabLCD(uint8_t cols, uint8_t rows) : _cols(cols), _rows(rows) {}

  // Find the display and start it. Returns false if none is found;
  // the sketch still runs, it just has nothing to show.
  bool init() {
    Wire.begin();
    // The shield is always at 0x20. Backpacks use 0x20-0x27 (PCF8574)
    // or 0x38-0x3F (PCF8574A); the lab displays are at 0x20.
    for (uint8_t addr = 0x20; addr <= 0x3F; addr++) {
      if (addr == 0x28) addr = 0x38;    // skip the unused range between
      if (!present(addr)) continue;
      _addr = addr;
      if (addr == SHIELD_ADDR && isMCP23017(addr)) {
        _type = RGB_SHIELD;
        _shield.begin(_cols, _rows);
      } else {
        _type = I2C_BACKPACK;
        _backpack = new LiquidCrystal_I2C(addr, _cols, _rows);
        _backpack->init();
      }
      return true;
    }
    _type = NONE;
    return false;
  }

  // Backlight on (white on the RGB shield)
  void backlight() {
    if (_type == I2C_BACKPACK) _backpack->backlight();
    else if (_type == RGB_SHIELD) _shield.setBacklight(SHIELD_WHITE);
  }

  void noBacklight() {
    if (_type == I2C_BACKPACK) _backpack->noBacklight();
    else if (_type == RGB_SHIELD) _shield.setBacklight(0);
  }

  void clear() {
    if (_type == I2C_BACKPACK) _backpack->clear();
    else if (_type == RGB_SHIELD) _shield.clear();
  }

  void setCursor(uint8_t col, uint8_t row) {
    if (_type == I2C_BACKPACK) _backpack->setCursor(col, row);
    else if (_type == RGB_SHIELD) _shield.setCursor(col, row);
  }

  // Print calls this for every character, so lcd.print() works too
  size_t write(uint8_t c) override {
    if (_type == I2C_BACKPACK) return _backpack->write(c);
    if (_type == RGB_SHIELD) return _shield.write(c);
    return 1;                           // no display: drop it quietly
  }
  using Print::write;

  // RGB shield only: backlight colour (0x1 red ... 0x7 white, as in the
  // Adafruit examples) and the five keypad buttons (BUTTON_UP etc.).
  // On the backpack, any colour turns the backlight on and 0 turns it off.
  void setBacklight(uint8_t colour) {
    if (_type == RGB_SHIELD) _shield.setBacklight(colour);
    else if (_type == I2C_BACKPACK) colour ? _backpack->backlight() : _backpack->noBacklight();
  }
  uint8_t readButtons() { return _type == RGB_SHIELD ? _shield.readButtons() : 0; }

  Type type() const { return _type; }
  uint8_t address() const { return _addr; }
  const char* name() const {
    if (_type == I2C_BACKPACK) return "I2C backpack LCD";
    if (_type == RGB_SHIELD) return "Adafruit RGB LCD shield";
    return "no LCD found";
  }

private:
  static const uint8_t SHIELD_ADDR = 0x20;
  static const uint8_t SHIELD_WHITE = 0x7;

  static bool present(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
  }

  // Write a test value to MCP23017 register IPOLA (0x02), read it
  // back, then put it back to 0. A PCF8574 treats those bytes as
  // output pins (0x02 then 0x5A, Enable bit P2 never set) and reads
  // back at most 0x02, so it can never return 0x5A.
  static bool isMCP23017(uint8_t addr) {
    const uint8_t IPOLA = 0x02, TEST = 0x5A;
    Wire.beginTransmission(addr);
    Wire.write(IPOLA);
    Wire.write(TEST);
    if (Wire.endTransmission() != 0) return false;

    Wire.beginTransmission(addr);
    Wire.write(IPOLA);
    Wire.endTransmission();
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
    bool mcp = (Wire.read() == TEST);

    if (mcp) {
      Wire.beginTransmission(addr);
      Wire.write(IPOLA);
      Wire.write((uint8_t)0x00);
      Wire.endTransmission();
    }
    return mcp;
  }

  uint8_t _cols, _rows;
  Type _type = NONE;
  uint8_t _addr = 0;
  LiquidCrystal_I2C* _backpack = nullptr;
  Adafruit_RGBLCDShield _shield;
};

#endif // LAB_LCD_H
