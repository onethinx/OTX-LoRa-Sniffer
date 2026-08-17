/********************************************************************************
 *    ___             _   _     _            
 *   / _ \ _ __   ___| |_| |__ (_)_ __ __  __
 *  | | | | '_ \ / _ \ __| '_ \| | '_ \\ \/ /
 *  | |_| | | | |  __/ |_| | | | | | | |>  < 
 *   \___/|_| |_|\___|\__|_| |_|_|_| |_/_/\_\
 *
 ********************************************************************************
 *
 * Copyright (c) 2019 Onethinx BV <info@onethinx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ********************************************************************************
 *
 * Created by: Rolf Nooteboom
 *
 * Library to use with the Onethinx Core LoRaWAN module.
 * For a description please see:
 *      https://github.com/onethinx/OnethinxCoreAPI
 *
 * Core revisions:
 *      0x000000AA   Fix DevAddr in OnethinxCore01.h and reserved byte amount fixed
 *      0x000000AB   Implemented M0+ reset: failed debugging
 *      0x000000AC   Implemented breakCurrentFunction
 *      0x000000AD   Added configurations pointer validity check
 *      0x000000AE   Transmit power issue solved (device was always sending at TX_MAX)
 *      0x000000B0   Added US sub-bands, enhanced RX1 + RX2 window accuracy, fixed CM0p SRAM memory mapping,
 *      0x000000B1   Added LP functionality, added FlashWrites, added possibility to read Dev Address
 *      0x000000B2   Cleared RX-timeout flag if no downlink response is expected, added DevEUI & build info,
 *                   fixed high power consumption glitches at wakeup, Added LowPowerDebug, Improved active-power consumption
 *      0x000000B3   Fix Join accept on SF12 in RX1 window
 *      0x000000B4   Fix WakeUp pin in Sleepmode, FlashRead fix for row !=0
 *      0x000000B5   Fix Flashwrites after sleep
 *      0x000000B7   Restructured stack core, Capsense configuration fix, Add LoRa<>LoRa functionality, Add MAC Cmd LinkADR,
 *                   Fixed RX window timing, Fix confirmed downlink reply, Fix US join implementation, Add Low Power Join, Stability fixes
 *      0x000000B8   Restructured stack core, added low-power idle/join
 *      0x000000B9   Fix ADR
 *      0x000000BA   Unlock functions to use Port 6 & 7 for Capsense and SDW IOs
 *      0x000000BB   Fix TX timeout setting for EU SF12/125 (payload > 27 bytes), Fix Flashwrites
 *      0x000000BC   Fix LoRa to LoRa Communication
 *      0x000000BD   Added FSK modulation
 *      0x000000BE   Fixed small timing issues, MAC commands, restructured stack etc.
 *      0x000000BF   Changed RX timing window to SysTick timer (32MHz instead of 32KHz)
 *      0x000000C0   Fix SX126x Wakeup settings from coldstart (used when BleEcoON = true), added MAC save functionality
 *                   to resume LoRaWAN operations after hibernate, added RX boost functionality, added Set & Get timestamp RTC function
 *      0x000000C1   Fix RX window timing after deepsleep
 *
 *      New Versioning 0xMMMM.mmxx: M = Major, m = Minor, x = Revision. Changes in major / minor needs API update, changes in revision don't.
 *      0x0001.D000  12-12-2024 Add Class C support, Add Get next possible Uplink time, Several Improvements
 *      0x0001.D020  17-03-2025 DeviceTime commands implemented
 *      0x0001.D100  21-03-2025 ACK on Class C confirmed downlinks

 ********************************************************************************/

#pragma once

#ifndef USE_OLD_CORE_API

#include <stdint.h>
#include <stdbool.h>

/* Version definition */
#define apiVersion             0x0001D314

/* Basic Types */
typedef struct Arr8B_t {
    uint8_t Bytes[8];
} Arr8B_t;

typedef struct Arr16B_t {
    uint8_t Bytes[16];
} Arr16B_t;

/* Date and Time union */
typedef union {
    uint32_t Value;
    struct {
        uint32_t Second     : 6;
        uint32_t Minute     : 6;
        uint32_t Hour       : 5;                                    //!< Hour in 24h mode
        uint32_t DayOfMonth : 5;                                    //!< First day of month = 0
        uint32_t Month      : 4;                                    //!< First month = 0
        uint32_t Year       : 6;                                    //!< Year 2000 = 0
    };
} DateTime_t;

/**
 * Macro to initialize a dateTime_t instance.
 * Parameters: _Second, _Minute, _Hour, _DayOfMonth, _Month, _Year.
 */
#define DateTime(_Second, _Minute, _Hour, _DayOfMonth, _Month, _Year)   { .Year = _Year, .Month = _Month, .DayOfMonth = _DayOfMonth, .Hour = _Hour, .Minute = _Minute, .Second = _Second }

/* Manufacturing DevEUI:
   If the code { 0, 0, 0, 0, 0, 0, 0, 0 } is recognized, it is replaced with the
   manufacturing DevEUI. This manufacturing DevEUI can be requested using OTX18_GetInfo. */
#define thisDevEUI             { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

/* Core Function Identifiers */
typedef enum CoreFunctions_e {
    CORE_FUNCTION_INIT                  = 0x01,
    CORE_FUNCTION_RESET                 = 0x02,
    CORE_FUNCTION_GET_INFO              = 0x03,
    CORE_FUNCTION_LW_JOIN               = 0x20,
    CORE_FUNCTION_LW_SEND               = 0x30,
    CORE_FUNCTION_LW_SEND_MAC           = 0x31,
    CORE_FUNCTION_GET_RX_DATA           = 0x40,
    CORE_FUNCTION_GET_RX_MAC_CMD        = 0x41,
    CORE_FUNCTION_SLEEP                 = 0x50,
    CORE_FUNCTION_SET_DATE_TIME         = 0x60,
    CORE_FUNCTION_GET_DATE_TIME         = 0x61,
    CORE_FUNCTION_LW_GET_UPLINK_INFO    = 0x68,
    CORE_FUNCTION_EEPROM_READ           = 0x70,
    CORE_FUNCTION_EEPROM_WRITE          = 0x71,
    CORE_FUNCTION_GET_RANDOM            = 0x72,
    CORE_FUNCTION_PROTECT               = 0x73,
    CORE_FUNCTION_AES_ECB_CRYPT         = 0x74,
    CORE_FUNCTION_SHA_CALC              = 0x75,
    /* Extended Core Functions */
	CORE_FUNCTION_L_RX 					= 0x80,
	CORE_FUNCTION_L_TX 					= 0x81,
    CORE_FUNCTION_SX126X                = 0x88,
    CORE_FUNCTION_DEBUG                 = 0xA3,
    CORE_FUNCTION_UNLOCK                = 0xA4,
} CoreFunctions_e;

typedef enum {
    SX126X_MODE_READ            = 1,
    SX126X_MODE_WRITE           = 2,
    SX126X_MODE_READ_WRITE      = 3
} SX126X_RWmode_e;

/* IPC Message Structures */
typedef struct __attribute__((packed, aligned(4))) {
    uint8_t  ClientId;
    uint8_t  UserCode;
    uint16_t IntrMask;
    volatile void *CoreArgumentsPtr;
} IpcMsg_t;

typedef struct __attribute__((packed, aligned(4))) {
    IpcMsg_t forCM0;
    IpcMsg_t fromCM0;
} IpcMsgs_t;

/* Key Types */
typedef enum {
    ABP_10x_key   = 0x01,
    OTAA_10x_key  = 0x02,
    OTAA_11x_key  = 0x03,
    PreStored_key = 0xF0,
    UserStored_key= 0xF1,
} KeyType_e;

typedef struct __attribute__((packed)) {
    KeyType_e KeyType;
    uint8_t   KeyIndex;
} StoredKeys_t;

