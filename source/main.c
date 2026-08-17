
/********************************************************************************
 *    ___             _   _     _			
 *   / _ \ _ __   ___| |_| |__ (_)_ __ __  __
 *  | | | | '_ \ / _ \ __| '_ \| | '_ \\ \/ /
 *  | |_| | | | |  __/ |_| | | | | | | |>  < 
 *   \___/|_| |_|\___|\__|_| |_|_|_| |_/_/\_\
 *
 ********************************************************************************
 *
 * Copyright (c) 2019-2022 Onethinx BV <info@onethinx.com>
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
 ********************************************************************************/

#include "project.h"
#include "OTX18_Core.h"
#include "uECC.h"
#include "PrintF.h"

extern volatile CoreArguments_t CoreArguments;

/* The Payload Size we use for this example */
#define PAYLOADSIZE 255

/* Let the M0p go into deepsleep when waiting for the CM4, we don't use the BLE ECO and we like to debug this code */
CoreConfiguration_t	CoreConfig = {
	.System =
	{
		.Idle =
		{
			.Mode = 		M0_ACTIVE,
			.BleEcoON =		true,
			.DebugON =		true,
		}
	}

};

/* The LoRa Radio parameters are defined below. Change the frequency according your region */
RadioParams_t RadioParams =
{
	.Frequency = 8680000,		// Frequency in 100Hz steps
	.TXpower = PWR_MAX,
	.RXboost = true,
	.PacketType = PACKET_TYPE_LORA,
	.LoRa = 
	{
		.Modulation =
		{
			.SF = LORA_SF7,
			.BW = LORA_BW_125,
			.CR = LORA_CR_4_5,
			.LowDataRateOptimize = LORA_LOWDATARATEOPTIMIZE_OFF
		},
		.Packet =
		{
			.PreambleLength = 8,
			.HeaderType = LORA_PACKET_VARIABLE_LENGTH,
			.PayloadSize = PAYLOADSIZE,
			.CRCmode = LORA_CRC_OFF,
			.IQmode = LORA_IQ_NORMAL,
			.SyncWord = LORA_MAC_PRIVATE_SYNCWORD
		}
	}
};

uint32_t BWlookup10H[] =
{
	781,			// LORA_BW_007                     = 0,
    1563,			// LORA_BW_015                     = 1,
    3125,			// LORA_BW_031                     = 2,
    6250,			// LORA_BW_062                     = 3,
    12500,			// LORA_BW_125                     = 4,
    25000,			// LORA_BW_250                     = 5,
    50000,			// LORA_BW_500                     = 6,
	100000,			// LORA_BW_1000                    = 7,
	1042,			// LORA_BW_010                     = 8,
    2083,			// LORA_BW_020                     = 9,
	4167			// LORA_BW_041                     = 10,
};
    

SleepConfig_t SleepConfig =
{
	.SleepMode = MODE_DEEP_SLEEP,
	.BleEcoON = true,
	.DebugON = true,
	.SleepCores = CORES_BOTH,
	.WakeUpPin = WakeUpPinHigh(true),
	.WakeUpTime = WakeUpDelay(0, 0, 0, 20), // day, hour, minute, second
};

/* Declare the TX and RX buffers */
uint8_t LoRaTXbuffer[PAYLOADSIZE] = "Hello LED x";
uint8_t LoRaRXbuffer[PAYLOADSIZE];

/* Declare the RadioStatus and CoreStatus globally for debugging purposes */
RadioState_t RadioState;
CoreStatus_t CoreStatus;

volatile uint8_t RXlength = 0;

#define RX_TIMEOUT_MS   30000



static void print_hex(const uint8_t *buf, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) printf("%02X ", buf[i]);
}

static void print_ascii(const uint8_t *buf, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) {
		uint8_t c = buf[i];
		printf("%c", (c >= 32 && c < 127) ? c : '.');
	}
}

static void print_radio_params(const RadioParams_t *p)
{
	printf("Listening LoRa: freq=%lu.%lu MHz, SF%u, BW125, CR4/5, preamble=%u, CRC=%s, sync=0x%02X\r\n",
		p->Frequency / 10000,
		(p->Frequency % 10000) / 10,
		p->LoRa.Modulation.SF,
		p->LoRa.Packet.PreambleLength,
		p->LoRa.Packet.CRCmode ? "on" : "off",
		p->LoRa.Packet.SyncWord
	);
}

uint16_t proto_crc16(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t)data[i] << 24;

        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc <<= 1;
        }
    }

    crc ^= 0xFFFFFFFF;
    return (uint16_t)crc;
}

uint8_t Get_Random8()
{
	uint32_t random;
	OTX18_GetRandom(&random, 8);
	return (uint8_t) random;
}

static int psoc6_rng(uint8_t *dest, unsigned size)
{
	while (size--) *dest++ = (uint8_t) Get_Random8(); // Get 8 bits of randomness
	return 1;
}

