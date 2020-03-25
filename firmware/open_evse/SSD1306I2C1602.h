/*
  $Id: SSD1306I2C1602.h,v 1.1 2016/08/11 17:55:59 ewan Exp $
  Arduino 128x32 OLED SSD1306/SH1106 16x02 library.
  SSD1306I2C1602.h (C) 2016 Ewan Parker.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

  The author may be reached at http//www.ewan.cc/ for queries.


  A SSD1306 or SH1106 controlled OLED 128x32 library to allow use as a
  common 16x02 LCD display using standard library calls.

  The assumption is that the SSD1306 driven 128x32 OLED display is connected to
  the Arduino Uno via I2C, thus.

  Arduino Uno       128x32 SSD1306 OLED
  ============      ===================
  GND          ---> GND
  5V           ---> VCC
  A5 (I2C SCL) ---> SCK
  A4 (I2C SDA) ---> SDA
*/

#ifndef SSD1306I2C1602_H
#define SSD1306I2C1602_H

// The default I2C address of the OLED control IC.
#define SSD_I2C_ADDR 0x3C

class SSD1306I2C1602 : public Print
{
public:
  // Constructor using a specified I2C address.
  SSD1306I2C1602(uint8_t address);

  // Constructor using the default I2C address.
  SSD1306I2C1602() : SSD1306I2C1602(SSD_I2C_ADDR) {}

  //// Destructor.
  //~SSD1306I2C1602();

  void begin(uint8_t cols, uint8_t rows);
  void clear();
  void home();
  void noDisplay();
  void display();
  //void noBlink();
  //void blink();
  //void noCursor();
  //void cursor();
  //void scrollDisplayLeft();
  //void scrollDisplayRight();
  //void leftToRight();
  //void rightToLeft();
  //void moveCursorLeft();
  //void moveCursorRight();
  //void autoscroll();
  //void noAutoscroll();
  //void createChar(uint8_t location, uint8_t charmap[]);
  void setCursor(uint8_t col, uint8_t row);
  void backlight(void);
  void noBacklight(void);
  void on(void);
  void off(void);
  //virtual void setBacklightPin (uint8_t value, t_backlighPol pol) { };
  //virtual void setBacklight (uint8_t value) { };
  virtual size_t write(uint8_t value);

  using Print::write;

private:
  void _setAddress(uint8_t addr);
  void _moveto(uint8_t col, uint8_t row);
  void _movetohalf(uint8_t col, uint8_t row);
  void _moveto8x8(uint8_t col, uint8_t row);

  uint8_t _address;
  uint8_t _x, _y;
};

#endif
