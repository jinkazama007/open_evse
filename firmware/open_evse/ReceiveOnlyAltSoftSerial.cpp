/* An Alternative Software Serial Library
 * http://www.pjrc.com/teensy/td_libs_ReceiveOnlyAltSoftSerial.html
 * Copyright (c) 2014 PJRC.COM, LLC, Paul Stoffregen, paul@pjrc.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Revisions are now tracked on GitHub
// https://github.com/PaulStoffregen/ReceiveOnlyAltSoftSerial
//
// Version 1.2: Support Teensy 3.x
//
// Version 1.1: Improve performance in receiver code
//
// Version 1.0: Initial Release


#include "ReceiveOnlyAltSoftSerial.h"
#include "config/ReceiveOnlyAltSoftSerial_Boards.h"
#include "config/ReceiveOnlyAltSoftSerial_Timers.h"
#include "open_evse.h"

#if defined(TELEINFO_SERIAL) && (TELEINFO_SERIAL == 2)
#if defined(TELEINFO_SERIAL) && defined(AVR44) && (TELEINFO_SERIAL == 2)
#endif // TELEINFO_SERIAL

/****************************************/
/**          Initialization            **/
/****************************************/

static uint16_t ticks_per_bit=0;
bool ReceiveOnlyAltSoftSerial::timing_error=false;

static uint8_t rx_state;
static uint8_t rx_byte;
static uint8_t rx_bit = 0;
static uint16_t rx_target;
static uint16_t rx_stop_ticks=0;
static volatile uint8_t rx_buffer_head;
static volatile uint8_t rx_buffer_tail;
#define RX_BUFFER_SIZE 80
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];

#ifndef INPUT_PULLUP
#define INPUT_PULLUP INPUT
#endif

#define MAX_COUNTS_PER_BIT  6241  // 65536 / 10.5

void ReceiveOnlyAltSoftSerial::init(uint32_t cycles_per_bit)
{
	//Serial.printf("cycles_per_bit = %d\n", cycles_per_bit);
	if (cycles_per_bit < MAX_COUNTS_PER_BIT) {
		CONFIG_TIMER_NOPRESCALE();
	} else {
		cycles_per_bit /= 8;
		//Serial.printf("cycles_per_bit/8 = %d\n", cycles_per_bit);
		if (cycles_per_bit < MAX_COUNTS_PER_BIT) {
			CONFIG_TIMER_PRESCALE_8();
		} else {
#if defined(CONFIG_TIMER_PRESCALE_256)
			cycles_per_bit /= 32;
			//Serial.printf("cycles_per_bit/256 = %d\n", cycles_per_bit);
			if (cycles_per_bit < MAX_COUNTS_PER_BIT) {
				CONFIG_TIMER_PRESCALE_256();
			} else {
				return; // baud rate too low for ReceiveOnlyAltSoftSerial
			}
#elif defined(CONFIG_TIMER_PRESCALE_128)
			cycles_per_bit /= 16;
			//Serial.printf("cycles_per_bit/128 = %d\n", cycles_per_bit);
			if (cycles_per_bit < MAX_COUNTS_PER_BIT) {
				CONFIG_TIMER_PRESCALE_128();
			} else {
				return; // baud rate too low for ReceiveOnlyAltSoftSerial
			}
#else
			return; // baud rate too low for ReceiveOnlyAltSoftSerial
#endif
		}
	}
	ticks_per_bit = cycles_per_bit;
	rx_stop_ticks = cycles_per_bit * 37 / 4;
	pinMode(INPUT_CAPTURE_PIN, INPUT_PULLUP);
	rx_state = 0;
	rx_buffer_head = 0;
	rx_buffer_tail = 0;
	ENABLE_INT_INPUT_CAPTURE();
}

void ReceiveOnlyAltSoftSerial::end(void)
{
	DISABLE_INT_COMPARE_B();
	DISABLE_INT_INPUT_CAPTURE();
	flushInput();
	DISABLE_INT_COMPARE_A();
	// TODO: restore timer to original settings?
}


/****************************************/
/**           Transmission             **/
/****************************************/

