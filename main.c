/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "pico/stdlib.h"

#include "preloaded.h"

#define BUTTON_R 5
#define BUTTON_ZR 6
#define BUTTON_L 7
#define BUTTON_ZL 8

#define BUTTON_SELECT 9
#define BUTTON_START 10
#define BUTTON_MODE 11

#define BUTTON_A 12
#define BUTTON_B 13
#define BUTTON_X 14
#define BUTTON_Y 15

#define LEFT_BUTTON 16
#define DOWN_BUTTON 17
#define UP_BUTTON 18
#define RIGHT_BUTTON 19


typedef struct TU_ATTR_PACKED{
	int8_t x;
	int8_t y;
	int8_t z;
	int8_t rz;
	uint8_t hat;
	uint16_t buttons;
}custom_hid_gamepad_report_t;

void led_blinking_task(void);
void hid_task(void);

void setupButtons(){
	for(int i = 0; i < 15; i++){
	  gpio_init(BUTTON_R + i);
	  gpio_set_dir(BUTTON_R + i, GPIO_IN);
	  gpio_pull_up(BUTTON_R + i);
	}
}

uint32_t getButtonsPressed(){
	return !gpio_get(LEFT_BUTTON)
	       | (!gpio_get(DOWN_BUTTON) << 1)
       		| (!gpio_get(UP_BUTTON) << 2)
 		| (!gpio_get(RIGHT_BUTTON) << 3)		
 		| (!gpio_get(BUTTON_A) << 4)		
 		| (!gpio_get(BUTTON_B) << 5)		
 		| (!gpio_get(BUTTON_X) << 6)		
 		| (!gpio_get(BUTTON_Y) << 7)
 		| (!gpio_get(BUTTON_SELECT) << 8)		
 		| (!gpio_get(BUTTON_START) << 9)		
 		| (!gpio_get(BUTTON_MODE) << 10)
 		| (!gpio_get(BUTTON_R) << 11)
 		| (!gpio_get(BUTTON_ZR) << 12)
 		| (!gpio_get(BUTTON_L) << 13)
 		| (!gpio_get(BUTTON_ZL) << 14);
}

/*------------- MAIN -------------*/
int main(void)
{
  board_init();
  tusb_init();
  stdio_init_all();
  setupButtons();

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();

    hid_task();
  }
}


//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_gamepad_key = false;

      custom_hid_gamepad_report_t report =
      {
        .x = 0, .y = 0, .z = 0, .rz = 0, .hat = 0, .buttons = 0
      };

      if ( btn )
      {
        report.hat = btn & 1? GAMEPAD_HAT_LEFT : report.hat;
	report.hat = btn & 2? GAMEPAD_HAT_DOWN : report.hat;
	report.hat = btn & 4? GAMEPAD_HAT_UP : report.hat;
	report.hat = btn & 8? GAMEPAD_HAT_RIGHT : report.hat;

	//new inputs VV
        report.hat = ((btn & 1) && (btn & 2))? GAMEPAD_HAT_DOWN_LEFT : report.hat;
        report.hat = ((btn & 8) && (btn & 2))? GAMEPAD_HAT_DOWN_RIGHT : report.hat;
        report.hat = ((btn & 1) && (btn & 4))? GAMEPAD_HAT_UP_LEFT : report.hat;
        report.hat = ((btn & 8) && (btn & 4))? GAMEPAD_HAT_UP_RIGHT : report.hat;
	//end of new inputs ^^
	     	
        report.buttons |= btn & (1 << 4)? GAMEPAD_BUTTON_A : 0;
        report.buttons |= btn & (1 << 5)? GAMEPAD_BUTTON_B : 0;
        report.buttons |= btn & (1 << 6)? GAMEPAD_BUTTON_X : 0;
        report.buttons |= btn & (1 << 7)? GAMEPAD_BUTTON_Y : 0;

        report.buttons |= btn & (1 << 8)? GAMEPAD_BUTTON_SELECT : 0;
        report.buttons |= btn & (1 << 9)? GAMEPAD_BUTTON_START : 0;
        report.buttons |= btn & (1 << 10)? GAMEPAD_BUTTON_MODE : 0;

        report.buttons |= btn & (1 << 11)? GAMEPAD_BUTTON_TR : 0;
        report.buttons |= btn & (1 << 12)? GAMEPAD_BUTTON_TR2 : 0;
        report.buttons |= btn & (1 << 13)? GAMEPAD_BUTTON_TL : 0;
        report.buttons |= btn & (1 << 14)? GAMEPAD_BUTTON_TL2 : 0;
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));

        has_gamepad_key = true;
      }else
      {
        report.hat = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;
        if (has_gamepad_key) tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
        has_gamepad_key = false;
      }
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 5;//10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = getButtonsPressed();//!gpio_get(LEFT_BUTTON);//board_button_read();

  // Remote wakeup
  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_GAMEPAD, btn);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;
  (void) report;
}