typedef enum {
	LoRaState_RX,
	LoRaState_TX_REPLY_HELLO,
	LoRaState_WAIT_KEY,
	LoRaState_TX_REPLY_KEY,
	LoRaState_CALC_KEY,
	LoRaState_CONNECTED
} LoRaState_t;

#define RF_PROTOCOL 0
#define HELLO_SIZE  5
#define KEY_SIZE    68

#define RF_PROTOCOL_THERMOSTAT_HELLO 0
#define RF_PROTOCOL_UMR_REPLY        1

extern CoreStatus_t CoreComm(CoreFunctions_e Function, WaitMode_e WaitMode);

static LoRaState_t LoRaState = LoRaState_RX;
static uint16_t PairingId;
static uint16_t PairingId;
static uint8_t RemotePublicKey[64];
static uint8_t UmrPrivateKey[32];
static uint8_t UmrPublicKey[64];
static uint8_t SharedSecret[32];
static uint8_t AesKey[32];

#define DEMCR      (*(volatile uint32_t *)0xE000EDFC)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)

volatile uint32_t t_rx_key;
volatile uint32_t t_rx_key_reply_us;

volatile uint32_t t_key[8];
volatile uint32_t t_key_reply_us[8];

static void CycleCounter_Init(void)
{
	DEMCR |= 0x01000000;
	DWT_CYCCNT = 0;
	DWT_CTRL |= 1;
}
#define LINE_SIZE     900
volatile int dbg_umr_pub_valid, dbg_remote_pub_valid;
static char line[LINE_SIZE];
static const char hex[] = "0123456789ABCDEF";
#define IABS(x) ((x) < 0 ? -(x) : (x))
static uint32_t last_us = 0;

static const cy_stc_ble_bless_eco_cfg_params_t bleCfg =
{
	.ecoXtalStartUpTime = (785 / 31.25),
	.loadCap = ((8.500 - 7.5) / 0.075),
	.ecoFreq = CY_BLE_BLESS_ECO_FREQ_32MHZ,
	.ecoSysDiv = CY_BLE_SYS_ECO_CLK_DIV_1		//	Set the BLE ECO clock to 32 MHz
};

// Continuous Wave
int main2()
{
	/* Switch to IMO before configuring the BLE ECO */
	Cy_SysClk_ClkPathSetSource(0, CY_SYSCLK_CLKPATH_IN_IMO);
	// Reset The BLE ECO
	Cy_BLE_EcoStop();
	Cy_BLE_EcoStart(&bleCfg);
	/* Switch back to the BLE ECO */
	Cy_SysClk_ClkPathSetSource(0, CY_SYSCLK_CLKPATH_IN_ALTHF);

	__enable_irq();
	Cy_GPIO_Pin_FastInit(LED_R_PORT, LED_R_NUM, CY_GPIO_DM_STRONG, 0UL, HSIOM_SEL_GPIO);		/* Red LED OFF */
	Cy_GPIO_Pin_FastInit(LED_B_PORT, LED_B_NUM, CY_GPIO_DM_STRONG, 1UL, HSIOM_SEL_GPIO);		/* Blue LED OFF */
	CoreStatus = OTX18_Init(&CoreConfig);
	while (1)
	{
		CoreArguments.Arg1 = 0xC0DE;
		CoreArguments.Arg2 = 8680000;			// Frequency in 100Hz steps
		//CoreArguments.Arg2 = 9150000;			// Frequency in 100Hz steps
		CoreArguments.Arg3 = 5;					// Time in seconds
		CoreArguments.Arg4 = PWR_ATT_28dB;			// TX power
		
		Cy_GPIO_Write(LED_R_PORT, LED_R_NUM, 1);
		CoreComm((CoreFunctions_e) CORE_FUNCTION_UNLOCK, M4_WAIT_ACTIVE);
		Cy_GPIO_Write(LED_R_PORT, LED_R_NUM, 0);
		CyDelay(1000);
	}
}

