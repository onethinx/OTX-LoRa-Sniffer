/********************************************************************************
 *    ___             _   _     _			
 *   / _ \ _ __   ___| |_| |__ (_)_ __ __  __
 *  | | | | '_ \ / _ \ __| '_ \| | '_ \\ \/ /
 *  | |_| | | | |  __/ |_| | | | | | | |>  < 
 *   \___/|_| |_|\___|\__|_| |_|_|_| |_/_/\_\
 *
 ********************************************************************************
 *
 * Copyright (c) 2019-2025 Onethinx BV <info@onethinx.com>
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
 * Created by: Rolf Nooteboom | Onethinx on 2025-06-21
 *
 * Quick PrintF functionality for PSoC6
 *
 ********************************************************************************/

#include "project.h"
#include "PrintF.h"

/********************************************************************************
 * UART PORT SETTING
 ********************************************************************************
 * The PSoC6 SCB UART can be routed to any ports/pins:
 * SCB#     RX                   TX 
 * ----     -------------------  -------------------
 * SCB0     P0[2]                P0[3]
 * SCB1     P10[0]               P10[1]
 * SCB2     P9[0]                P9[1]
 * SCB3     P6[0]                P6[1]
 * SCB4     P7[0] P8[0]          P7[1] P8[1]
 * SCB5     P5[0] P11[0]         P5[1] P11[1]
 * SCB6     P6[4] P12[0] P13[0]  P6[1] P12[1] P13[1]
 * SCB7     P1[0]                P0[1]
 * 
 * For OTX-18 the options are:
 * P6[0]/P6[1] (SWDCLK,SWDIO), P9[0]+P9[1] (IO1,2), P10[0]+P10[1] (IO10,12)
 * 
 * For more routing options use the exceptional UDB blocks:
 * 
********************************************************************************/

// ================ RX PIN MAPPING DEFINES =============================

#define UART_RX_P0_2   0x02
#define UART_RX_P1_0   0x10
#define UART_RX_P5_0   0x50
#define UART_RX_P6_0   0x60
#define UART_RX_P6_4   0x64
#define UART_RX_P7_0   0x70
#define UART_RX_P8_0   0x80
#define UART_RX_P9_0   0x90
#define UART_RX_P10_0  0xA0
#define UART_RX_P11_0  0xB0
#define UART_RX_P12_0  0xC0
#define UART_RX_P13_0  0xD0

#if UART_RX_PORT_PIN == UART_RX_P0_2
    #define UART_SCB_NUM    0
    #define UART_RX_PORT    GPIO_PRT0
    #define UART_RX_HSIOM   P0_2_SCB0_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P10_0
    #define UART_SCB_NUM    1
    #define UART_RX_PORT    GPIO_PRT10
    #define UART_RX_HSIOM   P10_0_SCB1_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P9_0
    #define UART_SCB_NUM    2
    #define UART_RX_PORT    GPIO_PRT9
    #define UART_RX_HSIOM   P9_0_SCB2_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P6_0
    #define UART_SCB_NUM    3
    #define UART_RX_PORT    GPIO_PRT6
    #define UART_RX_HSIOM   P6_0_SCB3_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P7_0
    #define UART_SCB_NUM    4
    #define UART_RX_PORT    GPIO_PRT7
    #define UART_RX_HSIOM   P7_0_SCB4_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P8_0
    #define UART_SCB_NUM    4
    #define UART_RX_PORT    GPIO_PRT8
    #define UART_RX_HSIOM   P8_0_SCB4_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P5_0
    #define UART_SCB_NUM    5
    #define UART_RX_PORT    GPIO_PRT5
    #define UART_RX_HSIOM   P5_0_SCB5_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P11_0
    #define UART_SCB_NUM    5
    #define UART_RX_PORT    GPIO_PRT11
    #define UART_RX_HSIOM   P11_0_SCB5_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P6_4
    #define UART_SCB_NUM    6
    #define UART_RX_PORT    GPIO_PRT6
    #define UART_RX_HSIOM   P6_4_SCB6_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P12_0
    #define UART_SCB_NUM    6
    #define UART_RX_PORT    GPIO_PRT12
    #define UART_RX_HSIOM   P12_0_SCB6_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P13_0
    #define UART_SCB_NUM    6
    #define UART_RX_PORT    GPIO_PRT13
    #define UART_RX_HSIOM   P13_0_SCB6_UART_RX

