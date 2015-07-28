#include "TZ10xx.h"
//#include "RTC_TZ10xx.h"
#include "PMU_TZ10xx.h"
#include "GPIO_TZ10xx.h"

#include "TZ01_system.h"
#include "TZ01_console.h"

#include "nixie_digit.h"

extern TZ10XX_DRIVER_PMU  Driver_PMU;
extern TZ10XX_DRIVER_GPIO Driver_GPIO;

static uint8_t value_table[] = {
    0x05,   //'0'
    0x07,   //'1'
    0x09,   //'2'
    0x02,   //'3'
    0x03,   //'4'
    0x00,   //'5'
    0x04,   //'6'
    0x01,   //'7'
    0x06,   //'8'
    0x08,   //'9'
    0x0f,   //' '
};

static uint16_t wait_c_on  = TICK_WAIT_NX_WAIT_C_ON;
static uint16_t wait_a_on  = TICK_WAIT_NX_WAIT_A_ON;
static uint16_t wait_a_off = TICK_WAIT_NX_WAIT_A_OFF;
static uint16_t wait_c_f   = TICK_WAIT_NX_WAIT_C_F;

uint8_t nx_val[4];
static NIXIE_DIGIT_STAT nx_stat = STAT_IDLE;

static void output_bcd(uint8_t val)
{
    //Number
    Driver_GPIO.WritePin(GPIO_BCD_0, (val >> 0) & 0x01);
    Driver_GPIO.WritePin(GPIO_BCD_1, (val >> 1) & 0x01);
    Driver_GPIO.WritePin(GPIO_BCD_2, (val >> 2) & 0x01);
    Driver_GPIO.WritePin(GPIO_BCD_3, (val >> 3) & 0x01);
}

/** State Functions **/

static void state_func_startup(NIXIE_DIGIT_EVT evt)
{
    TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_on);
    //DCDC_EN Hi
    Driver_GPIO.WritePin(GPIO_DCDC_EN, 1);
    
    nx_stat = STAT_WAIT_NX1_C_ON;       //NX1のカソードON待ちへ
}

static void state_func_wait_nx1_c_on(NIXIE_DIGIT_EVT evt)   //NX1のカソードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_on);
        
        //BCD出力
        output_bcd(nx_val[0]);
        
        nx_stat = STAT_WAIT_NX1_A_ON;   //NX1のアノードON待ちへ
    }
}


static void state_func_wait_nx1_a_on(NIXIE_DIGIT_EVT evt)   //NX1のアノードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_off);
        
        //NX1 アノードON
        Driver_GPIO.WritePin(GPIO_NX1, 1);
        //NUM10
        if (nx_val[0] & 0x80) {
            Driver_GPIO.WritePin(GPIO_NUM10, 1);
        }
        
        nx_stat = STAT_WAIT_NX1_A_OFF;  //NX1のアノードOFF待ちへ
    }
}

static void state_func_wait_nx1_a_off(NIXIE_DIGIT_EVT evt)  //NX1のアノードOFF待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_f);
        
        //NX1 アノードOFF
        Driver_GPIO.WritePin(GPIO_NX1, 0);
        //NUM10 OFF
        Driver_GPIO.WritePin(GPIO_NUM10, 0);
        
        nx_stat = STAT_WAIT_NX1_C_F;    //NX1のカソード'1111'出力待ち
    }
}

static void state_func_wait_nx1_c_f(NIXIE_DIGIT_EVT evt)    //NX1のカソード'1111'出力待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_on);
        
        //BCD出力
        output_bcd(0x0f);
        
        nx_stat = STAT_WAIT_NX2_C_ON;   //NX2のカソードON待ちへ
    }
}

static void state_func_wait_nx2_c_on(NIXIE_DIGIT_EVT evt)   //NX2のカソードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_on);
        
        //BCD出力
        output_bcd(nx_val[1]);
        
        nx_stat = STAT_WAIT_NX2_A_ON;   //NX2のアノードON待ちへ
    }
}

static void state_func_wait_nx2_a_on(NIXIE_DIGIT_EVT evt)   //NX2のアノードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_off);
        
        //NX2 アノードON
        Driver_GPIO.WritePin(GPIO_NX2, 1);
        //NUM10
        if (nx_val[1] & 0x80) {
            Driver_GPIO.WritePin(GPIO_NUM10, 1);
        }
        
        nx_stat = STAT_WAIT_NX2_A_OFF;  //NX2のアノードOFF待ちへ
    }
}
static void state_func_wait_nx2_a_off(NIXIE_DIGIT_EVT evt)  //NX2のアノードOFF待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_f);
        
        //NX2 アノードOFF
        Driver_GPIO.WritePin(GPIO_NX2, 0);
        //NUM10 OFF
        Driver_GPIO.WritePin(GPIO_NUM10, 0);
        
        nx_stat = STAT_WAIT_NX2_C_F;    //NX2のカソード'1111'出力待ち
    }
}