int main(void)
{
	/* Initialize the GPIO for the blue LED */
	Cy_GPIO_Pin_FastInit(LED_B_PORT, LED_B_NUM, CY_GPIO_DM_STRONG, 0UL, HSIOM_SEL_GPIO);
	/* Initialize the GPIO for the red LED */
	Cy_GPIO_Pin_FastInit(LED_R_PORT, LED_R_NUM, CY_GPIO_DM_STRONG, 0UL, HSIOM_SEL_GPIO);
	/* Initialize the GPIO for the button */
	Cy_GPIO_Pin_FastInit(BUTTON_PORT, BUTTON_NUM, CY_GPIO_DM_HIGHZ, 1UL, HSIOM_SEL_GPIO);

		/* Switch to IMO before configuring the BLE ECO */
	Cy_SysClk_ClkPathSetSource(0, CY_SYSCLK_CLKPATH_IN_IMO);
	// Reset The BLE ECO
	Cy_BLE_EcoStop();
	Cy_BLE_EcoStart(&bleCfg);
	/* Switch back to the BLE ECO */
	Cy_SysClk_ClkPathSetSource(0, CY_SYSCLK_CLKPATH_IN_ALTHF);
	SystemCoreClockUpdate();	// Update CoreClock references necesarry for CyDelay etc

	/* enable global interrupts */
	__enable_irq();

	PrintF_Start();

	CycleCounter_Init();
	
	// Set the initial LED state
	bool LED_B_nR = true;

	/* Initialize the LoRaWAN Core */
	CoreStatus = OTX18_Init(&CoreConfig);

	/* Unlock the LoRaWAN Core for the use of non-LoRaWAN related functionality such as bare LoRa Communication */
	/* Unlocking the Core may void LoRa Alliance Certification and/or CE Certification */
	/* It is the user's responsibility to observe compliance with applicable (local) regulatories (allowed frequencies, output power, duty cycle etc.) */
	OTX18_Unlock();

	uint32_t bw10 = BWlookup10H[RadioParams.LoRa.Modulation.BW];
	uint32_t sf = RadioParams.LoRa.Modulation.SF;
	uint32_t cr = RadioParams.LoRa.Modulation.CR;
	uint32_t de = RadioParams.LoRa.Modulation.LowDataRateOptimize == LORA_LOWDATARATEOPTIMIZE_ON;
	uint32_t crc = RadioParams.LoRa.Packet.CRCmode != LORA_CRC_OFF;
	uint32_t h = RadioParams.LoRa.Packet.HeaderType == LORA_PACKET_VARIABLE_LENGTH;

	uint32_t Tsym_us = ((1UL << sf) * 100000UL) / bw10;
	uint32_t divider = (sf << 2) - (de << 3);
	uint32_t crSymbols = cr + 4;
	uint32_t fixedSymbols_x4 = ((RadioParams.LoRa.Packet.PreambleLength + 12) << 2) + 1;
	int32_t lenBase = -(sf << 2) + 8 + (crc << 4) + 20 * h;

	print_radio_params(&RadioParams);

	while (1)
	{
		RadioParams.LoRa.Packet.PayloadSize = PAYLOADSIZE;

		OTX18_L_RX(&RadioParams, &RadioState, LoRaRXbuffer, sizeof(LoRaRXbuffer), RX_TIMEOUT_MS, RADIO_SYNC_NOWAIT | RADIO_SYNC_SET_AFTER, 10, M4_WAIT_ACTIVE);

		if (!RadioState.IrqStatus.IRQ_RX_DONE || RadioState.IrqStatus.IRQ_CRC_ERROR)
			continue;

		RXlength = CoreArguments.Arg4;

		uint32_t cycles = DWT_CYCCNT;
		uint32_t us = cycles / (SystemCoreClock / 1000000UL);
		
		char *p = line;

		uint8_t rawRssi = (uint8_t)RadioState.LoRaPacketStatus.RssiPkt;
		int8_t  rawSnr  = RadioState.LoRaPacketStatus.SnrPkt;

		int16_t rssi10 = -5 * rawRssi;
		int16_t snr100 = 25 * rawSnr;

		int32_t calc = (RXlength << 3) + lenBase;
		if (calc < 0) calc = 0;

		uint32_t payloadSymb = ((calc + divider - 1) / divider) * crSymbols;
		uint32_t toa_us = ((fixedSymbols_x4 + (payloadSymb << 2)) * Tsym_us) >> 2;
		uint32_t ts_us = us - toa_us;

		uint32_t d_us = last_us ? ts_us - last_us : 0;
		last_us = ts_us;

		p += sprintf(p,
			"[%lu.%03lu.%03lu - %lu.%03lu.%03lu] TOA=%lu.%03lu RX len=%u rssi=%d.%u snr=%d.%02u | ",

			ts_us / 1000000UL,
			(ts_us / 1000UL) % 1000UL,
			ts_us % 1000UL,

			d_us / 1000000UL,
			(d_us / 1000UL) % 1000UL,
			d_us % 1000UL,

			toa_us / 1000UL,
			toa_us % 1000UL,

			RXlength,
			rssi10 / 10, IABS(rssi10 % 10),
			snr100 / 100, IABS(snr100 % 100)
		);


		for (uint8_t i = 0; i < RXlength; i++) {
			uint8_t b = LoRaRXbuffer[i];
			*p++ = hex[b >> 4];
			*p++ = hex[b & 15];
			*p++ = ' ';
		}

		// *p++ = '|';
		// *p++ = ' ';

		// for (uint8_t i = 0; i < RXlength; i++) {
		// 	uint8_t c = LoRaRXbuffer[i];
		// 	*p++ = (c >= 32 && c < 127) ? c : '.';
		// }

		*p = 0;

		UART_WriteBlocking(line);
		UART_WriteBlocking("\r\n");
	}
}

/* [] END OF FILE */
