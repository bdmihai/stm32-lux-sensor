/*_____________________________________________________________________________
 │                                                                            |
 │ COPYRIGHT (C) 2022 Mihai Baneu                                             |
 │                                                                            |
 | Permission is hereby  granted,  free of charge,  to any person obtaining a |
 | copy of this software and associated documentation files (the "Software"), |
 | to deal in the Software without restriction,  including without limitation |
 | the rights to  use, copy, modify, merge, publish, distribute,  sublicense, |
 | and/or sell copies  of  the Software, and to permit  persons to  whom  the |
 | Software is furnished to do so, subject to the following conditions:       |
 |                                                                            |
 | The above  copyright notice  and this permission notice  shall be included |
 | in all copies or substantial portions of the Software.                     |
 |                                                                            |
 | THE SOFTWARE IS PROVIDED  "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS |
 | OR   IMPLIED,   INCLUDING   BUT   NOT   LIMITED   TO   THE  WARRANTIES  OF |
 | MERCHANTABILITY,  FITNESS FOR  A  PARTICULAR  PURPOSE AND NONINFRINGEMENT. |
 | IN NO  EVENT SHALL  THE AUTHORS  OR  COPYRIGHT  HOLDERS  BE LIABLE FOR ANY |
 | CLAIM, DAMAGES OR OTHER LIABILITY,  WHETHER IN AN ACTION OF CONTRACT, TORT |
 | OR OTHERWISE, ARISING FROM,  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR  |
 | THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                 |
 |____________________________________________________________________________|
 |                                                                            |
 |  Author: Mihai Baneu                           Last modified: 20.Oct.2022  |
 |                                                                            |
 |___________________________________________________________________________*/

#include "stm32f4xx.h"
#include "stm32rtos.h"
#include "stdlib.h"
#include "string.h"
#include "queue.h"
#include "gpio.h"
#include "system.h"
#include "tft.h"
#include "spi.h"
#include "st7735.h"
#include "printf.h"

extern const uint8_t u8x8_font_8x13B_1x2_f[];

 /* Queue used to communicate TFT update messages. */
QueueHandle_t tft_queue = NULL;

void tft_init()
{
    tft_queue = xQueueCreate(6, sizeof(tft_event_t));
}

void tft_run(void *params)
{
    (void)params;

    st7735_hw_control_t hw = {
        .res_high = gpio_tft_res_high,
        .res_low  = gpio_tft_res_low,
        .dc_high  = gpio_tft_dc_high,
        .dc_low   = gpio_tft_dc_low,
        .data_wr  = spi_write,
        .data_rd  = spi_read,
        .delay_us = delay_us
    };

    st7735_init(hw);

    /* perform a HW reset */
    st7735_hardware_reset();
    st7735_sleep_out_and_booster_on();
    delay_us(10000);
    st7735_normal_mode();

    /* configure the display */
    st7735_display_inversion_off();
    st7735_interface_pixel_format(ST7735_16_PIXEL);
    st7735_display_on();
    st7735_memory_data_access_control(0, 1, 0, 0, 0, 0);
    st7735_column_address_set(0, 128-1);
    st7735_row_address_set(0, 160-1);

    /* prepare the background */
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    st7735_draw_rectangle(10, 10, 150, 120, st7735_rgb_red, st7735_rgb_yellow);

    /* process events */
    for (;;) {
        tft_event_t tft_event;
        char txt[16] = { 0 };

        if (xQueueReceive(tft_queue, &tft_event, portMAX_DELAY) == pdPASS) {
            switch (tft_event.type) {
                case tft_event_lux:
                    sprintf(txt, "LUX: %6d", tft_event.lux);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 4*8, 7*8, st7735_rgb_black, st7735_rgb_yellow, txt);
                    break;
                
                default:
                    break;
            }
        }
    }
}
