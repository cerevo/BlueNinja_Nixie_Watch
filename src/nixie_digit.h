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

#define TICK_WAIT_NX_WAIT_C_ON  (120)   //�J�\�[�h�l�o�͑҂����� (��OFF����)
#define TICK_WAIT_NX_WAIT_A_ON  (0)     //�A�m�[�hON�҂�����
#define TICK_WAIT_NX_WAIT_A_OFF (80)    //�A�m�[�hOFF�҂�����(��ON����)
#define TICK_WAIT_NX_WAIT_C_F   (0)     //�J�\�[�h1111�o�͑҂�����

typedef enum {
    STAT_IDLE,
    STAT_STARTUP,
    STAT_WAIT_NX1_C_ON,     //NX1�̃J�\�[�hON�҂�
    STAT_WAIT_NX1_A_ON,     //NX1�̃A�m�[�hON�҂�
    STAT_WAIT_NX1_A_OFF,    //NX1�̃A�m�[�hOFF�҂�
    STAT_WAIT_NX1_C_F,      //NX1�̃J�\�[�h'1111'�o�͑҂�
    STAT_WAIT_NX2_C_ON,     //NX2�̃J�\�[�hON�҂�
    STAT_WAIT_NX2_A_ON,     //NX2�̃A�m�[�hON�҂�
    STAT_WAIT_NX2_A_OFF,    //NX2�̃A�m�[�hOFF�҂�
    STAT_WAIT_NX2_C_F,      //NX2�̃J�\�[�h'1111'�o�͑҂�
    STAT_WAIT_NX3_C_ON,     //NX3�̃J�\�[�hON�҂�
    STAT_WAIT_NX3_A_ON,     //NX3�̃A�m�[�hON�҂�
    STAT_WAIT_NX3_A_OFF,    //NX3�̃A�m�[�hOFF�҂�
    STAT_WAIT_NX3_C_F,      //NX3�̃J�\�[�h'1111'�o�͑҂�
    STAT_WAIT_NX4_C_ON,     //NX4�̃J�\�[�hON�҂�
    STAT_WAIT_NX4_A_ON,     //NX4�̃A�m�[�hON�҂�
    STAT_WAIT_NX4_A_OFF,    //NX4�̃A�m�[�hOFF�҂�
    STAT_WAIT_NX4_C_F,      //NX4�̃J�\�[�h'1111'�o�͑҂�
}   NIXIE_DIGIT_STAT;

typedef enum {
    EVT_NONE,       //�C�x���g�Ȃ�
    EVT_TIMEOUT,    //�^�C���A�E�g
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