/* LoRaWAN Key Structures */
typedef struct __attribute__((packed)) {
    Arr8B_t  DevEui;
    Arr8B_t  AppEui;
    Arr16B_t AppKey;
} OTAA_10x_t;
	
typedef struct __attribute__((packed)) {
    Arr8B_t  DevEui;
    Arr8B_t  JoinEui;
    Arr16B_t AppKey;
    Arr16B_t NwkKey;
} OTAA_11x_t;

typedef struct __attribute__((packed)) {
    Arr8B_t  DevEui;
    uint32_t DevAddr;
    Arr16B_t NwkSkey;
    Arr16B_t AppSkey;
} ABP_10x_t;

typedef struct __attribute__((packed)) {
    KeyType_e  KeyType       : 8;
    uint8_t    reserved      : 7;
    bool       PublicNetwork : 1;
    union {
        uint8_t    totalbytes[64];
        StoredKeys_t StoredKeys;
        OTAA_10x_t   OTAA_10x;
        OTAA_11x_t   OTAA_11x;
        ABP_10x_t    ABP_10x;
    };
} LoRaWAN_keys_t;

/* Radio Data Rate (DR_...) - values remain unchanged */
typedef enum {
    DR_0    = 0x00,                                                 //!< EU: SF12 125KHz, US: SF10 125KHz
    DR_1    = 0x01,                                                 //!< EU: SF11 125KHz, US: SF9 125KHz
    DR_2    = 0x02,                                                 //!< EU: SF10 125KHz, US: SF8 125KHz
    DR_3    = 0x03,                                                 //!< EU: SF9 125KHz,  US: SF7 125KHz
    DR_4    = 0x04,                                                 //!< EU: SF8 125KHz,  US: SF8 500KHz
    DR_5    = 0x05,                                                 //!< EU: SF7 125KHz
    DR_6    = 0x06,                                                 //!< EU: SF7 250KHz
    DR_7    = 0x07,                                                 //!< EU: FSK 50kbps
    DR_8    = 0x08,                                                 //!< US: SF12 500KHz (downlinks only)
    DR_9    = 0x09,                                                 //!< US: SF11 500KHz (downlinks only)
    DR_10   = 0x0A,                                                 //!< US: SF10 500KHz (downlinks only)
    DR_11   = 0x0B,                                                 //!< US: SF9 500KHz (downlinks only)
    DR_12   = 0x0C,                                                 //!< US: SF8 500KHz (downlinks only)
    DR_13   = 0x0D,                                                 //!< US: SF7 500KHz (downlinks only)
    DR_ADR  = 0xF0,                                                 //!< ADR (Adaptive DataRate and Power Setting)
    DR_AUTO = 0xF1,                                                 //!< Automatic DataRate (for joining only)
} RadioDataRate_e;

/* Radio TX Power (PWR_...) - values remain unchanged */
typedef enum {
    PWR_ADR                 = 0xF0,                                 //!< ADR (Automatic Datarate Adaption)
    PWR_MAX                 = 0x00,                                 //!< 15dBm for SX1261, 22dBm for SX1262
    PWR_ATT_2dB             = 0x01,                                 //!< 14dBm for SX1261, 20dBm for SX1262
    PWR_ATT_4dB             = 0x02,                                 //!< 12dBm for SX1261, 18dBm for SX1262
    PWR_ATT_6dB             = 0x03,                                 //!< 10dBm for SX1261, 16dBm for SX1262
    PWR_ATT_8dB             = 0x04,                                 //!< 8dBm for SX1261, 14dBm for SX1262
    PWR_ATT_10dB            = 0x05,                                 //!< 6dBm for SX1261, 12dBm for SX1262
    PWR_ATT_12dB            = 0x06,                                 //!< 4dBm for SX1261, 10dBm for SX1262
    PWR_ATT_14dB            = 0x07,                                 //!< 2dBm for SX1261,  8dBm for SX1262
    PWR_ATT_16dB            = 0x08,                                 //!< 0dBm for SX1261,  6dBm for SX1262
    PWR_ATT_18dB            = 0x09,                                 //!< -2dBm for SX1261,  4dBm for SX1262
    PWR_ATT_20dB            = 0x0A,                                 //!< -4dBm for SX1261,  2dBm for SX1262
    PWR_ATT_22dB            = 0x0B,                                 //!< -6dBm for SX1261,  0dBm for SX1262
    PWR_ATT_24dB            = 0x0C,                                 //!< -8dBm for SX1261, -2dBm for SX1262
    PWR_ATT_26dB            = 0x0D,                                 //!< -10dBm for SX1261, -4dBm for SX1262
    PWR_ATT_28dB            = 0x0E,                                 //!< -12dBm for SX1261, -6dBm for SX1262
    PWR_ATT_30dB            = 0x0F,                                 //!< -14dBm for SX1261, -8dBm for SX1262
    PWR_ATT_32dB            = 0x10,                                 //!< -16dBm for SX1261, -9dBm for SX1262
    PWR_MIN                 = 0x11,                                 //!< -17dBm for SX1261, -9dBm for SX1262
} RadioTXpower_e;

/* Radio Sub-Bands */
typedef enum {
    US_SUB_BAND_1           = 0,                                    //!< US sub-band 1: channels 0–7 (≈902.3–903.7 MHz)
    US_SUB_BAND_2           = 1,                                    //!< US sub-band 2: channels 8–15 (≈903.9–905.3 MHz)
    US_SUB_BAND_3           = 2,                                    //!< US sub-band 3: channels 16–23 (≈905.5–906.9 MHz)
    US_SUB_BAND_4           = 3,                                    //!< US sub-band 4: channels 24–31 (≈907.1–908.5 MHz)
    US_SUB_BAND_5           = 4,                                    //!< US sub-band 5: channels 32–39 (≈908.7–910.1 MHz)
    US_SUB_BAND_6           = 5,                                    //!< US sub-band 6: channels 40–47 (≈910.3–911.7 MHz)
    US_SUB_BAND_7           = 6,                                    //!< US sub-band 7: channels 48–55 (≈911.9–913.3 MHz)
    US_SUB_BAND_8           = 7,                                    //!< US sub-band 8: channels 56–63 (≈913.5–914.9 MHz)
    US_SUB_BAND_NONE        = 14,                                   //!< No US sub-band selected
    US_SUB_BANDS_ALL        = 15,                                   //!< All US sub-bands enabled
 
    AU_SUB_BAND_1           = 0,                                    //!< AU sub-band 1:  channels 0–7   (≈915.20 – 915.34 MHz)
    AU_SUB_BAND_2           = 1,                                    //!< AU sub-band 2:  channels 8–15  (≈915.36 – 905.3  MHz)
    AU_SUB_BAND_3           = 2,                                    //!< AU sub-band 3:  channels 16–23 (≈915.52 – 906.9  MHz)
    AU_SUB_BAND_4           = 3,                                    //!< AU sub-band 4:  channels 24–31 (≈915.68 – 908.5  MHz)
    AU_SUB_BAND_5           = 4,                                    //!< AU sub-band 5:  channels 32–39 (≈908.84  – 910.1  MHz)
    AU_SUB_BAND_6           = 5,                                    //!< AU sub-band 6:  channels 40–47 (≈910.90  – 911.7  MHz)
    AU_SUB_BAND_7           = 6,                                    //!< AU sub-band 7:  channels 48–55 (≈911.06  – 913.3  MHz)
    AU_SUB_BAND_8           = 7,                                    //!< AU sub-band 8:  channels 56–63 (≈913.22 – 914.9  MHz)
    AU_SUB_BAND_10          = 9,                                    //!< AU sub-band 10: channels 72–79 (≈913.38 – 914.9  MHz)
    AU_SUB_BAND_NONE        = 14,                                   //!< No AU sub-band selected
    AU_SUB_BANDS_ALL        = 15,                                   //!< All AU sub-bands enabled

    EU_SUB_BANDS_DEFAULT    = 0,                                    //!< Default EU channel mask (typically 868.1, 868.3, 868.5 MHz)
    EU_SUB_BAND1_ON         = 1,                                    //!< Not implemented, reserved for future use
    EU_SUB_BAND1_OFF        = 2,                                    //!< Not implemented, reserved for future use
    EU_SUB_BAND2_ON         = 3,                                    //!< Not implemented, reserved for future use
    EU_SUB_BAND2_OFF        = 4                                     //!< Not implemented, reserved for future use
} RadioSubBands_e;

