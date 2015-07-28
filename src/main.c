/**
 * @file   main.c
 * @brief  Application main.
 *
 * @author Cerevo Inc.
 */

/*
Copyright 2015 Cerevo Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "PMU_TZ10xx.h"
#include "RTC_TZ10xx.h"
#include "GPIO_TZ10xx.h"

#include "TZ01_system.h"
#include "TZ01_console.h"
#include "TZ01_battery_charger.h"
#include "utils.h"
#include "nixie_digit.h"

static uint16_t nx_wait_c_on  = TICK_WAIT_NX_WAIT_C_ON;
static uint16_t nx_wait_a_on  = TICK_WAIT_NX_WAIT_A_ON;
static uint16_t nx_wait_a_off = TICK_WAIT_NX_WAIT_A_OFF;
static uint16_t nx_wait_c_f   = TICK_WAIT_NX_WAIT_C_F;

extern TZ10XX_DRIVER_PMU  Driver_PMU;
extern TZ10XX_DRIVER_RTC Driver_RTC;
extern TZ10XX_DRIVER_GPIO Driver_GPIO;

static int input_4digit_dec(void)
{
    int i;
    int digit = 0;
    int input = -1;
    uint8_t val[] = { '\0', '\0', '\0', '\0', '\0', '\0'};
    uint8_t key;
    
    for (;;) {
        if (TZ01_console_getc(&key)) {
            if ((key >= '0') && (key <= '9')) {
                //Decimal
                if (digit < 4) {
                    digit++;
                    TZ01_console_putc(key);
                    val[digit] = key;
                }
            } else if (key == '\x08') {
                //Backspace
                if (digit > 0) {
                    TZ01_console_putc('\x08');
                    TZ01_console_putc('\x20');
                    TZ01_console_putc('\x08');
                    val[digit] = '\0';
                    digit--;
                }
            } else if (key == '\r') {
                //Confirm
                if (digit > 0) {
                    //文字列→数値
                    input = atoi(&val[1]);
                }
                TZ01_console_puts("\r\n");
                break;
            }
        }
        Usleep(10000);
    }
    return input;
}

uint8_t msg[80];
static void config(void)
{
    //int wait_c_on, wait_a_on, wait_a_off, wait_c_f;
    int us_1round, per_on;
    int hh_mm, hh, mm;
    uint8_t hour, min;
    uint8_t key;
    bool confirm = false;

    /* ダイナミック点灯 時間設定 */
    while (confirm == false) {
        /*
        sprintf(msg, "Wait Cathode ON(us)[%d]:", nx_wait_c_on);
        TZ01_console_puts(msg);
        wait_c_on = input_4digit_dec();
        
        sprintf(msg, "Wait Anode ON(us)[%d]:", nx_wait_a_on);
        TZ01_console_puts(msg);
        wait_a_on = input_4digit_dec();
        
        sprintf(msg, "Wait Anode OFF(us)[%d]:", nx_wait_a_off);
        TZ01_console_puts(msg);
        wait_a_off = input_4digit_dec();
        
        sprintf(msg, "Wait Cathode ALL '1'(us)[%d]:", nx_wait_c_f);
        TZ01_console_puts(msg);
        wait_c_f = input_4digit_dec();
        */
        
        sprintf(msg, "One round time(us)[%d]:", nx_wait_c_on + nx_wait_a_on + nx_wait_a_off + nx_wait_c_f);
        TZ01_console_puts(msg);
        us_1round = input_4digit_dec();
        if (us_1round < 0) {
            us_1round = nx_wait_c_on + nx_wait_a_on + nx_wait_a_off + nx_wait_c_f;
        }
        
        
        sprintf(msg, "On time(%%)[%d]:", (int)((float)nx_wait_a_off / (float)(nx_wait_c_on + nx_wait_a_on + nx_wait_a_off + nx_wait_c_f) * 100));
        TZ01_console_puts(msg);
        per_on = input_4digit_dec();
        if (per_on < 0) {
            per_on = (int)((float)nx_wait_a_off / (float)(nx_wait_c_on + nx_wait_a_on + nx_wait_a_off + nx_wait_c_f) * 100);
        }
        
        TZ01_console_puts("OK? y/n");
        for (;;) {
            if (TZ01_console_getc(&key)) {
                if ((key == 'y') || (key == 'Y')) {
                    TZ01_console_puts("\r\n");
                    confirm = true;
                    break;
                } else if ((key == 'n') || (key == 'N')) {
                    TZ01_console_puts("\r\n");
                    confirm = false;
                    break;
                }
            } else {
                Usleep(10000);
            }
        }
    }
    
    nx_wait_c_on  = (int)((float)us_1round * ((100 - (float)per_on) / 100));
    nx_wait_a_off = (int)((float)us_1round * ((float)per_on / 100));
    nx_wait_a_on = 0;
    nx_wait_c_f = 0;
    sprintf(msg, "wait_c_on=%d, wait_a_off=%d, wait_a_on=%d, wait_c_f=%d\r\n", 
        nx_wait_c_on, nx_wait_a_off, nx_wait_a_on, nx_wait_c_f);
    TZ01_console_puts(msg);
