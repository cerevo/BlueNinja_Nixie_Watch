/**
 * @file ble_tracker.c
 * @breaf Cerevo CDP-TZ01B sample program.
 * BLE Motion Tracker
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
#include "TZ10xx.h"
#include "RTC_TZ10xx.h"
#include "PMU_TZ10xx.h"
#include "RNG_TZ10xx.h"
#include "GPIO_TZ10xx.h"

#include "twic_interface.h"
#include "twic_led.h"
#include "blelib.h"

#include "TZ01_system.h"
#include "TZ01_console.h"

#define APP_TZ1EM_WF_GPIO TZ1EM_WF_G2
#define TZ01_TRACKER_MTU    (40)

extern TZ10XX_DRIVER_RTC Driver_RTC;
extern TZ10XX_DRIVER_RNG Driver_RNG;

static uint64_t tz01_tracker_bdaddr  = 0xc0ce00000000;   //

static bool disconnected = false;

/*--- GATT profile definition ---*/
const uint8_t tz01_tracker_gap_device_name[] = "CDP-TZ01B_CTS";
const uint8_t tz01_tracker_gap_appearance[] = {0x00, 0x00};

const uint8_t tz01_tracker_di_manufname[] = "Cerevo";
const uint8_t tz01_tracker_di_fw_version[] = "0.1";
const uint8_t tz01_tracker_di_sw_version[] = "0.1";
const uint8_t tz01_tracker_di_model_string[] = "CDP-TZ01B_CTS";

/* BLElib unique id. */
enum {
    BLE_GATT_UNIQUE_ID_GAP_SERVICE = 0,
	BLE_GATT_UNIQUE_ID_GAP_DEVICE_NAME,
	BLE_GATT_UNIQUE_ID_GAP_APPEARANCE,
	BLE_GATT_UNIQUE_ID_DI_SERVICE,
	BLE_GATT_UNIQUE_ID_DI_MANUF_NAME,
	BLE_GATT_UNIQUE_ID_DI_FW_VERSION,
	BLE_GATT_UNIQUE_ID_DI_SW_VERSION,
	BLE_GATT_UNIQUE_ID_DI_MODEL_STRING,
	BLE_GATT_UNIQUE_ID_CTS_SERVICE,
	BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME,
    BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME_DESC,
};

/* GAP */
const BLELib_Characteristics gap_device_name = {
	BLE_GATT_UNIQUE_ID_GAP_DEVICE_NAME, 0x2a00, 0, BLELIB_UUID_16,
	BLELIB_PROPERTY_READ,
	BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
	tz01_tracker_gap_device_name, sizeof(tz01_tracker_gap_device_name),
	NULL, 0
};
const BLELib_Characteristics gap_appearance = {
	BLE_GATT_UNIQUE_ID_GAP_APPEARANCE, 0x2a01, 0, BLELIB_UUID_16,
	BLELIB_PROPERTY_READ,
	BLELIB_PERMISSION_READ,
	tz01_tracker_gap_appearance, sizeof(tz01_tracker_gap_appearance),
	NULL, 0
};
const BLELib_Characteristics *const gap_characteristics[] = { &gap_device_name, &gap_appearance };
const BLELib_Service gap_service = {
	BLE_GATT_UNIQUE_ID_GAP_SERVICE, 0x1800, 0, BLELIB_UUID_16,
	true, NULL, 0,
	gap_characteristics, 2
};

/* DIS(Device Informatin Service) */
const BLELib_Characteristics di_manuf_name = {
	BLE_GATT_UNIQUE_ID_DI_MANUF_NAME, 0x2a29, 0, BLELIB_UUID_16,
	BLELIB_PROPERTY_READ,
	BLELIB_PERMISSION_READ,
	tz01_tracker_di_manufname, sizeof(tz01_tracker_di_manufname),
	NULL, 0
};
const BLELib_Characteristics di_fw_version = {
	BLE_GATT_UNIQUE_ID_DI_FW_VERSION, 0x2a26, 0, BLELIB_UUID_16,
	BLELIB_PROPERTY_READ,
	BLELIB_PERMISSION_READ,
	tz01_tracker_di_fw_version, sizeof(tz01_tracker_di_fw_version),
	NULL, 0
};
const BLELib_Characteristics di_sw_version = {
	BLE_GATT_UNIQUE_ID_DI_SW_VERSION, 0x2a28, 0, BLELIB_UUID_16,
	BLELIB_PROPERTY_READ,
	BLELIB_PERMISSION_READ,
	tz01_tracker_di_sw_version, sizeof(tz01_tracker_di_sw_version),
	NULL, 0
};
const BLELib_Characteristics di_model_string = {
	BLE_GATT_UNIQUE_ID_DI_MODEL_STRING, 0x2a24, 0, BLELIB_UUID_16,
	BLELIB_PROPERTY_READ,
	BLELIB_PERMISSION_READ,
	tz01_tracker_di_model_string, sizeof(tz01_tracker_di_model_string),
	NULL, 0
};
const BLELib_Characteristics *const di_characteristics[] = {
	&di_manuf_name, &di_fw_version, &di_sw_version, &di_model_string
};
const BLELib_Service di_service = {
	BLE_GATT_UNIQUE_ID_DI_SERVICE, 0x180a, 0, BLELIB_UUID_16,
	true, NULL, 0,
	di_characteristics, 4
};