/* Idle Mode for M0+ */
typedef enum IdleMode_e {
    M0_ACTIVE               = 0x0,                                  //!< Keep M0+ active during system idle
    M0_SLEEP                = 0x1,                                  //!< Put M0+ in Sleep mode during system idle
    M0_DEEP_SLEEP           = 0x2,                                  //!< Put M0+ in DeepSleep mode during system idle
} IdleMode_e;

/* Wait Mode for M4 */
typedef enum WaitMode_e {
    M4_NO_WAIT              = 0x0,                                  //!< Do not wait till stack finished
    M4_WAIT_ACTIVE          = 0x1,                                  //!< Wait while stack busy, M4 remains Active
    M4_WAIT_SLEEP           = 0x2,                                  //!< M4 goes into Sleep while stack is busy
    M4_WAIT_DEEP_SLEEP      = 0x3,                                  //!< M4 goes into DeepSleep while stack is busy
} WaitMode_e;

/* LoRaWAN Class */
typedef enum LwClass_e {
    LW_CLASS_A              = 0x00,
    LW_CLASS_B              = 0x01,
    LW_CLASS_C              = 0x02,
} LwClass_e;

/* MAC Message Types */
typedef enum MessageType_e {
    NO_MESSAGE              = 0x00,
    MT_CLASS_A              = 0x01,
    MT_CLASS_B              = 0x02,
    MT_CLASS_C              = 0x03,
    MT_MULTICAST01          = 0x10,
    MT_MULTICAST02          = 0x11,
    MT_MULTICAST03          = 0x12,
    MT_MULTICAST04          = 0x13,
} MessageType_e;

/* Nonce Modes */
typedef enum NonceMode_e {
    NONCE_RANDOM            = 0x00,                                 //!< Random DevNonce/JoinNonce
    NONCE_INCR_SAVE         = 0x01,                                 //!< Increment and save to EEPROM (LoRaWAN 1.0.4)
    NONCE_INCR              = 0x02,                                 //!< Increment; resets on power cycle
} NonceMode_e;

/* Multicast Session Configuration */

typedef enum
{
    T_UNIT_1SEC             = 0,
    T_UNIT_10SEC            = 1,
    T_UNIT_1MIN             = 2,
    T_UNIT_10MIN            = 3,
    T_UNIT_1HOUR            = 4,
    T_UNIT_10HOUR           = 5,
    T_UNIT_1DAY             = 6,
    T_UNIT_10DAY            = 7
} TimeScale_e;

// Bit layout: [7:5] Unit | [4:0] Timeout
// Timeout = Timeout × Unit scale:
// 0: sec, 1: 10 sec, 2: min, 3: 10 min,
// 4: hour, 5: 10 hour, 6: day, 7: 10 day
typedef union
{
    uint8_t Value : 8;
    struct
    {
        uint8_t                 Timeout     : 5;   // 0..31
        TimeScale_e             Unit        : 3;   // 0..7
    };
} Timeout53_t;

typedef struct __attribute__((packed)) {
    uint32_t                Frequency;                              //!< RX frequency (Hz)
    DateTime_t              Start;                                  //!< Session start time
    Timeout53_t             Timeout53;                              //!< Timeout for Class C sessions
    RadioDataRate_e         DataRate    : 8;                        //!< Data rate index
} MultiCastSession_t;

/* Multicast Group Configuration */
typedef struct {
    uint32_t                DevAddr;
    Arr16B_t                NwkSkey;
    Arr16B_t                AppSkey;
    MultiCastSession_t      Session;
} MulticastGroup_t;

/* Default Frequency and Class-C Settings */
#define FrequencyDefault    0xFFFFFFF0
#define ClassCdefault       { .Frequency = FrequencyDefault, .Start = MCstartDirect, .Timeout53 = MCtimeOutInfinite, .DataRate = DR_AUTO }

typedef union {
    uint32_t            TimeStamp;                              //! Timestamp when the uplink has a free channel (use GetTimeStamp function)
    DateTime_t          DateTime;                               //! Date and time when the uplink has a free channel (use GetDateTime function)
} Time_t;

/**
 * @brief Structure containing information about the uplink transmission.
 */
typedef struct UplinkInfo_t {
    uint8_t             Channel;                                    //! Channel number used for the uplink transmission
    RadioDataRate_e     Datarate;                                   //! Radio data rate used for the transmission
    Time_t              TimeFree;                                   //! Time when the uplink has a free channel
    uint8_t             MaxFrmPayloadSize;                          //! The maximum available payload size (limited to 222 if a repeater is used). Good practice: save some bytes (+/- 5) for MAC commands every now and then
    bool                HasFreeChannel;                             //! Flag indicating whether a free channel is available for transmission
} UplinkInfo_t;

/* Core Configuration Union */
typedef union {
    struct __attribute__((packed)) {
        struct __attribute__((packed)) {
            bool                 Confirmed              : 1;
            bool                 DutyCycleDisregard     : 1;        //!< Disable DutyCycle Enforcement (use only in controlled test environments)
            uint8_t                                     : 6;
            RadioDataRate_e      DataRate               : 8;
            RadioTXpower_e       Power                  : 8;
            uint8_t              FPort                  : 8;
        } TX;
        struct __attribute__((packed)) {
            LoRaWAN_keys_t      *KeysPtr;
            RadioDataRate_e      DataRate               : 8;        //!< Not used for US version (defined by LoRaWAN spec)
            RadioTXpower_e       Power                  : 8;
            uint8_t              MaxAttempts            : 8;
            uint8_t              SubBand_1st            : 4;
            uint8_t              SubBand_2nd            : 4;
        } Join;
        struct __attribute__((packed)) {
            struct {
                IdleMode_e     Mode                     : 2;        //!< M0 Idle Mode: Active, Sleep, or DeepSleep
                bool           BleEcoON                 : 1;        //!< BLE ECO remains ON during idle
                bool           DebugON                  : 1;        //!< Debug Port active during idle (for debugging)
                uint8_t                                 : 4;        //!< Reserved
            } Idle;
            NonceMode_e        NonceMode                : 4;        //!< Nonce Mode: Random (LoRaWAN < 1.0.4), Incremental Save, or Incremental
        } System;
        struct __attribute__((packed)) {
            bool               Boost                    : 1;
            LwClass_e          Class                    : 2;        //!< LoRaWAN Class: A, B, or C
            uint8_t                                     : 5;        //!< Reserved
        } RX;
        struct __attribute__((packed)) {
            MulticastGroup_t    *MultiCastGroupPtr[4];
        } MultiCastPtrs;
    };
    uint8_t reserved[32];
} CoreConfiguration_t;


/* Parameter Error Codes */
typedef enum ParamErrors_e {
    PARAM_OK                    = 0x00,
    PARAM_INVALID               = 0x01,
    PARAM_OUT_OF_RANGE          = 0x02,
    PARAM_UNDEFINED_ERROR       = 0xFA,
} ParamErrors_e;