#elif UART_RX_PORT_PIN == UART_RX_P1_0
    #define UART_SCB_NUM    7
    #define UART_RX_PORT    GPIO_PRT1
    #define UART_RX_HSIOM   P1_0_SCB7_UART_RX

#else
    #error "Unsupported UART_RX_PORT_PIN"
#endif

// ================ TX PIN MAPPING DEFINES =============================

#define UART_TX_P0_3   0x03
#define UART_TX_P1_1   0x11
#define UART_TX_P5_1   0x51
#define UART_TX_P6_1   0x61
#define UART_TX_P6_5   0x65
#define UART_TX_P7_1   0x71
#define UART_TX_P8_1   0x81
#define UART_TX_P9_1   0x91
#define UART_TX_P10_1  0xA1
#define UART_TX_P11_1  0xB1
#define UART_TX_P12_1  0xC1
#define UART_TX_P13_1  0xD1

#if UART_TX_PORT_PIN == UART_TX_P0_3
    #define UART_TX_SCB_NUM 0
    #define UART_TX_PORT    GPIO_PRT0
    #define UART_TX_HSIOM   P0_3_SCB0_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P10_1
    #define UART_TX_SCB_NUM 1
    #define UART_TX_PORT    GPIO_PRT10
    #define UART_TX_HSIOM   P10_1_SCB1_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P9_1
    #define UART_TX_SCB_NUM 2
    #define UART_TX_PORT    GPIO_PRT9
    #define UART_TX_HSIOM   P9_1_SCB2_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P6_1
    #define UART_TX_SCB_NUM 3
    #define UART_TX_PORT    GPIO_PRT6
    #define UART_TX_HSIOM   P6_1_SCB3_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P7_1
    #define UART_TX_SCB_NUM 4
    #define UART_TX_PORT    GPIO_PRT7
    #define UART_TX_HSIOM   P7_1_SCB4_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P8_1
    #define UART_TX_SCB_NUM 4
    #define UART_TX_PORT    GPIO_PRT8
    #define UART_TX_HSIOM   P8_1_SCB4_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P5_1
    #define UART_TX_SCB_NUM 5
    #define UART_TX_PORT    GPIO_PRT5
    #define UART_TX_HSIOM   P5_1_SCB5_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P11_1
    #define UART_TX_SCB_NUM 5
    #define UART_TX_PORT    GPIO_PRT11
    #define UART_TX_HSIOM   P11_1_SCB5_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P12_1
    #define UART_TX_SCB_NUM 6
    #define UART_TX_PORT    GPIO_PRT12
    #define UART_TX_HSIOM   P12_1_SCB6_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P13_1
    #define UART_TX_SCB_NUM 6
    #define UART_TX_PORT    GPIO_PRT13
    #define UART_TX_HSIOM   P13_1_SCB6_UART_TX

#elif UART_TX_PORT_PIN == UART_TX_P1_1
    #define UART_TX_SCB_NUM 7
    #define UART_TX_PORT    GPIO_PRT0
    #define UART_TX_HSIOM   P0_1_SCB7_UART_TX

#else
    #error "Unsupported UART_TX_PORT_PIN"
#endif

#if UART_SCB_NUM != UART_TX_SCB_NUM
    #error "RX and TX pins are mapped to different SCB instances — cannot route UART"
#endif

#define JOIN_HELPER(a)  SCB##a
#define JOIN(a)         JOIN_HELPER(a)

#define UART_HW         JOIN(UART_SCB_NUM)
#define PCLK_SCB_CLOCK  UART_SCB_NUM
#define UART_RX_PIN     (UART_RX_PORT_PIN & 0x0F)
#define UART_TX_PIN     (UART_TX_PORT_PIN & 0x0F)

/** UART Mode */
typedef enum
{
    CY_SCB_UART_STANDARD  = 0U, /**< Configures the SCB for Standard UART operation */
    CY_SCB_UART_SMARTCARD = 1U, /**< Configures the SCB for SmartCard operation */
    CY_SCB_UART_IRDA      = 2U, /**< Configures the SCB for IrDA operation */
} cy_en_scb_uart_mode_t;