static uint8_t curr_time[10];
static uint8_t cts_ct_desc_val[] = { 0x00, 0x00 };

/* CTS Current Time */
const BLELib_Descriptor cts_ct_desc = {
	BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME_DESC, 0x2902, 0, BLELIB_UUID_16,
	BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
	cts_ct_desc_val, sizeof(cts_ct_desc_val)
};
const BLELib_Descriptor *const cts_ct_descriptors[] = { &cts_ct_desc };
const BLELib_Characteristics cts_ct = {
    BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME, 0x2a2b, 0x0000, BLELIB_UUID_16,
    BLELIB_PROPERTY_NOTIFY | BLELIB_PROPERTY_READ | BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    curr_time, sizeof(curr_time),
    cts_ct_descriptors, 1
};

/* Characteristics list */
const BLELib_Characteristics *const cts_characteristics[] = {
    &cts_ct,
};
/* TZ1 Tracker service */
const BLELib_Service cts_service = {
	BLE_GATT_UNIQUE_ID_CTS_SERVICE, 0x1805, 0x0000, BLELIB_UUID_16,
	true, NULL, 0,
	cts_characteristics, 1
};

/* Service list */
const BLELib_Service *const tz01_tracker_service_list[] = {
	&gap_service, &di_service, &cts_service
};

/*- INDICATION data -*/
uint8_t tz01_tracker_advertising_data[] = {
	0x02, /* length of this data */
	0x01, /* AD type = Flags */
	0x06, /* LE General Discoverable Mode = 0x02 */
	/* BR/EDR Not Supported (i.e. bit 37
	 * of LMP Extended Feature bits Page 0) = 0x04 */

	0x03, /* length of this data */
	0x08, /* AD type = Short local name */
	'T',  /* (T) */
	'Z',  /* (Z) */

	0x05, /* length of this data */
	0x03, /* AD type = Complete list of 16-bit UUIDs available */
	0x00, /* Generic Access Profile Service 1800 */
	0x18,
	0x0A, /* Device Information Service 180A */
	0x18,
};

uint8_t tz01_tracker_scan_resp_data[] = {
	0x02, /* length of this data */
	0x01, /* AD type = Flags */
	0x06, /* LE General Discoverable Mode = 0x02 */
	/* BR/EDR Not Supported (i.e. bit 37
	 * of LMP Extended Feature bits Page 0) = 0x04 */

	0x02, /* length of this data */
	0x0A, /* AD type = TX Power Level (1 byte) */
	0x00, /* 0dB (-127...127 = 0x81...0x7F) */

	0x0e, /* length of this data */
	0x09, /* AD type = Complete local name */
	'C', 'D', 'P', '-', 'T', 'Z', '0', '1', 'B', '_', 'C', 'T', 'S' /* CDP-TZ01B_CTS */
};

/*=== TZ1 motion tracker application ===*/
/*= Timer =*/
static bool tz01_cts_notif_enable         = false;

static uint64_t central_bdaddr;

/*= BLElib callback functions =*/
void connectionCompleteCb(const uint8_t status, const bool master, const uint64_t bdaddr, const uint16_t conn_interval)
{
	central_bdaddr = bdaddr;

    //Notification disabled.
    tz01_cts_notif_enable = false;

    BLELib_requestMtuExchange(TZ01_TRACKER_MTU);

    //tz1smHalTimerStart(tz01_tracker_timer_id_notif, 500);
    TZ01_system_tick_start(USRTICK_NO_BLE_MAIN, 100);
}