/* Radio Error Codes */
typedef enum RadioErrors_e {
    RADIO_OK                    = 0x00,                             //!< Operation completed successfully
    RADIO_BUSY_ERROR            = 0x01,                             //!< Radio Busy Error
    RADIO_BUCK_START_ERROR      = 0x02,                             //!< Buck converter failed to start
    RADIO_XOSC_START_ERROR      = 0x03,                             //!< XOSC failed to start
    RADIO_RC13M_CALIB_ERROR     = 0x04,                             //!< RC 13MHz calibration failed
    RADIO_RC64K_CALIB_ERROR     = 0x05,                             //!< RC 64kHz calibration failed
    RADIO_PLL_CALIB_ERROR       = 0x06,                             //!< PLL calibration failed
    RADIO_PLL_LOCK_ERROR        = 0x07,                             //!< PLL lock failed
    RADIO_IMG_CALIB_ERROR       = 0x08,                             //!< Image calibration failed
    RADIO_ADC_CALIB_ERROR       = 0x09,                             //!< ADC calibration failed
    RADIO_PA_RAMP_ERROR         = 0x0A,                             //!< PA ramp failed
    RADIO_INVALID_FREQUENCY     = 0x0B,                             //!< Invalid frequency set
    RADIO_TIMEOUT_ERROR         = 0x0C,                             //!< Radio didn't respond (undervoltage?)
    RADIO_UNDEFINED_ERROR       = 0xFA                              //!< Undefined or unknown Radio error
} RadioErrors_e;

/* MAC Error Codes */
typedef enum MacErrors_e {
    MAC_OK                      = 0x00,                             //!< Operation completed successfully
    MAC_BUSY_ERROR              = 0x01,                             //!< MAC layer is busy processing another command
    MAC_NOT_JOINED_ERROR        = 0x02,                             //!< Device has not joined the network
    MAC_CHANNELS_OCCUPIED_ERROR = 0x03,                             //!< All channels are occupied; no free channels available
    MAC_UNRECOGNIZED_KEY_TYPE   = 0x04,                             //!< The provided key type is not recognized
    MAC_EMPTY_PAYLOAD_ERROR     = 0x05,                             //!< No payload provided for the operation
    MAC_RXHEADER_ERROR          = 0x06,                             //!< Error in the received header
    MAC_RX_MIC_ERROR            = 0x07,                             //!< Message Integrity Code (MIC) error in received packet
    MAC_RX_INVALID_DEVADDR      = 0x08,                             //!< The device address in the received packet is invalid
    MAC_INVALID_PACKET_ERROR    = 0x09,                             //!< The received packet is invalid
    MAC_RX_TIMEOUT_ERROR        = 0x0A,                             //!< Reception timed out waiting for a downlink response
    MAC_CRC_ERROR               = 0x0B,                             //!< Cyclic Redundancy Check (CRC) error in received packet
    MAC_FCNT_ERROR              = 0x0C,                             //!< Frame counter error detected
    MAC_CONFIRMATION_ERROR      = 0x0D,                             //!< Error with the confirmed downlink reception
    MAC_PAYLOAD_SIZE_ERROR      = 0x0E,                             //!< The payload size is not within allowed limits
    MAC_JOIN_NONCE_ERROR        = 0x0F,                             //!< Error with the join nonce during network join procedure
    MAC_UNSUPPORTED_CLASS       = 0x10,                             //!< The requested device class is not supported
    MAC_UNDEFINED_ERROR         = 0xFA                              //!< Undefined or unknown MAC error
} MacErrors_e;

/* System Error Codes */
typedef enum SystemErrors_e {
    SYSTEM_OK                   = 0x00,                             //!< Operation completed successfully
    SYSTEM_BUSY_ERROR           = 0x01,                             //!< The system is currently busy and cannot process new requests
    SYSTEM_NOT_STARTED          = 0x02,                             //!< The system has not been initialized or started
    SYSTEM_IPC_ERROR            = 0x03,                             //!< An error occurred during inter-process communication (IPC)
    SYSTEM_EEPROM_WRITE_ERROR   = 0x04,                             //!< Failed to write data to EEPROM
    SYSTEM_VERSION_MATCH_ERROR  = 0x05,                             //!< System version mismatch detected
    SYSTEM_UNDEFINED_ERROR      = 0xFA                              //!< An undefined or unknown system error occurred
} SystemErrors_e;

/*!
 * \brief Bitfield flags indicating which MAC commands were received
 *        (Check Status.Mac.MacCmdReceived read using OTX18_GetMacCmd() )
 */
typedef union
{
    uint32_t Value;
    struct
    {
        uint32_t RX_LINK_CHECK_ANS        : 1;  //!< Received LinkCheckAns (CID 0x02)
        uint32_t RX_DEVICE_TIME_ANS       : 1;  //!< Received DeviceTimeAns (CID 0x0D)
        uint32_t                          : 14; //!< Reserved
        uint32_t RX_LINK_ADR_REQ          : 1;  //!< Received LinkADRReq (CID 0x03)
        uint32_t RX_DUTY_CYCLE_REQ        : 1;  //!< Received DutyCycleReq (CID 0x04)
        uint32_t RX_PARAM_SETUP_REQ       : 1;  //!< Received RXParamSetupReq (CID 0x05)
        uint32_t RX_DEV_STATUS_REQ        : 1;  //!< Received DevStatusReq (CID 0x06)
        uint32_t RX_NEW_CHANNEL_REQ       : 1;  //!< Received NewChannelReq (CID 0x07)
        uint32_t RX_RX_TIMING_SETUP_REQ   : 1;  //!< Received RXTimingSetupReq (CID 0x08)
        uint32_t RX_TX_PARAM_SETUP_REQ    : 1;  //!< Received TXParamSetupReq (CID 0x09)
        uint32_t RX_DL_CHANNEL_REQ        : 1;  //!< Received DlChannelReq (CID 0x0A)
        uint32_t                          : 9;  //!< Reserved
    };
} ReceivedMacCmd_t;

/* Parameter Status Union */
typedef union {
    struct {
        ParamErrors_e   ErrorStatus : 8;                            //!< Parameter Errors
        uint8_t                     : 8;                            //!< Reserved
    };
    uint8_t Reserved[16];
} ParameterStatus_t;

/* Radio Status Union */
typedef union {
    struct {                            
        RadioErrors_e   ErrorStatus   : 8;                          //!< Radio Errors
        uint8_t                       : 6;                          //!< Reserved
        bool            IsConfigured  : 1;                          //!< Radio configured?
        bool            IsBusy        : 1;                          //!< Radio busy?
    };                          
    uint8_t Reserved[16];
} RadioStatus_t;

/* MAC Status Union */
typedef union {
    struct {
        MacErrors_e     ErrorStatus         : 8;                    //!< MAC Errors
        uint8_t         BytesToRead         : 8;                    //!< Total bytes in RX Buffer
        MessageType_e   MessageReceived     : 8;                    //!< Message type received (cleared after read)
        bool            DownlinkReceived    : 1;                    //!< Valid downlink received
        bool            MacCmdReceived      : 1;                    //!< MAC command received
        bool            ConfDown            : 1;                    //!< Confirmed downlink received
        bool            IsConfigured        : 1;                    //!< MAC configured?
        bool            IsJoined            : 1;                    //!< Device joined?
        bool            IsBusy              : 1;                    //!< MAC busy?
        bool            IsPublicNetwork     : 1;                    //!< True for public network
        bool                                : 1;                    //!< Reserved
        uint32_t        DevAddr;                                    //!< Device address from join
        uint8_t         Margin;                                     //!< Demodulation margin (dB)
        uint8_t         GwCnt;                                      //!< Number of gateways received
        uint8_t         Fport;                                      //!< Port number of last packet
        uint8_t         ActiveMulticast;                            //!< The currentlu active Multicast session ID ( 0..3, 0xFF = none )
    };
    uint8_t Reserved[16];
} MacStatus_t;

