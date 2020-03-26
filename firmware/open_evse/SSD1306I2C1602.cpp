/*
  $Id: SSD1306I2C1602.cpp,v 1.4 2016/09/03 14:51:16 ewan Exp $
  Arduino 128x32 OLED SSD1306/SH1106 16x02 library.
  SSD1306I2C1602.cpp (C) 2016 Ewan Parker.

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
*/

#include <Arduino.h>
#include "SSD1306I2C1602.h"
#include "Wire.h"
#include "8x16_pages.h"
#include <avr/pgmspace.h>
#include "open_evse.h"

// Fundamental commands.
#define SSD_CMD_SET_NORMAL_DISPLAY 0xA6
#define SSD_CMD_SET_INVERSE_DISPLAY 0xA7
#define SSD_CMD_SET_DISPLAY_OFF 0xAE
#define SSD_CMD_SET_DISPLAY_ON 0xAF

// Address setting commands.
#define SSD_CMD_SET_LOWER_COL_START_ADDR 0x00
#define SSD_CMD_SET_HIGHER_COL_START_ADDR 0x10
#define SSD_CMD_SET_PAGE_START_ADDR 0xB0

// Hardware configuration.
#define SSD_CMD_SET_MULTIPLEX_RATIO 0xA8
#define SSD_CMD_SET_COM_PINS 0xDA

// Enable the onboard charge pump.
#define SSD_CMD_CHARGE_PUMP_SETTING 0x8D

// Constructor.  We need the I2C address, but we can use a default.
SSD1306I2C1602::SSD1306I2C1602(uint8_t addr)
{
  _setAddress(addr);
}

//// Destructor
//SSD1306I2C1602::~SSD1306I2C1602()
//{
//  Wire.end();
//}

// Public methods.

// Set up the display.  Cols and rows are not used.
void SSD1306I2C1602::begin(uint8_t cols, uint8_t rows)
{
  Wire.begin();
  Wire.beginTransmission(_address);
  #if defined(I2COLED_ROTATE)
    Wire.write(0); Wire.write(0x3C); Wire.write(0xA1); Wire.write(0xC8);
  #endif
  Wire.write(0); Wire.write(SSD_CMD_SET_DISPLAY_OFF);
  Wire.write(0); Wire.write(SSD_CMD_SET_PAGE_START_ADDR | 0x00);
  Wire.write(0); Wire.write(SSD_CMD_SET_HIGHER_COL_START_ADDR | 0b0000);
  Wire.write(0); Wire.write(SSD_CMD_SET_LOWER_COL_START_ADDR | 0b0001);
  Wire.write(0); Wire.write(SSD_CMD_SET_MULTIPLEX_RATIO);
  #if defined(I2COLED) && (I2COLED == 12864)
    Wire.write(63);
  #else
    Wire.write(31);
  #endif
  Wire.write(0); Wire.write(SSD_CMD_SET_COM_PINS);
  #if defined(I2COLED) && (I2COLED == 12864)
    Wire.write(0x12);
  #else
    Wire.write(2);
  #endif  
  Wire.write(0); Wire.write(SSD_CMD_CHARGE_PUMP_SETTING); Wire.write(0x14);
  Wire.endTransmission();
  delay(100); // Wait 100ms.
  clear();
  Wire.beginTransmission(_address);
  Wire.write(0); Wire.write(SSD_CMD_SET_DISPLAY_ON);
  Wire.endTransmission();
}

// Clear the display.
void SSD1306I2C1602::clear()
{
  #if defined(I2COLED) && (I2COLED == 12864)
    for (uint8_t p = 0; p < 8; p++)
  #else
    for (uint8_t p = 0; p < 4; p++)
  #endif
  {
    Wire.beginTransmission(_address);
    Wire.write(0); Wire.write(SSD_CMD_SET_PAGE_START_ADDR | p);
    Wire.write(0); Wire.write(SSD_CMD_SET_HIGHER_COL_START_ADDR | 0);
    Wire.write(0); Wire.write(SSD_CMD_SET_LOWER_COL_START_ADDR | 0);
    Wire.endTransmission(false);
    for (uint8_t x1 = 0; x1 <= 5; x1++)
    {
      Wire.beginTransmission(_address);
      Wire.write(0x40);
      for (uint8_t x2 = 0; x2 <= 26; x2++)
      {
        Wire.write(0x00);
      }
      Wire.endTransmission(false);
    }
  }
  home();
}

// Move to the top-left.
void SSD1306I2C1602::home()
{
  setCursor(0, 0);
}

// Turn off the display.
void SSD1306I2C1602::noDisplay()
{
  Wire.beginTransmission(_address);
  Wire.write(0); Wire.write(SSD_CMD_SET_DISPLAY_OFF);
  Wire.endTransmission();
}

// Turn on the display.
void SSD1306I2C1602::display()
{
  Wire.beginTransmission(_address);
  Wire.write(0); Wire.write(SSD_CMD_SET_DISPLAY_ON);
  Wire.endTransmission();
}

// Move position to column (x) and row (y).
void SSD1306I2C1602::setCursor(uint8_t col, uint8_t row)
{
  _x = col; _y = row;
}

// Turn on and off the backlight (not applicable for OLED displays).
void SSD1306I2C1602::backlight(void) {}
void SSD1306I2C1602::noBacklight(void) {}

// Turn the module on.
void SSD1306I2C1602::on (void)
{
  backlight();
  display();
}

// Turn the module off.
void SSD1306I2C1602::off (void)
{
  noDisplay();
  noBacklight();
}

// Write a character onto the OLED display and return the number of characters
// actually written.
size_t SSD1306I2C1602::write(uint8_t c)
{
  if (_x < 16)
  {
    int i = c<<4;
    _moveto(_x, _y);
    Wire.beginTransmission(_address);
    Wire.write(0x40);
    for (short j = 0; j < 8; j++) Wire.write(pgm_read_byte(&bmap8x16[i++]));
    Wire.endTransmission(false);
    _movetohalf(_x, _y);
    Wire.beginTransmission(_address);
    Wire.write(0x40);
    for (short j = 0; j < 8; j++) Wire.write(pgm_read_byte(&bmap8x16[i++]));
    Wire.endTransmission();
    ++_x;
  }
  return 1;
}

// Private methods.

// Set the I2C address to use for this display.
void SSD1306I2C1602::_setAddress(uint8_t addr)
{
  _address = addr;
}

// Move to an 8x8 co-ordinate.
void SSD1306I2C1602::_moveto8x8(uint8_t x, uint8_t y)
{
  uint8_t page = y;
  uint8_t col = x << 3;
  Wire.beginTransmission(_address);
  Wire.write(0); Wire.write(SSD_CMD_SET_PAGE_START_ADDR | page);
  Wire.write(0); Wire.write(SSD_CMD_SET_HIGHER_COL_START_ADDR | (col >> 4));
  Wire.write(0); Wire.write(SSD_CMD_SET_LOWER_COL_START_ADDR | (col & 0x0F));
  Wire.endTransmission();
}

// Move to a 8x16 (i.e. display) co-ordinate.
void SSD1306I2C1602::_moveto(uint8_t x, uint8_t y)
{
  _moveto8x8(x, y << 1);
}

// Move one 8x8 block down from a 8x16 co-ordinate.
void SSD1306I2C1602::_movetohalf(uint8_t x, uint8_t y)
{
  _moveto8x8(x, (y << 1) + 1);
}