void connectionUpdateCb(const uint8_t status, const uint16_t conn_interval, const uint16_t conn_latency)
{
}

void disconnectCb(const uint8_t status, const uint8_t reason)
{
    //Notification disabled.
    tz01_cts_notif_enable = false;

    disconnected = true;
    
    //tz1smHalTimerStop(tz01_tracker_timer_id_notif);
    TZ01_system_tick_stop(USRTICK_NO_BLE_MAIN);
}

BLELib_RespForDemand mtuExchangeDemandCb(const uint16_t client_rx_mtu_size, uint16_t *resp_mtu_size)
{
    uint8_t msg[64];
	*resp_mtu_size = TZ01_TRACKER_MTU;
    sprintf(msg, "client_rx_mtu_size=%d, resp_mtu_size=%d\r\n", client_rx_mtu_size, *resp_mtu_size);
    TZ01_console_puts(msg);
	return BLELIB_DEMAND_ACCEPT;
}

void mtuExchangeResultCb(const uint8_t status, const uint16_t negotiated_mtu_size)
{
    uint8_t msg[32];
    sprintf(msg, "negotiated_mtu_size=%d\r\n", negotiated_mtu_size);
    TZ01_console_puts(msg);
}

void notificationSentCb(const uint8_t unique_id)
{
}

void indicationConfirmCb(const uint8_t unique_id)
{
}

void updateCompleteCb(const uint8_t unique_id)
{
}

void queuedWriteCompleteCb(const uint8_t status)
{
}

BLELib_RespForDemand readoutDemandCb(const uint8_t *const unique_id_array, const uint8_t unique_id_num)
{
	return BLELIB_DEMAND_ACCEPT;
}

BLELib_RespForDemand writeinDemandCb(const uint8_t unique_id, const uint8_t *const value, const uint8_t value_len)
{
    RTC_TIME now;
    switch (unique_id) {
        case BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME_DESC:
            /* Notification Enable/Disable */
            tz01_cts_notif_enable  = (value[0] != 0);
            break;
        case BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME:
            /* Write current time */
            memcpy(curr_time, value, sizeof(curr_time));
            now.year = (curr_time[0] | (curr_time[1] << 8)) - 2000;
            now.mon  = curr_time[2];
            now.mday = curr_time[3];
            now.hour = curr_time[4];
            now.min  = curr_time[5];
            now.sec  = curr_time[6];
            now.wday = curr_time[7];
            Driver_RTC.SetTime(&now);
            break;
    }
	return BLELIB_DEMAND_ACCEPT;
}

void writeinPostCb(const uint8_t unique_id, const uint8_t *const value, const uint8_t value_len)
{
}

void isrNewEventCb(void)
{
	/* this sample always call BLELib_run() */
}

void isrWakeupCb(void)
{
	/* this callback is not used currently */
}

BLELib_CommonCallbacks tz01_common_callbacks = {
	connectionCompleteCb,
	connectionUpdateCb,
	mtuExchangeResultCb,
	disconnectCb,
	isrNewEventCb,
	isrWakeupCb
  };

BLELib_ServerCallbacks tz01_server_callbacks = {
	mtuExchangeDemandCb,
	notificationSentCb,
	indicationConfirmCb,
	updateCompleteCb,
	queuedWriteCompleteCb,
	readoutDemandCb,
	writeinDemandCb,
	writeinPostCb,
  };

void rtc_periodic_handler(RTC_EVENT e)
{
    tz1smHalTimerFakeAlarmTrigger(128 /* Hz */);
}

int ble_cts_init(void)
{
    if (TZ1EM_STATUS_OK != tz1emInitializeSystem())
        return 1; /* Must not use UART for LOG before twicIfLeIoInitialize. */

    Driver_RTC.SetPeriodicInterrupt(RTC_PERIOD_EVERY_1_128_SECOND, rtc_periodic_handler);
    
    /* create random bdaddr */
    uint32_t randval;
    Driver_PMU.SetPowerDomainState(PMU_PD_ENCRYPT, PMU_PD_MODE_ON);
    Driver_RNG.Initialize();
    Driver_RNG.PowerControl(ARM_POWER_FULL);
    Driver_RNG.Read(&randval);
    Driver_RNG.Uninitialize();
    tz01_tracker_bdaddr |= (uint64_t)randval;

    /* initialize BLELib */
    int ret;
    BLELib_initialize(tz01_tracker_bdaddr, BLELIB_BAUDRATE_2304, &tz01_common_callbacks, &tz01_server_callbacks, NULL);
    ret = BLELib_registerService(tz01_tracker_service_list, 3);
    BLELib_setLowPowerMode(BLELIB_LOWPOWER_ON);

    twicLedInit();

    return ret;
}