/* System Status Union */
typedef union {
    struct {
        uint32_t       Version;                                     //!< Stack version
        SystemErrors_e ErrorStatus          : 8;                    //!< System Errors
        uint8_t        BatteryLevel;                                //!< Battery level
        uint8_t                             : 4;                    //!< Reserved
        bool           BreakCurrentFunction : 1;                    //!< Break execution
        bool           IsStarted            : 1;                    //!< System started?
        bool           IsBusy               : 1;                    //!< System busy?
        bool           IsSleeping           : 1;                    //!< System sleeping?
    };  
    uint8_t Reserved[16];   
} SystemStatus_t;

/* Core Status Structure */
typedef struct {
    ParameterStatus_t       Parameters;
    RadioStatus_t           Radio;
    MacStatus_t             Mac;
    SystemStatus_t          System;
} CoreStatus_t;

/* Error Status Union */
typedef union {
    uint32_t ErrorValue;
    struct __attribute__((packed)) {
        ParamErrors_e       ParamErrors         : 8;
        RadioErrors_e       RadioErrors         : 8;
        MacErrors_e         MacErrors           : 8;
        SystemErrors_e      SystemErrors        : 8;
    };
} ErrorStatus_t;

/* Core Arguments Structure */
typedef volatile struct {
    CoreFunctions_e         Function;
    CoreConfiguration_t     *ConfigurationPtr;
    CoreStatus_t            Status;
    uint32_t                Arg1;
    uint32_t                Arg2;
    uint32_t                Arg3;
    uint32_t                Arg4;
} CoreArguments_t;

#define ERRORSTATUS_NO_ERROR     0

/* Wake-up Pin Configuration */
typedef struct {
    bool        Enabled             : 1;
    bool        RisingEdge          : 1;
    bool        InternalPullUpDown  : 1;
    uint32_t                        : 21;
} WakeUpPin_t;

/* Wake-up Time Configuration */
typedef struct {
    bool        Enabled     : 1;
    bool        IsDateTime  : 1;
    uint8_t                 : 6;
    union {
        DateTime_t      DateTime;
        struct {
            uint8_t     Days;                                       //!< Delay in days
            uint8_t     Hours;                                      //!< Delay in hours
            uint8_t     Minutes;                                    //!< Delay in minutes
            uint8_t     Seconds;                                    //!< Delay in seconds
        };
    };
} WakeUpTime_t;

/* Sleep Mode Enumeration */
typedef enum SleepMode_e {
    MODE_SLEEP              = 0x1,
    MODE_DEEP_SLEEP         = 0x2,
    MODE_HIBERNATE          = 0x3,                                  //!< Hibernate: WCO & RTC off
    MODE_HIBERNATE_RTC_ON   = 0x4,                                  //!< Hibernate with RTC on (~0.7uA extra)
    MODE_HIBERNATE_MACSAVE  = 0x5,                                  //!< Hibernate, MAC saved to EEPROM
} SleepMode_e;

/* Sleep Cores Enumeration */
typedef enum SleepCores_e {
    CORES_M0P               = 1,
    CORES_M4                = 2,
    CORES_BOTH              = 3
} SleepCores_e;

/* User Uplink MAC Command Enumeration */
typedef enum MacCmd_e {
    MACCMD_NONE             = 0,
    LINK_CHECK_REQ          = 1,
    DEVICE_TIME_REQ         = 2
} MacCmd_e;

/*!
 * ============================================================================
 * Crypto Enumerations
 * ============================================================================
 */

/** Defines the direction of the Crypto methods */
typedef enum
{
    CRYPTO_ENCRYPT            = 0x00u,   /**< The forward mode, plain text will be encrypted into cipher text */
    CRYPTO_DECRYPT            = 0x01u    /**< The reverse mode, cipher text will be decrypted into plain text */
} crypto_dir_mode_t;

/** The key length options for the AES method. */
typedef enum
{
    CRYPTO_KEY_AES_128        = 0x00u,   /**< The AES key size is 128 bits */
    CRYPTO_KEY_AES_192        = 0x01u,   /**< The AES key size is 192 bits */
    CRYPTO_KEY_AES_256        = 0x02u    /**< The AES key size is 256 bits */
} crypto_aes_key_length_t;

/** Defines modes of SHA method */
typedef enum
{
    CRYPTO_MODE_SHA1          = 0x00u,   /**< Sets the SHA1 mode */
    CRYPTO_MODE_SHA224        = 0x01u,   /**< Sets the SHA224 mode */
    CRYPTO_MODE_SHA256        = 0x02u,   /**< Sets the SHA256 mode */
    CRYPTO_MODE_SHA384        = 0x03u,   /**< Sets the SHA384 mode */
    CRYPTO_MODE_SHA512        = 0x04u,   /**< Sets the SHA512 mode */
    CRYPTO_MODE_SHA512_256    = 0x05u,   /**< Sets the SHA512/256 mode */
    CRYPTO_MODE_SHA512_224    = 0x06u,   /**< Sets the SHA512/224 mode */
} crypto_sha_mode_t;

/*!
 * ============================================================================
 * Low Level Radio Settings
 * ============================================================================
 */

/*!
 * \brief Represents the possible packet type (i.e. modem) used
 */
typedef enum
{
    PACKET_TYPE_GFSK                        = 0x00,
    PACKET_TYPE_LORA                        = 0x01,
    PACKET_TYPE_NONE                        = 0x0F,
} RadioPacketTypes_e;


/* ==== The LoRa Radio Settings  ====  */
 
/*!
 * \brief Represents the possible spreading factor values in LoRa packet types
 */
typedef enum
{
    LORA_SF5                        = 0x05,
    LORA_SF6                        = 0x06,
    LORA_SF7                        = 0x07,
    LORA_SF8                        = 0x08,
    LORA_SF9                        = 0x09,
    LORA_SF10                       = 0x0A,
    LORA_SF11                       = 0x0B,
    LORA_SF12                       = 0x0C,
} RadioLoRaSpreadingFactors_e;

/*!
 * \brief Represents the bandwidth values for LoRa packet type
 */
typedef enum
{
    LORA_BW_500                     = 6,
    LORA_BW_250                     = 5,
    LORA_BW_125                     = 4,
    LORA_BW_062                     = 3,
    LORA_BW_041                     = 10,
    LORA_BW_031                     = 2,
    LORA_BW_020                     = 9,
    LORA_BW_015                     = 1,
    LORA_BW_010                     = 8,
    LORA_BW_007                     = 0,
} RadioLoRaBandwidths_e;

/*!
 * \brief Represents the coding rate values for LoRa packet type
 */
typedef enum
{
    LORA_CR_4_5                     = 0x01,
    LORA_CR_4_6                     = 0x02,
    LORA_CR_4_7                     = 0x03,
    LORA_CR_4_8                     = 0x04,
} RadioLoRaCodingRates_e;

typedef enum
{
    LORA_LOWDATARATEOPTIMIZE_OFF    = 0x00,
    LORA_LOWDATARATEOPTIMIZE_ON     = 0x01
} RadioLoraLowDataRateOptimize_e;

/*!
 * \brief Holds the Radio lengths mode for the LoRa packet type
 */
typedef enum
{
    LORA_PACKET_VARIABLE_LENGTH     = 0x00,                         //!< The packet is on variable size, header included
    LORA_PACKET_FIXED_LENGTH        = 0x01,                         //!< The packet is known on both sides, no header included in the packet
    LORA_PACKET_EXPLICIT            = LORA_PACKET_VARIABLE_LENGTH,
    LORA_PACKET_IMPLICIT            = LORA_PACKET_FIXED_LENGTH,
} RadioLoRaHeaderType_e;

/*!
 * \brief Represents the CRC mode for LoRa packet type
 */
typedef enum
{
    LORA_CRC_ON                             = 0x01,         //!< CRC activated
    LORA_CRC_OFF                            = 0x00,         //!< CRC not used
} RadioLoRaCrcMode_e;

/*!
 * \brief Represents the IQ mode for LoRa packet type
 */
