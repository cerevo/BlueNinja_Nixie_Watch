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
#include "ble_cts.h"

static uint16_t nx_wait_c_on  = TICK_WAIT_NX_WAIT_C_ON;
static uint16_t nx_wait_a_on  = TICK_WAIT_NX_WAIT_A_ON;
static uint16_t nx_wait_a_off = TICK_WAIT_NX_WAIT_A_OFF;
static uint16_t nx_wait_c_f   = TICK_WAIT_NX_WAIT_C_F;

extern TZ10XX_DRIVER_PMU  Driver_PMU;
extern TZ10XX_DRIVER_RTC Driver_RTC;
extern TZ10XX_DRIVER_GPIO Driver_GPIO;

#define CNT_DISPLAY (1000)
#define CNT_GETTIME (50)

static int cnt_display = CNT_DISPLAY;
static int cnt_gettime = 0;

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
    int us_1round, per_on;
    int hh_mm, hh, mm;
    uint8_t hour, min;
    uint8_t key;
    bool confirm = false;

    /* ダイナミック点灯 時間設定 */
    while (confirm == false) {
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


typedef struct {
    uint8_t     on      :1;
    uint8_t     hist    :3;
    uint16_t    on_cnt;
}   BtnStat;

static void sw_init(BtnStat *btn)
{
    btn->on = 0;
    btn->hist = 0x07;
    btn->on_cnt = 0;
}

static void sw_update(BtnStat *btn, int gpio)
{
    uint32_t val;
    
    btn->hist = (btn->hist << 1) & 0x07;    //履歴を更新
    if (Driver_GPIO.ReadPin(gpio, &val) == GPIO_ERROR) {
        return;
    }
    btn->hist |= (val & 0x01);
    
    if (btn->hist == 0x07) {
        //OFF
        btn->on = 0;
        btn->on_cnt = 0;
    }
    if (btn->hist == 0x00) {
        //ON
        btn->on = 1;
        btn->on_cnt++;
    }
}

static uint8_t sw_state(BtnStat *btn)
{
    if (btn->on == 1) {
        if (btn->on_cnt < 200) {
            return 1;   //ON
        } else {
            return 2;   //HOLD
        }
    } else {
        return 0;       //OFF
    }
}

typedef enum {
    APP_STAT_NONE,
    APP_STAT_DISP,
    APP_STAT_BLE,
}   APP_STAT;

APP_STAT app_stat = APP_STAT_NONE;

static void state_none(int sw)
{
}

/**
 * DISPLAY
 */
static void state_display(int sw)
{
    RTC_TIME now;
    uint8_t nx1, nx2, nx3, nx4;
    uint8_t k;
    /* スイッチイベント */
    if (sw == 1) {
        /* PUSH */
        TZ01_console_puts("Pushed!\r\n");
        cnt_display = CNT_DISPLAY;
    }
    if (sw == 2) {
        /* HOLD */
        TZ01_console_puts("Hold!\r\n");
        NixieDigit_stop();
        //reload
        cnt_display = CNT_DISPLAY;
        cnt_gettime = CNT_GETTIME;
        //BLE_CTSへ遷移
        ble_cts_start();
        app_stat = APP_STAT_BLE;
    }
    
    /* 10msイベントタイムアウト */
    if (TZ01_system_tick_check_timeout(USRTICK_NO_DISP_EVT_INTERVAL)) {
        TZ01_system_tick_start(USRTICK_NO_DISP_EVT_INTERVAL, 10);
        if (cnt_display-- <= 0) {
            //SLEEP
            NixieDigit_stop();
            Driver_PMU.SetPowerMode(PMU_POWER_MODE_SLEEPDEEP2);
            TZ01_console_puts("!Wakeup!\r\n");
            NixieDigit_start();
            //Reload counter.
            cnt_display = CNT_DISPLAY;
            cnt_gettime = 0;
            //Timer start
            TZ01_system_tick_start(USRTICK_NO_GPIO_INTERVAL, 10);
        }
        
        if (cnt_gettime-- <= 0) {
            //Update clock.
            Driver_RTC.GetTime(&now);
            nx1 = now.hour / 10;
            nx2 = now.hour % 10;
            nx3 = now.min  / 10;
            nx4 = now.min  % 10;
            NixieDigit_set_nx1(nx1, false);
            NixieDigit_set_nx2(nx2, (now.sec % 2) ? true : false);
            NixieDigit_set_nx3(nx3, false);
            NixieDigit_set_nx4(nx4, false);
            //Reload counter
            cnt_gettime = CNT_GETTIME;
        }
    }
    
    if (TZ01_console_getc(&k)) {
        if (k == 'c') {
            NixieDigit_stop();
            config();
            NixieDigit_reconfig(nx_wait_c_on, nx_wait_a_on, nx_wait_a_off, nx_wait_c_f);
            NixieDigit_start();
        }
    }
    NixieDigit_run();
}

/**
 * BLE
 */
static void state_ble_cts(int sw)
{
    int ret;
    
    ret = ble_cts_run(sw);
    if (ret == 1) {
        NixieDigit_start();         //表示再開
        app_stat = APP_STAT_DISP;   //DISPLAYへ遷移
    }
}

static void (*state_func[])(int sw) = {
    state_none,
    state_display,
    state_ble_cts
};

int main(void)
{
    uint8_t *bcreg;
    RTC_TIME now = {
        55, 33, 12, 8, 6, 15, 1 /* 15-06-08(Mon) 12:33:45 */
    };
    
    BtnStat sw2_stat;
    
    /* Initialize */
    if (ble_cts_init() != 0) {
        return 1;
    }
    TZ01_system_init();
    TZ01_console_init();
    
    TZ01_console_puts("Initialize\r\n");
    
    TZ01_battery_charger_init(true);
    TZ01_battery_charger_set_configs();
    bcreg = TZ01_battery_charger_get_configs();
    sprintf(msg, "Battery charger: Done. Register#2=%02x\r\n", bcreg[1]);
    TZ01_console_puts(msg);

    Driver_PMU.StandbyInputBuffer(PMU_IO_FUNC_GPIO_6, 0);
    Driver_GPIO.Configure(6, GPIO_DIRECTION_INPUT_HI_Z, GPIO_EVENT_DISABLE, NULL);
    if (Driver_PMU.ConfigureWakeup(PMU_WAKEUP_FACTOR_GPIO_6, PMU_WAKEUP_EVENT_EDGE_NEG) != PMU_OK) {
        return 1;
    }
    if (Driver_PMU.EnableWakeup(PMU_WAKEUP_FACTOR_GPIO_6, true) != PMU_OK) {
        return 1;
    }
    TZ01_console_puts("Wakeup: Done.\r\n");
    
    Driver_PMU.StartClockSource(PMU_CLOCK_SOURCE_OSC32K);
    Driver_PMU.SelectClockSource(PMU_CSM_RTC, PMU_CLOCK_SOURCE_32K);
    Driver_PMU.SetPrescaler(PMU_CD_RTC, 1);
    Driver_RTC.Initialize();
    Driver_RTC.SetTime(&now); /* Set current date and time */
    TZ01_console_puts("RTC: Done.\r\n");
    
    NixieDigit_init(nx_wait_c_on, nx_wait_a_on, nx_wait_a_off, nx_wait_c_f);
    if (NixieDigit_start() != 0) {
        TZ01_console_puts("NixieDigit_start() failed.\r\n");
        return 1;
    }
    Driver_GPIO.WritePin(TZ01_SYSTEM_PWSW_PORT_LED, 0);
    TZ01_console_puts("Done\r\nPress 'c' key into configure mode.\r\n");
    
    TZ01_system_tick_start(USRTICK_NO_GPIO_INTERVAL, 10);
    TZ01_system_tick_start(USRTICK_NO_DISP_EVT_INTERVAL, 10);
    
    sw_init(&sw2_stat);
    
    /** メインループ **/
    app_stat = APP_STAT_DISP;
    int sw_stat = 0, prev_sw_stat = 0, sw_event = 0;
    for (;;) {
        if (TZ01_system_run() == RUNEVT_POWOFF) {
            /* Power off operation OR Low voltage detected */
            break;
        }
        
        sw_event = 0;
        if (TZ01_system_tick_check_timeout(USRTICK_NO_GPIO_INTERVAL)) {
            /* 10ms event*/
            TZ01_system_tick_start(USRTICK_NO_GPIO_INTERVAL, 10);
            
            sw_update(&sw2_stat, 6);
            
            sw_stat = sw_state(&sw2_stat);
            if (sw_stat != prev_sw_stat) {
                switch (sw_stat) {
                    case 0:
                        sw_event = 0;
                        break;
                    case 1:
                        sw_event = 1;
                        break;
                    case 2:
                        sw_event = 2;
                        break;
                }
            } else {
                sw_event = 0;
            }
            prev_sw_stat = sw_stat;
        }
        (state_func[app_stat])(sw_event);
    }
    
    NixieDigit_stop();
    return 0;
}