/** UART Stop Bits */
typedef enum
{
    CY_SCB_UART_STOP_BITS_1   = 2U,  /**< UART looks for 1 Stop Bit    */
    CY_SCB_UART_STOP_BITS_1_5 = 3U,  /**< UART looks for 1.5 Stop Bits */
    CY_SCB_UART_STOP_BITS_2   = 4U,  /**< UART looks for 2 Stop Bits   */
    CY_SCB_UART_STOP_BITS_2_5 = 5U,  /**< UART looks for 2.5 Stop Bits */
    CY_SCB_UART_STOP_BITS_3   = 6U,  /**< UART looks for 3 Stop Bits   */
    CY_SCB_UART_STOP_BITS_3_5 = 7U,  /**< UART looks for 3.5 Stop Bits */
    CY_SCB_UART_STOP_BITS_4   = 8U,  /**< UART looks for 4 Stop Bits   */
} cy_en_scb_uart_stop_bits_t;

/** UART Parity */
typedef enum
{
    CY_SCB_UART_PARITY_NONE = 0U,    /**< UART has no parity check   */
    CY_SCB_UART_PARITY_EVEN = 2U,    /**< UART has even parity check */
    CY_SCB_UART_PARITY_ODD  = 3U,    /**< UART has odd parity check  */
} cy_en_scb_uart_parity_t;

/** UART Polarity */
typedef enum
{
    CY_SCB_UART_ACTIVE_LOW  = 0U,   /**< Signal is active low */
    CY_SCB_UART_ACTIVE_HIGH = 1U,   /**< Signal is active high */
} cy_en_scb_uart_polarity_t;

typedef struct stc_scb_uart_config
{
    cy_en_scb_uart_mode_t    uartMode;
    uint32_t    oversample;
    uint32_t    dataWidth;
    bool        enableMsbFirst;
    cy_en_scb_uart_stop_bits_t    stopBits;
    cy_en_scb_uart_parity_t    parity;
    bool        enableInputFilter;
    bool        dropOnParityError;
    bool        dropOnFrameError;
    bool        enableMutliProcessorMode;
    uint32_t    receiverAddress;
    uint32_t    receiverAddressMask;
    bool        acceptAddrInFifo;
    bool        irdaInvertRx;
    bool        irdaEnableLowPowerReceiver;
    bool        smartCardRetryOnNack;
    bool        enableCts;
    cy_en_scb_uart_polarity_t    ctsPolarity;
    uint32_t    rtsRxFifoLevel;
    cy_en_scb_uart_polarity_t    rtsPolarity;
    uint32_t    breakWidth;
    uint32_t    rxFifoTriggerLevel;
    uint32_t    rxFifoIntEnableMask;
    uint32_t    txFifoTriggerLevel;
    uint32_t    txFifoIntEnableMask;
} cy_stc_scb_uart_config_t;

const cy_stc_scb_uart_config_t UART_config = 
{
	.uartMode = CY_SCB_UART_STANDARD,
	.enableMutliProcessorMode = false,
	.smartCardRetryOnNack = false,
	.irdaInvertRx = false,
	.irdaEnableLowPowerReceiver = false,
	.oversample = UART_OVERSAMPLE,
	.enableMsbFirst = false,
	.dataWidth = 8UL,
	.parity = CY_SCB_UART_PARITY_NONE,
	.stopBits = CY_SCB_UART_STOP_BITS_1,
	.enableInputFilter = false,
	.breakWidth = 11UL,
	.dropOnFrameError = false,
	.dropOnParityError = false,
	.receiverAddress = 0x0UL,
	.receiverAddressMask = 0x0UL,
	.acceptAddrInFifo = false,
	.enableCts = false,
	.ctsPolarity = CY_SCB_UART_ACTIVE_LOW,
	.rtsRxFifoLevel = 0UL,
	.rtsPolarity = CY_SCB_UART_ACTIVE_LOW,
	.rxFifoTriggerLevel = 63UL,
	.rxFifoIntEnableMask = 0UL,
	.txFifoTriggerLevel = 63UL,
	.txFifoIntEnableMask = 0UL,
};

const cy_stc_gpio_pin_config_t UART_TX_config = 
{
	.outVal = 1,
	.driveMode = CY_GPIO_DM_STRONG_IN_OFF,
	.hsiom = UART_TX_HSIOM,
	.intEdge = CY_GPIO_INTR_DISABLE,
	.intMask = 0UL,
	.vtrip = CY_GPIO_VTRIP_CMOS,
	.slewRate = CY_GPIO_SLEW_FAST,
	.driveSel = CY_GPIO_DRIVE_1_2,
	.vregEn = 0UL,
	.ibufMode = 0UL,
	.vtripSel = 0UL,
	.vrefSel = 0UL,
	.vohSel = 0UL,
};