typedef enum
{
    LORA_IQ_NORMAL                          = 0x00,
    LORA_IQ_INVERTED                        = 0x01,
} RadioLoRaIQMode_e;

typedef enum {
	LORA_MAC_PRIVATE_SYNCWORD               = 0x1424,
	LORA_MAC_PUBLIC_SYNCWORD                = 0x3444
} LoRaSyncWord_e;

typedef struct {
    RadioLoRaSpreadingFactors_e			SF                      : 8;
	RadioLoRaBandwidths_e				BW                      : 8;
	RadioLoRaCodingRates_e				CR                      : 8;
    RadioLoraLowDataRateOptimize_e      LowDataRateOptimize     : 8;
 }  LoRaModulationParams_t;

typedef struct {
    uint16_t                            PreambleLength;
    RadioLoRaHeaderType_e               HeaderType              : 8;
    uint8_t                             PayloadSize;                    //!< Size of the payload (in bytes) to transmit or maximum size of the payload that the receiver can accept.
    RadioLoRaCrcMode_e                  CRCmode                 : 8;
    RadioLoRaIQMode_e                   IQmode                  : 8;
    uint16_t                            SyncWord;                       //!< The SX126x LoRa syncword setting
}  LoRaPacketParams_t;

/* ==== The FSK Radio Settings  ==== */

/*!
 * \brief Represents the modulation shaping parameter
 */
typedef enum
{
    MOD_SHAPING_OFF                         = 0x00,
    MOD_SHAPING_G_BT_03                     = 0x08,
    MOD_SHAPING_G_BT_05                     = 0x09,
    MOD_SHAPING_G_BT_07                     = 0x0A,
    MOD_SHAPING_G_BT_1                      = 0x0B,
} RadioFSKModShapings_e;

/*!
 * \brief Represents the modulation shaping parameter
 */
typedef enum
{
    RX_BW_4800                              = 0x1F,
    RX_BW_5800                              = 0x17,
    RX_BW_7300                              = 0x0F,
    RX_BW_9700                              = 0x1E,
    RX_BW_11700                             = 0x16,
    RX_BW_14600                             = 0x0E,
    RX_BW_19500                             = 0x1D,
    RX_BW_23400                             = 0x15,
    RX_BW_29300                             = 0x0D,
    RX_BW_39000                             = 0x1C,
    RX_BW_46900                             = 0x14,
    RX_BW_58600                             = 0x0C,
    RX_BW_78200                             = 0x1B,
    RX_BW_93800                             = 0x13,
    RX_BW_117300                            = 0x0B,
    RX_BW_156200                            = 0x1A,
    RX_BW_187200                            = 0x12,
    RX_BW_234300                            = 0x0A,
    RX_BW_312000                            = 0x19,
    RX_BW_373600                            = 0x11,
    RX_BW_467000                            = 0x09,
} RadioFSKRxBandwidth_e;

/*!
 * \brief Represents the modulation shaping parameter
 */
// Values can be added with the formula: BR = 1024000000 / BitRate 
typedef enum
{
    BITRATE_25K                             = 40960,
    BITRATE_50K                             = 20480,
    BITRATE_100K                            = 10240,
    INTERNAL_BR_MAX                         = 0xFFFFFF
} RadioFSKRBitrate_e;

/*!
 * \brief Represents the modulation shaping parameter
 */
// Values can be added with the formula: Fdev = (Frequency Deviation * 2^25) / Fxtal            Fdev = Frequency Deviation * 1.048576
typedef enum
{
    FDEV_SSB_25K                            = 26214,
    FDEV_DSB_50K                            = 26214,
    FDEV_SSB_30K                            = 31457,
    FDEV_DSB_60K                            = 31457,
    FDEV_SSB_50K                            = 52429,
    FDEV_DSB_100K                           = 52429,
    INTERNAL_DEV_MAX                        = 0xFFFFFF
} RadioFSKRDeviation_e;


/*!
 * \brief Represents the preamble length used to detect the packet on Rx side
 */
typedef enum
{
    RADIO_PREAMBLE_DETECTOR_OFF             = 0x00,         //!< Preamble detection length off
    RADIO_PREAMBLE_DETECTOR_08_BITS         = 0x04,         //!< Preamble detection length 8 bits
    RADIO_PREAMBLE_DETECTOR_16_BITS         = 0x05,         //!< Preamble detection length 16 bits
    RADIO_PREAMBLE_DETECTOR_24_BITS         = 0x06,         //!< Preamble detection length 24 bits
    RADIO_PREAMBLE_DETECTOR_32_BITS         = 0x07,         //!< Preamble detection length 32 bit
} RadioFSKPreambleDetection_e;

/*!
 * \brief Represents the possible combinations of SyncWord correlators activated
 */
typedef enum
{
    RADIO_ADDRESSCOMP_FILT_OFF              = 0x00,         //!< No correlator turned on, i.e. do not search for SyncWord
    RADIO_ADDRESSCOMP_FILT_NODE             = 0x01,
    RADIO_ADDRESSCOMP_FILT_NODE_BROAD       = 0x02,
} RadioFSKAddressComp_e;

/*!
 *  \brief Radio GFSK packet length mode
 */
typedef enum
{
    RADIO_PACKET_FIXED_LENGTH               = 0x00,         //!< The packet is known on both sides, no header included in the packet
    RADIO_PACKET_VARIABLE_LENGTH            = 0x01,         //!< The packet is on variable size, header included
} RadioFSKPacketLengthModes_e;

/*!
 * \brief Radio whitening initial seed value
 */
typedef enum
{
    RADIO_WHITENINGSEED                     = 0x01FF,
} RadioFSKWhiteningSeed_e;

/*!
 * \brief Radio whitening mode activated or deactivated
 */
typedef enum
{
    RADIO_DC_FREE_OFF                       = 0x00,
    RADIO_DC_FREEWHITENING                  = 0x01,
} RadioFSKDcFree_e;

/*!
 * \brief Represents the CRC length
 */
typedef enum
{
    RADIO_CRC_OFF                           = 0x01,         //!< No CRC in use
    RADIO_CRC_1_BYTES                       = 0x00,
    RADIO_CRC_2_BYTES                       = 0x02,
    RADIO_CRC_1_BYTES_INV                   = 0x04,
    RADIO_CRC_2_BYTES_INV                   = 0x06,
    RADIO_CRC_2_BYTES_IBM                   = RADIO_CRC_2_BYTES,
    RADIO_CRC_2_BYTES_CCIT                  = RADIO_CRC_2_BYTES_INV,
} RadioFSKCrcTypes_e;

/*!
 * \brief LFSR initial value to compute the FSK CRC
 */
typedef enum
{
    RADIO_CRC_IBM_SEED                      = 0xFFFF,   //!< FSR initial value to compute IBM type CRC
    RADIO_CRC_CCITT_SEED                    = 0x1D0F,   //!< FSR initial value to compute CCIT type CRC
} RadioFSKCrcSeed_e;

/*!
 * \brief Polynomial used to compute the FSK CRC
 */
typedef enum
{
    RADIO_CRC_POLYNOMIAL_IBM                = 0x8005,   //!< Polynomial to compute IBM type CRC
    RADIO_CRC_POLYNOMIAL_CCITT              = 0x1021,   //!< Polynomial value to compute CCIT type CRC
} RadioFSKCrcPolynomial_e;

typedef struct {
    RadioFSKRBitrate_e			        BitRate                 : 32;
	RadioFSKRDeviation_e			    Fdev                    : 32;
	RadioFSKModShapings_e				PulseShape              : 8;
	RadioFSKRxBandwidth_e				RxBandwidth             : 8;
 }  FSKModulationParams_t;