static void state_func_wait_nx2_c_f(NIXIE_DIGIT_EVT evt)    //NX2のカソード'1111'出力待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_on);
        
        //BCD出力
        output_bcd(0x0f);
        
        nx_stat = STAT_WAIT_NX3_C_ON;   //NX3のカソードON待ちへ
    }
}

static void state_func_wait_nx3_c_on(NIXIE_DIGIT_EVT evt)   //NX3のカソードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_on);
        
        //BCD出力
        output_bcd(nx_val[2]);
        
        nx_stat = STAT_WAIT_NX3_A_ON;   //NX3のアノードON待ちへ
    }
}

static void state_func_wait_nx3_a_on(NIXIE_DIGIT_EVT evt)   //NX3のアノードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_off);
        
        //NX3 アノードON
        Driver_GPIO.WritePin(GPIO_NX3, 1);
        //NUM10
        if (nx_val[2] & 0x80) {
            Driver_GPIO.WritePin(GPIO_NUM10, 1);
        }
        
        nx_stat = STAT_WAIT_NX3_A_OFF;  //NX3のアノードOFF待ちへ
    }
}

static void state_func_wait_nx3_a_off(NIXIE_DIGIT_EVT evt)  //NX3のアノードOFF待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_f);
        
        //NX3 アノードOFF
        Driver_GPIO.WritePin(GPIO_NX3, 0);
        //NUM10 OFF
        Driver_GPIO.WritePin(GPIO_NUM10, 0);
        
        nx_stat = STAT_WAIT_NX3_C_F;    //NX2のカソード'1111'出力待ち
    }
}

static void state_func_wait_nx3_c_f(NIXIE_DIGIT_EVT evt)    //NX3のカソード'1111'出力待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_on);
        
        //BCD出力
        output_bcd(0x0f);
        
        nx_stat = STAT_WAIT_NX4_C_ON;   //NX4のカソードON待ちへ
    }
}

static void state_func_wait_nx4_c_on(NIXIE_DIGIT_EVT evt)   //NX4のカソードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_on);
        
        //BCD出力
        output_bcd(nx_val[3]);
        
        nx_stat = STAT_WAIT_NX4_A_ON;   //NX3のアノードON待ちへ
    }
}

static void state_func_wait_nx4_a_on(NIXIE_DIGIT_EVT evt)   //NX4のアノードON待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_a_off);
        
        //NX4 アノードON
        Driver_GPIO.WritePin(GPIO_NX4, 1);
        //NUM10
        if (nx_val[3] & 0x80) {
            Driver_GPIO.WritePin(GPIO_NUM10, 1);
        }
        
        nx_stat = STAT_WAIT_NX4_A_OFF;  //NX4のアノードOFF待ちへ
    }
}

static void state_func_wait_nx4_a_off(NIXIE_DIGIT_EVT evt)  //NX4のアノードOFF待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_f);
        
        //NX4 アノードOFF
        Driver_GPIO.WritePin(GPIO_NX4, 0);
        //NUM10 OFF
        Driver_GPIO.WritePin(GPIO_NUM10, 0);
        
        nx_stat = STAT_WAIT_NX4_C_F;    //NX4のカソード'1111'出力待ち
    }
}

static void state_func_wait_nx4_c_f(NIXIE_DIGIT_EVT evt)    //NX4のカソード'1111'出力待ち
{
    if (evt == EVT_TIMEOUT) {
        TZ01_system_tick_start_us(USRTICK_NO_NIXIE_DISP, wait_c_on);
        
        //BCD出力
        output_bcd(0x0f);
        
        nx_stat = STAT_WAIT_NX1_C_ON;   //NX1のカソードON待ちへ
    }
}

static void state_func_term(NIXIE_DIGIT_EVT evt)
{
}

//関数リスト
static void (*state_func[])(NIXIE_DIGIT_EVT evt) = {
    NULL,
    state_func_startup,
    state_func_wait_nx1_c_on,   //NX1のカソードON待ち
    state_func_wait_nx1_a_on,   //NX1のアノードON待ち
    state_func_wait_nx1_a_off,  //NX1のアノードOFF待ち
    state_func_wait_nx1_c_f,    //NX1のカソード'1111'出力待ち
    state_func_wait_nx2_c_on,   //NX2のカソードON待ち
    state_func_wait_nx2_a_on,   //NX2のアノードON待ち
    state_func_wait_nx2_a_off,  //NX2のアノードOFF待ち
    state_func_wait_nx2_c_f,    //NX2のカソード'1111'出力待ち
    state_func_wait_nx3_c_on,   //NX3のカソードON待ち
    state_func_wait_nx3_a_on,   //NX3のアノードON待ち
    state_func_wait_nx3_a_off,  //NX3のアノードOFF待ち
    state_func_wait_nx3_c_f,    //NX3のカソード'1111'出力待ち
    state_func_wait_nx4_c_on,   //NX4のカソードON待ち
    state_func_wait_nx4_a_on,   //NX4のアノードON待ち
    state_func_wait_nx4_a_off,  //NX4のアノードOFF待ち
    state_func_wait_nx4_c_f,    //NX4のカソード'1111'出力待ち
};