const cy_stc_gpio_pin_config_t UART_RX_config = 
{
	.outVal = 1,
	.driveMode = CY_GPIO_DM_HIGHZ,
	.hsiom = UART_RX_HSIOM,
	.intEdge = CY_GPIO_INTR_DISABLE,
	.intMask = 0UL,
	.vtrip = CY_GPIO_VTRIP_CMOS,
	.slewRate = CY_GPIO_SLEW_FAST,
	.driveSel = CY_GPIO_DRIVE_1_2,
	.vregEn = 0UL,
	.ibufMode = 0UL,
	.vtripSel = 0UL,
	.vrefSel = 0UL,
	.vohSel = 0UL,
};

#define CY_SCB_UART_RX_TRIGGER              (SCB_INTR_RX_TRIGGER_Msk)
#define CY_SCB_UART_RX_NOT_EMPTY            (SCB_INTR_RX_NOT_EMPTY_Msk)
#define CY_SCB_UART_RX_FULL                 (SCB_INTR_RX_FULL_Msk)
#define CY_SCB_UART_RX_OVERFLOW             (SCB_INTR_RX_OVERFLOW_Msk)
#define CY_SCB_UART_RX_UNDERFLOW            (SCB_INTR_RX_UNDERFLOW_Msk)
#define CY_SCB_UART_RX_ERR_FRAME            (SCB_INTR_RX_FRAME_ERROR_Msk)
#define CY_SCB_UART_RX_ERR_PARITY           (SCB_INTR_RX_PARITY_ERROR_Msk)
#define CY_SCB_UART_RX_BREAK_DETECT         (SCB_INTR_RX_BREAK_DETECT_Msk)
#define CY_SCB_UART_TX_TRIGGER              (SCB_INTR_TX_TRIGGER_Msk)
#define CY_SCB_UART_TX_NOT_FULL             (SCB_INTR_TX_NOT_FULL_Msk)
#define CY_SCB_UART_TX_EMPTY                (SCB_INTR_TX_EMPTY_Msk)
#define CY_SCB_UART_TX_OVERFLOW             (SCB_INTR_TX_OVERFLOW_Msk)
#define CY_SCB_UART_TX_UNDERFLOW            (SCB_INTR_TX_UNDERFLOW_Msk)
#define CY_SCB_UART_TX_DONE                 (SCB_INTR_TX_UART_DONE_Msk)
#define CY_SCB_UART_TX_NACK                 (SCB_INTR_TX_UART_NACK_Msk)
#define CY_SCB_UART_TX_ARB_LOST             (SCB_INTR_TX_UART_ARB_LOST_Msk)



#define CY_SCB_UART_RX_CTRL_SET_PARITY_Msk      (SCB_UART_RX_CTRL_PARITY_ENABLED_Msk | SCB_UART_RX_CTRL_PARITY_Msk)
#define CY_SCB_UART_RX_CTRL_SET_PARITY_Pos      SCB_UART_RX_CTRL_PARITY_Pos

#define CY_SCB_UART_TX_CTRL_SET_PARITY_Msk      (SCB_UART_TX_CTRL_PARITY_ENABLED_Msk | SCB_UART_TX_CTRL_PARITY_Msk)
#define CY_SCB_UART_TX_CTRL_SET_PARITY_Pos      SCB_UART_TX_CTRL_PARITY_Pos

#define CY_SCB_UART_TX_INTR_MASK    (CY_SCB_UART_TX_TRIGGER  | CY_SCB_UART_TX_NOT_FULL  | CY_SCB_UART_TX_EMPTY | \
                                     CY_SCB_UART_TX_OVERFLOW | CY_SCB_UART_TX_UNDERFLOW | CY_SCB_UART_TX_DONE  | \
                                     CY_SCB_UART_TX_NACK     | CY_SCB_UART_TX_ARB_LOST)

#define CY_SCB_UART_RX_INTR_MASK    (CY_SCB_UART_RX_TRIGGER    | CY_SCB_UART_RX_NOT_EMPTY | CY_SCB_UART_RX_FULL      | \
                                     CY_SCB_UART_RX_OVERFLOW   | CY_SCB_UART_RX_UNDERFLOW | CY_SCB_UART_RX_ERR_FRAME | \
                                     CY_SCB_UART_RX_ERR_PARITY | CY_SCB_UART_RX_BREAK_DETECT)