typedef struct {
    uint16_t                            PreambleLength;
    RadioFSKPreambleDetection_e         PreambleDetectorLength  : 8;
    Arr8B_t                             SyncWord;                                       //!< The SX126x FSK syncword setting
    uint8_t                             SyncWordLength          : 8;                    //!< Size of the SyncWord in bits.
    RadioFSKAddressComp_e               AddrComp                : 8;
    RadioFSKPacketLengthModes_e         PacketType              : 8;
    uint8_t                             PayloadLength;                                  //!< Size of the payload (in bytes) to transmit or maximum size of the payload that the receiver can accept.
    RadioFSKCrcTypes_e                  CRCType                 : 8;
    RadioFSKCrcSeed_e                   CrcSeed                 : 16;                   //!< LFSR initial value to compute the FSK CRC
    RadioFSKCrcPolynomial_e             CrcPolynomial           : 16;                   //!< Polynomial used to compute the FSK CRC
    RadioFSKDcFree_e                    Whitening               : 8;
    RadioFSKWhiteningSeed_e             WhiteningSeed           : 16;
}  FSKPacketParams_t;


/* ==== The Radio Settings  ==== */

typedef struct {
    uint32_t                            Frequency;
    RadioTXpower_e                     TXpower;                                        //!< TXpower (PWR = MAX - (2 * value)), not used in receive mode
    union
    {
        struct
        {
            LoRaModulationParams_t              Modulation;
            LoRaPacketParams_t                  Packet;
        } LoRa;
        struct
        {
            FSKModulationParams_t              Modulation;
            FSKPacketParams_t                  Packet;
        } FSK;
    };
    RadioPacketTypes_e                  PacketType;
    uint8_t                             RXboost                 : 1;                    //!< RX Boost mode
    uint8_t                                                     : 7;                    //!< Reserved
}  RadioParams_t;

/*!
 * \brief Represents the possible device error states
 */
typedef union
{
    uint8_t Value;
    struct
    {   //bit order is lsb -> msb
        uint8_t				    : 1;  //!< Reserved
        uint8_t CmdStatus		: 3;  //!< Command status
        uint8_t ChipMode		: 3;  //!< Chip mode
        uint8_t CpuBusy		    : 1;  //!< Flag for CPU radio busy
    };
} ChipStatus_t;

/*!
 * \brief Represents the possible IRQ states
 */
typedef union
{
    uint16_t Value;
    struct
    {
        uint8_t IRQ_TX_DONE					    : 1;                    //!< Radio received TX DONE IRQ
        uint8_t IRQ_RX_DONE					    : 1;                    //!< Radio received RX DONE IRQ
        uint8_t IRQ_PREAMBLE_DETECTED			: 1;                    //!< Radio received PREAMBLE DETECTED IRQ
        uint8_t IRQ_SYNCWORD_VALID			    : 1;                    //!< Radio received SYNCWORD VALID IRQ
        uint8_t IRQ_HEADER_VALID				: 1;                    //!< Radio received HEADER VALID IRQ
        uint8_t IRQ_HEADER_ERROR				: 1;                    //!< Radio received HEADER ERROR IRQ
        uint8_t IRQ_CRC_ERROR					: 1;                    //!< Radio received CRC ERROR IRQ
        uint8_t IRQ_CAD_DONE					: 1;                    //!< Radio received CAD DONE IRQ
        uint8_t IRQ_CAD_ACTIVITY_DETECTED		: 1;                    //!< Radio received CAD ACTIVITY DETECTED IRQ
        uint8_t IRQ_RX_TX_TIMEOUT				: 1;                    //!< Radio received RX TX TIMEOUT IRQ
        uint8_t								    : 6;                    //!< Reserved
    };
} IrqStatus_t;

/*!
 * \brief Represents the possible device error states
 */
typedef union
{
    uint16_t Value;
    struct
    {
        uint8_t Rc64kCalib              : 1;                    //!< RC 64kHz oscillator calibration failed
        uint8_t Rc13mCalib              : 1;                    //!< RC 13MHz oscillator calibration failed
        uint8_t PllCalib                : 1;                    //!< PLL calibration failed
        uint8_t AdcCalib                : 1;                    //!< ADC calibration failed
        uint8_t ImgCalib                : 1;                    //!< Image calibration failed
        uint8_t XoscStart               : 1;                    //!< XOSC oscillator failed to start
        uint8_t PllLock                 : 1;                    //!< PLL lock failed
        uint8_t BuckStart               : 1;                    //!< Buck converter failed to start
        uint8_t PaRamp                  : 1;                    //!< PA ramp failed
        uint8_t                         : 7;                    //!< Reserved
    };
} DeviceErrors_t;

/*!
 * \brief Represents the packet status for every packet type
 */
typedef union
{
	uint32_t value;
	struct {
		int8_t RssiPkt;                                //!< The RSSI of the last packet
	    int8_t SnrPkt;                                 //!< The SNR of the last packet multiplied by 4
        int8_t SignalRssiPkt;                          //!< The RSSI of the LoRa signal of the last packet
	};
} LoRaPacketStatus_t;

/*!
 * \brief Represents the LoRa Packet statistics
 */
typedef struct  __attribute__((scalar_storage_order("big-endian"))) 
{
	uint16_t NbPktReceived;
    uint16_t NbPktCrcErr;
    uint16_t NbPktHeaderErr;
} LoRaStats_t;

typedef struct 
{
    ChipStatus_t                    ChipStatus;                 //!< The device error states
    IrqStatus_t                     IrqStatus;                  //!< The IRQ states
    DeviceErrors_t                  DeviceErrors;               //!< The device error states
    LoRaPacketStatus_t              LoRaPacketStatus;           //!< The packet status for every packet type
    LoRaStats_t                     LoRaStats;                  //!< The LoRa Packet statistics
} RadioState_t;

typedef enum {
	RADIO_SYNC_NONE          = 0x00,
	RADIO_SYNC_NOWAIT        = 0x00,
	RADIO_SYNC_WAIT          = 0x01,
	RADIO_SYNC_SET_AT_START  = 0x02,
	RADIO_SYNC_SET_AFTER     = 0x04,
} RadioSyncFlags_t;

typedef struct __attribute__((packed, aligned(4)))
{
	uint8_t             payloadSize;
	RadioSyncFlags_t    syncFlags;
	uint16_t            timeOutMS;
	uint32_t            syncDelayUs;
} RadioPacketArgs_t;

/* Macros for Wake-up Pin/Time Initialization */
#define WakeUpPinHigh(pullDown)                                     { .Enabled = true,  .RisingEdge = true,  .InternalPullUpDown = (pullDown) }
#define wakeUpPinLow(pullUp)                                        { .Enabled = true,  .RisingEdge = false, .InternalPullUpDown = (pullUp) }
#define WakeUpPinOff                                                { .Enabled = false }
#define WakeUpDateTime(_dateTime)                                   { .Enabled = true, .IsDateTime = true,  .DateTime = (_dateTime) }
#define WakeUpDelay(_days, _hours, _minutes, _seconds)              { .Enabled = true, .IsDateTime = false, .Days = (_days), .Hours = (_hours), .Minutes = (_minutes), .Seconds = (_seconds) }
#define WakeUpTimeOff                                               { .Enabled = false }

/* Multicast Group Macros */
#define MultiCastGroupOff                                           ((MulticastGroup_t *) 0x00000000)
#define MultiCastDisable                                            {{ MultiCastGroupOff, MultiCastGroupOff, MultiCastGroupOff, MultiCastGroupOff }}
#define MultiCastGroup(Ptr)                                         {{ (MulticastGroup_t *) (Ptr), MultiCastGroupOff, MultiCastGroupOff, MultiCastGroupOff }}
#define MultiCast2Group(Ptr1, Ptr2)                                 {{ (MulticastGroup_t *) (Ptr1), (MulticastGroup_t *) (Ptr2), MultiCastGroupOff, MultiCastGroupOff }}
#define MultiCast3Group(Ptr1, Ptr2, Ptr3)                           {{ (MulticastGroup_t *) (Ptr1), (MulticastGroup_t *) (Ptr2), (MulticastGroup_t *) (Ptr3), MultiCastGroupOff }}
#define MultiCast4Group(Ptr1, Ptr2, Ptr3, Ptr4)                     {{ (MulticastGroup_t *) (Ptr1), (MulticastGroup_t *) (Ptr2), (MulticastGroup_t *) (Ptr3), (MulticastGroup_t *) (Ptr4) }}

