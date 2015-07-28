#ifndef _NIXIE_DIGIT_H_
#define _NIXIE_DIGIT_H_

#define GPIO_DCDC_EN    (12)
#define GPIO_BCD_0      (22)
#define GPIO_BCD_1      (13)
#define GPIO_BCD_2      (20)
#define GPIO_BCD_3      (21)
#define GPIO_NUM10      (23)
#define GPIO_NX1        (16)
#define GPIO_NX2        (17)
#define GPIO_NX3        (18)
#define GPIO_NX4        (19)

#define TICK_WAIT_NX_WAIT_C_ON  (120)   //カソード値出力待ち時間 (桁OFF時間)
#define TICK_WAIT_NX_WAIT_A_ON  (0)     //アノードON待ち時間
#define TICK_WAIT_NX_WAIT_A_OFF (80)    //アノードOFF待ち時間(桁ON時間)
#define TICK_WAIT_NX_WAIT_C_F   (0)     //カソード1111出力待ち時間

typedef enum {
    STAT_IDLE,
    STAT_STARTUP,
    STAT_WAIT_NX1_C_ON,     //NX1のカソードON待ち
    STAT_WAIT_NX1_A_ON,     //NX1のアノードON待ち
    STAT_WAIT_NX1_A_OFF,    //NX1のアノードOFF待ち
    STAT_WAIT_NX1_C_F,      //NX1のカソード'1111'出力待ち
    STAT_WAIT_NX2_C_ON,     //NX2のカソードON待ち
    STAT_WAIT_NX2_A_ON,     //NX2のアノードON待ち
    STAT_WAIT_NX2_A_OFF,    //NX2のアノードOFF待ち
    STAT_WAIT_NX2_C_F,      //NX2のカソード'1111'出力待ち
    STAT_WAIT_NX3_C_ON,     //NX3のカソードON待ち
    STAT_WAIT_NX3_A_ON,     //NX3のアノードON待ち
    STAT_WAIT_NX3_A_OFF,    //NX3のアノードOFF待ち
    STAT_WAIT_NX3_C_F,      //NX3のカソード'1111'出力待ち
    STAT_WAIT_NX4_C_ON,     //NX4のカソードON待ち
    STAT_WAIT_NX4_A_ON,     //NX4のアノードON待ち
    STAT_WAIT_NX4_A_OFF,    //NX4のアノードOFF待ち
    STAT_WAIT_NX4_C_F,      //NX4のカソード'1111'出力待ち
}   NIXIE_DIGIT_STAT;

typedef enum {
    EVT_NONE,       //イベントなし
    EVT_TIMEOUT,    //タイムアウト
}   NIXIE_DIGIT_EVT;

int NixieDigit_init(uint16_t c_on, uint16_t a_on, uint16_t a_off, uint16_t c_f);
int NixieDigit_reconfig(uint16_t c_on, uint16_t a_on, uint16_t a_off, uint16_t c_f);
int NixieDigit_start(void);
int NixieDigit_stop(void);
int NixieDigit_set_nx1(uint8_t val, bool dp);
int NixieDigit_set_nx2(uint8_t val, bool dp);
int NixieDigit_set_nx3(uint8_t val, bool dp);
int NixieDigit_set_nx4(uint8_t val, bool dp);
int NixieDigit_run(void);

#endif
