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
 |  Author: Mihai Baneu                           Last modified: 05.Oct.2022  |
 |                                                                            |
 |___________________________________________________________________________*/

#include "stm32f4xx.h"
#include "stm32rtos.h"
#include "task.h"
#include "queue.h"
#include "system.h"
#include "gpio.h"
#include "isr.h"
#include "i2c.h"
#include "spi.h"
#include "led.h"
#include "tft.h"
#include "tsl2561.h"

static void lux_read(void *pvParameters)
{
    (void)pvParameters;
    uint32_t lux;
    uint16_t broadband;
    uint16_t ir;

    tsl2561_hw_control_t hw_control = {
        .data_rd = i2c_read,
        .data_wr = i2c_write,
        .address = 0x39
    };

    tsl2561_init(hw_control);
    
    for (;;) {
        tft_event_t tft_event;

        // first pass
        tsl2561_config(TSL2561_INTEGRATIONTIME_402MS, TSL2561_GAIN_16X);
        tsl2561_start();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        tsl2561_read(&broadband, &ir);
        tsl2561_stop();
        lux = tsl2561_calculate_lux(broadband, ir, TSL2561_INTEGRATIONTIME_402MS, TSL2561_GAIN_16X);

        // second pass
        if ((lux == 65536) || (lux == 0)) {
            tsl2561_config(TSL2561_INTEGRATIONTIME_402MS, TSL2561_GAIN_1X);
            tsl2561_start();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            tsl2561_read(&broadband, &ir);
            tsl2561_stop();
            lux = tsl2561_calculate_lux(broadband, ir, TSL2561_INTEGRATIONTIME_402MS, TSL2561_GAIN_1X);
        } else {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        tft_event.type = tft_event_lux;
        tft_event.lux = lux;
        xQueueSendToBack(tft_queue, &tft_event, (TickType_t) 1);
    }
}

int main(void)
{
    /* initialize the system */
    system_init();

    /* initialize the gpio */
    gpio_init();

    /* initialize the interupt service routines */
    isr_init();

    /* initialize the i2c interface */
    i2c_init();

    /* initialize the spi interface */
    spi_init();

    /* init led handler */
    led_init();

    /* init lcd display */
    tft_init();

    /* create the tasks specific to this application. */
    xTaskCreate(led_run,  "led",       configMINIMAL_STACK_SIZE,     NULL, 3, NULL);
    xTaskCreate(tft_run,  "tft",       configMINIMAL_STACK_SIZE*2,   NULL, 2, NULL);
    xTaskCreate(lux_read, "lux_read",  configMINIMAL_STACK_SIZE,     NULL, 2, NULL);

    /* start the scheduler. */
    vTaskStartScheduler();

    /* should never get here ... */
    blink(10);
    return 0;
}