static int nixiedigit_set_nx(uint8_t digit, uint8_t val, bool dp)
{
    if (digit > 3) {
        return 1;
    }
    
    if (val > 10) {
        return 1;
    }
    
    nx_val[digit] = value_table[val];
    if (dp) {
        nx_val[digit] |= 0x80;  //dpビットON
    }
    
    return 0;
}

/* 公開関数 */

int NixieDigit_init(uint16_t c_on, uint16_t a_on, uint16_t a_off, uint16_t c_f) 
{
    //ダイナミック点灯待ち時間
    wait_c_on = c_on;
    wait_a_on = a_on;
    wait_a_off = a_off;
    wait_c_f = c_f;
    
    //PMU
    
    //GPIO
    Driver_GPIO.Configure(GPIO_DCDC_EN, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_BCD_0, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_BCD_1, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_BCD_2, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_BCD_3, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_NUM10, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_NX1, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_NX2, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_NX3, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    Driver_GPIO.Configure(GPIO_NX4, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);

Driver_GPIO.Configure(9, GPIO_DIRECTION_OUTPUT_2MA, GPIO_EVENT_DISABLE, NULL);
    
    //変数
    nx_val[0] = 0x0f;
    nx_val[1] = 0x0f;
    nx_val[2] = 0x0f;
    nx_val[3] = 0x0f;
    
    nx_stat = STAT_IDLE;
    
    return 0;
}

int NixieDigit_reconfig(uint16_t c_on, uint16_t a_on, uint16_t a_off, uint16_t c_f)
{
    if (nx_stat != STAT_IDLE) {
        return 1;
    }
    
    //ダイナミック点灯待ち時間
    wait_c_on = c_on;
    wait_a_on = a_on;
    wait_a_off = a_off;
    wait_c_f = c_f;
    
    return 0;
}

int NixieDigit_start(void)
{
    if (nx_stat != STAT_IDLE) {
        return 1;
    }
    
    nx_val[0] = 0x0f;
    nx_val[1] = 0x0f;
    nx_val[2] = 0x0f;
    nx_val[3] = 0x0f;
    
    nx_stat = STAT_STARTUP;
    
    return 0;
}

int NixieDigit_stop(void)
{
    if (nx_stat == STAT_IDLE) {
        return 1;
    }
    
    nx_val[0] = 0x0f;
    nx_val[1] = 0x0f;
    nx_val[2] = 0x0f;
    nx_val[3] = 0x0f;

    //GPIO OFF
    Driver_GPIO.WritePin(GPIO_DCDC_EN, 0);
    Driver_GPIO.WritePin(GPIO_NX1, 0);
    Driver_GPIO.WritePin(GPIO_NX2, 0);
    Driver_GPIO.WritePin(GPIO_NX3, 0);
    Driver_GPIO.WritePin(GPIO_NX4, 0);
    Driver_GPIO.WritePin(GPIO_BCD_0, 0);
    Driver_GPIO.WritePin(GPIO_BCD_1, 0);
    Driver_GPIO.WritePin(GPIO_BCD_2, 0);
    Driver_GPIO.WritePin(GPIO_BCD_3, 0);
    Driver_GPIO.WritePin(GPIO_NUM10, 0);
    
    nx_stat = STAT_IDLE;
    
    return 0;
}

int NixieDigit_set_nx1(uint8_t val, bool dp)
{
    return nixiedigit_set_nx(0, val, dp);
}

int NixieDigit_set_nx2(uint8_t val, bool dp)
{
    return nixiedigit_set_nx(1, val, dp);
}

int NixieDigit_set_nx3(uint8_t val, bool dp)
{
    return nixiedigit_set_nx(2, val, dp);
}

int NixieDigit_set_nx4(uint8_t val, bool dp)
{
    return nixiedigit_set_nx(3, val, dp);
}

int NixieDigit_run(void)
{
    /* イベント */
    NIXIE_DIGIT_EVT evt = EVT_NONE;
    if (TZ01_system_tick_check_timeout(USRTICK_NO_NIXIE_DISP)) {
        evt = EVT_TIMEOUT;
    }
    
    /* 状態 */
    if (state_func[nx_stat] != NULL) {
        (state_func[nx_stat])(evt);
    }
}