/*
    if (wait_c_on >= 0) {
        nx_wait_c_on = wait_c_on;
    }
    if (wait_a_on >= 0) {
        nx_wait_a_on = wait_a_on;
    }
    if (wait_a_off >= 0) { 
        nx_wait_a_off = wait_a_off;
    }
    if (wait_c_f >= 0) {
        nx_wait_c_f = wait_c_f;
    }
*/    
    /* 時刻入力 */
    confirm = false;
    while (confirm == false) {
        TZ01_console_puts("Time(HHMM):");
        hh_mm = input_4digit_dec();
        
        hh = hh_mm / 100;
        mm = hh_mm % 100;
        
        if ((hh < 0) || (hh > 23)) {
            continue;
        }
        if ((mm < 0) || (mm > 59)) {
            continue;
        }
        
        TZ01_console_puts("OK? y/n");
        for (;;) {
            if (TZ01_console_getc(&key)) {
                if ((key == 'y') || (key == 'Y')) {
                    TZ01_console_puts("\r\n");
                    confirm = true;
                    break;
                } else if ((key == 'n') || (key == 'N')) {
                    TZ01_console_puts("\r\n");
                    confirm = false;
                    break;
                }
            } else {
                Usleep(10000);
            }
        }
    }
    hour = (uint8_t)hh;
    min  = (uint8_t)mm;
    //時刻設定
    RTC_TIME now = {
        00, min, hour, 8, 6, 15, 1 /* 15-06-08(Mon) HH:MM:00 */
    };
    Driver_RTC.SetTime(&now); /* Set current date and time */    

}


int main(void)
{
    uint8_t nx1, nx2, nx3, nx4;
    uint8_t *bcreg;
    RTC_TIME now= {
        55, 33, 12, 8, 6, 15, 1 /* 15-06-08(Mon) 12:33:45 */
    };
    
    /* Initialize */
    TZ01_system_init();
    TZ01_console_init();
    
    TZ01_console_puts("Initialize\r\n");
    
    TZ01_battery_charger_init(true);
    TZ01_battery_charger_set_configs();
    bcreg = TZ01_battery_charger_get_configs();
    sprintf(msg, "Battery charger: Done. Register#2=%02x\r\n", bcreg[1]);
    TZ01_console_puts(msg);

    Driver_PMU.StartClockSource(PMU_CLOCK_SOURCE_OSC32K);
    Driver_PMU.SelectClockSource(PMU_CSM_RTC, PMU_CLOCK_SOURCE_32K);
    Driver_PMU.SetPrescaler(PMU_CD_RTC, 1);
    Driver_RTC.Initialize();
    Driver_RTC.SetTime(&now); /* Set current date and time */
    TZ01_console_puts("RTC: Done.\r\n");

    //TZ01_console_puts("Configuration...\r\n");
    //config();
    
    NixieDigit_init(nx_wait_c_on, nx_wait_a_on, nx_wait_a_off, nx_wait_c_f);
    if (NixieDigit_start() != 0) {
        TZ01_console_puts("NixieDigit_start() failed.\r\n");
        return 1;
    }
    Driver_GPIO.WritePin(TZ01_SYSTEM_PWSW_PORT_LED, 0);
    TZ01_console_puts("Done\r\nPress 'c' key into configure mode.\r\n");
    
    TZ01_system_tick_start(USRTICK_NO_GPIO_INTERVAL, 100);
    for (;;) {
        if (TZ01_system_run() == false) {
            /* Power off operation OR Low voltage detected */
            break;
        }
        
        if (TZ01_system_tick_check_timeout(USRTICK_NO_GPIO_INTERVAL)) {
            TZ01_system_tick_start(USRTICK_NO_GPIO_INTERVAL, 500);
            Driver_RTC.GetTime(&now);
            /*
            sprintf(
                msg, "%02d/%02d/%02d %02d:%02d:%02d\r\n",
                now.year, now.mon, now.mday, now.hour, now.min, now.sec
            );
            TZ01_console_puts(msg);
            */
            nx1 = now.hour / 10;
            nx2 = now.hour % 10;
            nx3 = now.min  / 10;
            nx4 = now.min  % 10;
            
            NixieDigit_set_nx1(nx1, false);
            NixieDigit_set_nx2(nx2, (now.sec % 2) ? true : false);
            NixieDigit_set_nx3(nx3, false);
            NixieDigit_set_nx4(nx4, false);
        } else {
            uint8_t k;
            if (TZ01_console_getc(&k)) {
                if (k == 'c') {
                    NixieDigit_stop();
                    config();
                    NixieDigit_reconfig(nx_wait_c_on, nx_wait_a_on, nx_wait_a_off, nx_wait_c_f);
                    NixieDigit_start();
                }
            }
        }
        
        NixieDigit_run();
    }
    
    NixieDigit_stop();
    return 0;
}