static uint8_t msg[80];
static bool is_adv = false;

int ble_cts_start(void)
{
    disconnected = false;
    is_adv = false;
    return 0;
}

int ble_cts_run(const int key_stat)
{
    RTC_TIME now;
    int ret, res = 0;
    BLELib_State state;
    bool has_event;

    state = BLELib_getState();
    has_event = BLELib_hasEvent();

    switch (state) {
        case BLELIB_STATE_UNINITIALIZED:
        case BLELIB_STATE_INITIALIZED:
            break;

        case BLELIB_STATE_ADVERTISE_READY:
            twicSetLed(TWIC_LED_GPIO_LED3, false);
            if (disconnected == true) {
                sprintf(msg, "Disconnected. state=%d\r\n", ret, state);
                TZ01_console_puts(msg);
                res = 1;
            } else {
                if (is_adv == false) {
                    ret = BLELib_startAdvertising(tz01_tracker_advertising_data, sizeof(tz01_tracker_advertising_data), tz01_tracker_scan_resp_data, sizeof(tz01_tracker_scan_resp_data));
                    sprintf(msg, "BLELib_startAdvertising(): %d state=%d\r\n", ret, state);
                    TZ01_console_puts(msg);
                    is_adv = true;
                }
            }
            break;
        case BLELIB_STATE_ADVERTISING:
            is_adv = false;
            twicSetLed(TWIC_LED_GPIO_LED3, true);
            if (key_stat == 1 /* Push */) {
                ret = BLELib_stopAdvertising();
                sprintf(msg, "BLELib_stopAdvertising(): %d\r\n", ret);
                TZ01_console_puts(msg);
                disconnected = true;
            }
            break;

        case BLELIB_STATE_ONLINE:
            if (key_stat == 1 /* Push */) {
                ret = BLELib_disconnect(central_bdaddr);
                sprintf(msg, "BLELib_disconnect(): %d\r\n", ret);
                TZ01_console_puts(msg);
                break;
            }

            if (TZ01_system_tick_check_timeout(USRTICK_NO_BLE_MAIN)) {
                TZ01_system_tick_start(USRTICK_NO_BLE_MAIN, 1000);
                Driver_RTC.GetTime(&now);
                curr_time[0] = (now.year + 2000) & 0xff;        // Year
                curr_time[1] = ((now.year + 2000) >> 8) & 0xff; // Year
                curr_time[2] = now.mon;                         // Month
                curr_time[3] = now.mday;                        // Day
                curr_time[4] = now.hour;                        // Hour
                curr_time[5] = now.min;                         // Minutes
                curr_time[6] = now.sec;                         // Second
                curr_time[7] = now.wday;                        // Week of day
                curr_time[8] = 0;                               // 1/256 second
                curr_time[9] = 0x00;                            // Adjust Reason
                
                sprintf(msg, "%04d/%02d/%02d %02d:%02d:%02d\r\n", now.year + 2000, now.mon, now.mday, now.hour, now.min, now.sec);
                TZ01_console_puts(msg);
                BLELib_updateValue(BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME, curr_time, sizeof(curr_time));
                
                if (tz01_cts_notif_enable) {
                    ret = BLELib_notifyValue(BLE_GATT_UNIQUE_ID_CTS_CURRENT_TIME, curr_time, 10);
                    if (ret != 0) {
                        sprintf(msg, "%d BLELib_notifyValue(): %d\r\n", __LINE__, ret);
                        TZ01_console_puts(msg);
                    }
                }
            }
        default:
            break;
    }

    if (has_event) {
        ret = BLELib_run();
        if (ret != 0) {
            sprintf(msg, "BLELib_run(): %d state=%d\r\n", ret, state);
            TZ01_console_puts(msg);
        }
    }

    return res;
}

void ble_cts_stop(void)
{
    switch (BLELib_getState()) {
        case BLELIB_STATE_ADVERTISING:
            BLELib_stopAdvertising();
            twicSetLed(TWIC_LED_GPIO_LED3, false);
            break;

        case BLELIB_STATE_ONLINE:
            BLELib_disconnect(central_bdaddr);
            break;
        default:
            break;
    }
}