void ReceiveOnlyAltSoftSerial::writeByte(uint8_t b)
{

}


ISR(COMPARE_A_INTERRUPT)
{

}

void ReceiveOnlyAltSoftSerial::flushOutput(void)
{

}


/****************************************/
/**            Reception               **/
/****************************************/

ISR(CAPTURE_INTERRUPT)
{
	uint8_t state, bit, head;
	uint16_t capture, target;
	uint16_t offset, offset_overflow;

	capture = GET_INPUT_CAPTURE();
	bit = rx_bit;
	if (bit) {
		CONFIG_CAPTURE_FALLING_EDGE();
		rx_bit = 0;
	} else {
		CONFIG_CAPTURE_RISING_EDGE();
		rx_bit = 0x80;
	}
	state = rx_state;
	if (state == 0) {
		if (!bit) {
			uint16_t end = capture + rx_stop_ticks;
			SET_COMPARE_B(end);
			ENABLE_INT_COMPARE_B();
			rx_target = capture + ticks_per_bit + ticks_per_bit/2;
			rx_state = 1;
		}
	} else {
		target = rx_target;
		offset_overflow = 65535 - ticks_per_bit;
		while (1) {
			offset = capture - target;
			if (offset > offset_overflow) break;
			rx_byte = (rx_byte >> 1) | rx_bit;
			target += ticks_per_bit;
			state++;
			if (state >= 9) {
				DISABLE_INT_COMPARE_B();
				head = rx_buffer_head + 1;
				if (head >= RX_BUFFER_SIZE) head = 0;
				if (head != rx_buffer_tail) {
					rx_buffer[head] = rx_byte;
					rx_buffer_head = head;
				}
				CONFIG_CAPTURE_FALLING_EDGE();
				rx_bit = 0;
				rx_state = 0;
				return;
			}
		}
		rx_target = target;
		rx_state = state;
	}
	//if (GET_TIMER_COUNT() - capture > ticks_per_bit) ReceiveOnlyAltSoftSerial::timing_error = true;
}

ISR(COMPARE_B_INTERRUPT)
{
	uint8_t head, state, bit;

	DISABLE_INT_COMPARE_B();
	CONFIG_CAPTURE_FALLING_EDGE();
	state = rx_state;
	bit = rx_bit ^ 0x80;
	while (state < 9) {
		rx_byte = (rx_byte >> 1) | bit;
		state++;
	}
	head = rx_buffer_head + 1;
	if (head >= RX_BUFFER_SIZE) head = 0;
	if (head != rx_buffer_tail) {
		rx_buffer[head] = rx_byte;
		rx_buffer_head = head;
	}
	rx_state = 0;
	CONFIG_CAPTURE_FALLING_EDGE();
	rx_bit = 0;
}



int ReceiveOnlyAltSoftSerial::read(void)
{
	uint8_t head, tail, out;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head == tail) return -1;
	if (++tail >= RX_BUFFER_SIZE) tail = 0;
	out = rx_buffer[tail];
	rx_buffer_tail = tail;
	return out;
}

int ReceiveOnlyAltSoftSerial::peek(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head == tail) return -1;
	if (++tail >= RX_BUFFER_SIZE) tail = 0;
	return rx_buffer[tail];
}

int ReceiveOnlyAltSoftSerial::available(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head >= tail) return head - tail;
	return RX_BUFFER_SIZE + head - tail;
}

int ReceiveOnlyAltSoftSerial::availableForWrite(void)
{ 

};

void ReceiveOnlyAltSoftSerial::flushInput(void)
{
	rx_buffer_head = rx_buffer_tail;
}


#ifdef ALTSS_USE_FTM0
void ftm0_isr(void)
{
	uint32_t flags = FTM0_STATUS;
	FTM0_STATUS = 0;
	if (flags & (1<<0) && (FTM0_C0SC & 0x40)) altss_compare_b_interrupt();
	if (flags & (1<<5)) altss_capture_interrupt();
	if (flags & (1<<6) && (FTM0_C6SC & 0x40)) altss_compare_a_interrupt();
}
#endif
#endif