void Cy_SCB_UART_Init(CySCB_Type *base, cy_stc_scb_uart_config_t const *config)
{
    if ((NULL == base) || (NULL == config))
    {
        return;
    }

    uint32_t ovs = (config->oversample - 1UL);
    

    /* Configure the UART interface */
    SCB_CTRL(base) = _BOOL2FLD(SCB_CTRL_ADDR_ACCEPT, config->acceptAddrInFifo)  |
                 _BOOL2FLD(SCB_CTRL_BYTE_MODE, (config->dataWidth <= 8UL))      |
                 _VAL2FLD(SCB_CTRL_OVS, ovs)                                    |
                 _VAL2FLD(SCB_CTRL_MODE, 2UL);

    SCB_UART_CTRL(base) = _VAL2FLD(SCB_UART_CTRL_MODE, (uint32_t) config->uartMode);

    /* Configure the RX direction */
    SCB_UART_RX_CTRL(base) = _BOOL2FLD(SCB_UART_RX_CTRL_POLARITY, config->irdaInvertRx)              |
                         _BOOL2FLD(SCB_UART_RX_CTRL_MP_MODE, config->enableMutliProcessorMode)       |
                         _BOOL2FLD(SCB_UART_RX_CTRL_DROP_ON_PARITY_ERROR, config->dropOnParityError) |
                         _BOOL2FLD(SCB_UART_RX_CTRL_DROP_ON_FRAME_ERROR, config->dropOnFrameError)   |
                         _VAL2FLD(SCB_UART_RX_CTRL_BREAK_WIDTH, (config->breakWidth - 1UL))          |
                         _VAL2FLD(SCB_UART_RX_CTRL_STOP_BITS,   ((uint32_t) config->stopBits) - 1UL) |
                         _VAL2FLD(CY_SCB_UART_RX_CTRL_SET_PARITY, (uint32_t) config->parity);

    SCB_RX_CTRL(base) = _BOOL2FLD(SCB_RX_CTRL_MSB_FIRST, config->enableMsbFirst)          |
                    _BOOL2FLD(SCB_RX_CTRL_MEDIAN, ((config->enableInputFilter) || \
                                             (config->uartMode == CY_SCB_UART_IRDA))) |
                    _VAL2FLD(SCB_RX_CTRL_DATA_WIDTH, (config->dataWidth - 1UL));

    SCB_RX_MATCH(base) = _VAL2FLD(SCB_RX_MATCH_ADDR, config->receiverAddress) |
                     _VAL2FLD(SCB_RX_MATCH_MASK, config->receiverAddressMask);

    /* Configure the TX direction */
    SCB_UART_TX_CTRL(base) = _BOOL2FLD(SCB_UART_TX_CTRL_RETRY_ON_NACK, ((config->smartCardRetryOnNack) && \
                                                              (config->uartMode == CY_SCB_UART_SMARTCARD))) |
                         _VAL2FLD(SCB_UART_TX_CTRL_STOP_BITS, ((uint32_t) config->stopBits) - 1UL)          |
                         _VAL2FLD(CY_SCB_UART_TX_CTRL_SET_PARITY, (uint32_t) config->parity);

    SCB_TX_CTRL(base)  = _BOOL2FLD(SCB_TX_CTRL_MSB_FIRST,  config->enableMsbFirst)    |
                     _VAL2FLD(SCB_TX_CTRL_DATA_WIDTH,  (config->dataWidth - 1UL)) |
                     _BOOL2FLD(SCB_TX_CTRL_OPEN_DRAIN, (config->uartMode == CY_SCB_UART_SMARTCARD));

    SCB_RX_FIFO_CTRL(base) = _VAL2FLD(SCB_RX_FIFO_CTRL_TRIGGER_LEVEL, config->rxFifoTriggerLevel);

    /* Configure the flow control */
    SCB_UART_FLOW_CTRL(base) = _BOOL2FLD(SCB_UART_FLOW_CTRL_CTS_ENABLED, config->enableCts) |
                           _BOOL2FLD(SCB_UART_FLOW_CTRL_CTS_POLARITY, (CY_SCB_UART_ACTIVE_HIGH == config->ctsPolarity)) |
                           _BOOL2FLD(SCB_UART_FLOW_CTRL_RTS_POLARITY, (CY_SCB_UART_ACTIVE_HIGH == config->rtsPolarity)) |
                           _VAL2FLD(SCB_UART_FLOW_CTRL_TRIGGER_LEVEL, config->rtsRxFifoLevel);

    SCB_TX_FIFO_CTRL(base) = _VAL2FLD(SCB_TX_FIFO_CTRL_TRIGGER_LEVEL, config->txFifoTriggerLevel);

    /* Set up interrupt sources */
    SCB_INTR_RX_MASK(base) = (config->rxFifoIntEnableMask & CY_SCB_UART_RX_INTR_MASK);
    SCB_INTR_TX_MASK(base) = (config->txFifoIntEnableMask & CY_SCB_UART_TX_INTR_MASK);

    return;
}