/* Multicast Session Macros */
#define MCstartDirect                                               { .Value = 0 }
#define MCtimeOutInfinite                                           { .Value = 0xFF }
#define MCsession(_Frequency, _Start, _Timeout53, _DataRate)        { .Frequency = _Frequency, .Start = _Start, .Timeout53 = _Timeout53, .DataRate = _DataRate }
#define MCsessionContinuous                                         { .Frequency = FrequencyDefault, .Start = MCstartDirect, .Timeout = MCtimeOutInfinite, .DataRate = DR_AUTO }

/* Sleep Configuration Structure */
typedef struct __attribute__((packed)) {
    WakeUpPin_t  WakeUpPin;                                         //!< 24 bits: Wake-up pin configuration
    WakeUpTime_t WakeUpTime;                                        //!< 40 bits: Wake-up time configuration
    SleepMode_e  SleepMode      : 3;                                //!< Sleep, DeepSleep, or Hibernate mode
    bool         BleEcoON       : 1;                                //!< BLE ECO ON during sleep
    bool         DebugON        : 1;                                //!< Debug Port active during sleep
    SleepCores_e SleepCores     : 3;                                //!< Select which cores will be put in sleep mode
    uint32_t                    : 32;                               //!< Reserved
} SleepConfig_t;                

/* Stack Region Enumeration */
typedef enum {              
    stack_AS        = 1,                                            //!< Asia 923-925 MHz
    stack_AU        = 2,                                            //!< Australia 915-928 MHz
    stack_CN_L      = 3,                                            //!< China 470-510 MHz
    stack_CN_H      = 4,                                            //!< China 779-787 MHz
    stack_EU_L      = 5,                                            //!< Europe 433 MHz
    stack_EU_H      = 6,                                            //!< Europe 863-870 MHz
    stack_IN        = 7,                                            //!< India 865-867 MHz
    stack_KR        = 8,                                            //!< Korea 920-923 MHz
    stack_US        = 9,                                            //!< North America 902-928 MHz
    stack_RU        = 10,                                           //!< Russia 864-870 MHz
} stackRegion_e;                

/* Core Information Structure */
typedef struct __attribute__((packed)) {                
    uint32_t        BuildYear       : 6;                            //!< Firmware build year
    uint32_t        BuildMonth      : 4;                            //!< Firmware build month
    uint32_t        BuildDayOfMonth : 5;                            //!< Firmware build day
    uint32_t        BuildHour       : 5;                            //!< Build hour (24h mode)
    uint32_t        BuildMinute     : 6;                            //!< Build minute
    uint32_t        BuildSecond     : 6;                            //!< Build second
    uint32_t        BuildNumber;                                    //!< Incremental build number
    uint8_t         DevEUI[8];                                      //!< Device EUI
    char            BuildType;                                      //!< Firmware build type
    stackRegion_e   StackRegion     : 8;                            //!< Stack region
    char            StackOption;                                    //!< Stack option: 'S', 'P', 'C'
    char            StackStage;                                     //!< Lifecycle stage: 'a','A','b','B','r','R'
    char            CodeName[16];                                   //!< Code name
} CoreInfo_t;

/* OTX18 Core API Function Prototypes  */

CoreStatus_t   OTX18_Init               (CoreConfiguration_t *CoreConfigurationPtr);
CoreStatus_t   OTX18_GetInfo            (CoreInfo_t *CoreInfo);
CoreStatus_t   OTX18_GetRXdata          (uint8_t *RXdata,  uint8_t MaxLength);
CoreStatus_t   OTX18_GetMacCmd          (ReceivedMacCmd_t *ReceivedMacCmds);

/* LoRaWAN functions */        
CoreStatus_t   OTX18_LW_Join            (WaitMode_e WaitMode);
CoreStatus_t   OTX18_Join_Ext           (uint16_t RX1delayMs, uint32_t RX2freq, RadioDataRate_e RX2dataRate, uint32_t TXfreq, WaitMode_e waitMode);

CoreStatus_t   OTX18_LW_Send            (uint8_t *Buffer,  uint8_t Length, WaitMode_e WaitMode);
CoreStatus_t   OTX18_LW_SendMac         (uint8_t *Buffer,  uint8_t Length, WaitMode_e WaitMode, MacCmd_e MACcmd);
CoreStatus_t   OTX18_LW_GetUplinkInfo   (UplinkInfo_t* UplinkInfo, bool IsDateTime);

/* EEPROM functions */        
CoreStatus_t   OTX18_EepromRead         (uint8_t *Buffer,  uint8_t Block,  uint8_t Length);
CoreStatus_t   OTX18_EepromWrite        (uint8_t *Buffer,  uint8_t Block,  uint8_t Length);

/* Time functions */ 
CoreStatus_t   OTX18_SetTime            (void *Time, bool IsDateTime);
CoreStatus_t   OTX18_GetTime            (void *Time, bool IsDateTime);
CoreStatus_t   OTX18_GetTimeStamp       (uint32_t *ts);

/* Encryption functions */
CoreStatus_t   OTX18_AES128_ECB_Encrypt (uint32_t *Key, uint8_t *DataIn, uint8_t *DataOut, uint16_t Size);
CoreStatus_t   OTX18_AES128_ECB_Decrypt (uint32_t *Key, uint8_t *DataIn, uint8_t *DataOut, uint16_t Size);
CoreStatus_t   OTX18_AES256_ECB_Encrypt (uint32_t *Key, uint8_t *DataIn, uint8_t *DataOut, uint16_t Size);
CoreStatus_t   OTX18_AES256_ECB_Decrypt (uint32_t *Key, uint8_t *DataIn, uint8_t *DataOut, uint16_t Size);
CoreStatus_t   OTX18_SHA256_Calc        (uint8_t *DataIn, uint16_t Size, uint8_t Hash[32]);

/* Other functions */       
CoreStatus_t   OTX18_Sleep              (SleepConfig_t *SleepConfig);
CoreStatus_t   OTX18_GetStatus          (void);
ErrorStatus_t  OTX18_GetError           (void);
CoreStatus_t   OTX18_Reset              (void);
CoreStatus_t   OTX18_GetRandom          (uint32_t* RandomValue, uint8_t BitSize);
CoreStatus_t   OTX18_Protect            (uint32_t UnlockCode);
// Open stack extended implementation below

// Use the Unlock function before using any other extended functions. Unlocking may void LoRa Alliance Certification by Similarity.
void           OTX18_Unlock();

CoreStatus_t   OTX18_L_RX               (RadioParams_t *RadioParams, RadioState_t *RadioState, uint8_t *payload, uint8_t payloadSize, uint16_t timeOutMS, RadioSyncFlags_t syncFlags, uint32_t syncDelayUs, WaitMode_e waitMode);
CoreStatus_t   OTX18_L_TX               (RadioParams_t *RadioParams, RadioState_t *RadioState, uint8_t *payload, uint8_t payloadSize, uint16_t timeOutMS, RadioSyncFlags_t syncFlags, uint32_t syncDelayUs, WaitMode_e waitMode);

CoreStatus_t   OTX18_SX126xReadWrite    (uint8_t *OpcParam, uint8_t OpcParamSize, uint8_t *Buf, uint8_t Size, SX126X_RWmode_e SX126X_RWmode);

void           OTX18_Debug              (bool debugLedsOn, uint32_t * coreStatePNT); 

#endif /* USE_OLD_CORE_API */