__STATIC_INLINE void Cy_SCB_UART_Enable(CySCB_Type *base)
{
    SCB_CTRL(base) |= SCB_CTRL_ENABLED_Msk;
}
#define CY_SCB_FIFO_SIZE            (128UL)

__STATIC_INLINE uint32_t Cy_SCB_GetFifoSize(CySCB_Type const *base)
{
    return (_FLD2BOOL(SCB_CTRL_BYTE_MODE, SCB_CTRL(base)) ?
                (CY_SCB_FIFO_SIZE) : (CY_SCB_FIFO_SIZE / 2UL));
}

__STATIC_INLINE uint32_t Cy_SCB_GetNumInTxFifo(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_TX_FIFO_STATUS_USED, SCB_TX_FIFO_STATUS(base));
}
__STATIC_INLINE void Cy_SCB_WriteTxFifo(CySCB_Type* base, uint32_t data)
{
    SCB_TX_FIFO_WR(base) = data;
}

void UART_WriteBlocking(const char *ptr)
{
	uint32_t fifoSize = Cy_SCB_GetFifoSize(UART_HW);

	while (*ptr)
	{
		while (Cy_SCB_GetNumInTxFifo(UART_HW) >= fifoSize) {}
		Cy_SCB_WriteTxFifo(UART_HW, (uint32_t)*ptr++);
	}
}

__STATIC_INLINE uint32_t Cy_SCB_GetTxSrValid(CySCB_Type const *base)
{
    return _FLD2VAL(SCB_TX_FIFO_STATUS_SR_VALID, SCB_TX_FIFO_STATUS(base));
}

void PrintF_Start(void)
{
    if (Cy_SysClk_PeriphGetDividerEnabled(CY_SYSCLK_DIV_8_BIT, PERI_DIV_NR)) while(1) {}          // Hangs here if divider is already enabled (probably already used): select different divider.
	Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_8_BIT, PERI_DIV_NR, PERI_DIV_VALUE);
	Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_8_BIT, PERI_DIV_NR);

	Cy_SysClk_PeriphAssignDivider(PCLK_SCB_CLOCK, CY_SYSCLK_DIV_8_BIT, PERI_DIV_NR);

	Cy_GPIO_Pin_Init(UART_RX_PORT, UART_RX_PIN, &UART_RX_config);
	Cy_GPIO_Pin_Init(UART_TX_PORT, UART_TX_PIN, &UART_TX_config);

	Cy_SCB_UART_Init(UART_HW, &UART_config);
	Cy_SCB_UART_Enable(UART_HW);
}

/*******************************************************************************
* Function Name: _write
********************************************************************************
* Summary: 
* NewLib C library is used to retarget printf to _write. printf is redirected to 
* this function when GCC compiler is used to print data to terminal using UART. 
*
* \param file
* This variable is not used.
* \param *ptr
* Pointer to the data which will be transfered through UART.
* \param len
* Length of the data to be transfered through UART.
*
* \return
* returns the number of characters transferred using UART.
* \ref int
*******************************************************************************/
int _write(int file __attribute__((unused)), char *ptr, int len)
{
    int numToCopy = Cy_SCB_GetFifoSize(UART_HW) - Cy_SCB_GetNumInTxFifo(UART_HW);

    /* Adjust the data elements to write */
    if (numToCopy > len) numToCopy = len;

    /* Put data into TX FIFO */
    for (int idx = 0UL; idx < numToCopy; ++idx)
    {
        Cy_SCB_WriteTxFifo(UART_HW, (uint32_t) ptr[idx]);
        CyDelayUs(2);
    }

    return (numToCopy);
}