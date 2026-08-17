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
#pragma once

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

#define UART_RX_PORT_PIN UART_RX_P10_0
#define UART_TX_PORT_PIN UART_TX_P10_1

/********************************************************************************
 * UART BAUDRATE SETTING
 ********************************************************************************
 *
 * The baudrate is calculated by:
 *     Baudrate = CLK_PERI (8MHz) / (PERI_DIV_VALUE + 1) / UART_OVERSAMPLE
 *
 * Recommended baudrate settings:
 *
 *  BAUDRATE   PERI_DIV_VALUE   UART_OVERSAMPLE   ACTUAL       ERROR (%)
 *  --------   ---------------  ----------------  ----------   ----------
 *  1,000,000  0                8                 1,000,000    0.00%
 *  500,000    0                16                500,000      0.00%
 *  250,000    1                16                250,000      0.00%
 *  125,000    6                16                125,000      0.00%
 *  115,200    4                11                116,279      +0.94%
 *   38,400    12               16                38,095       −0.79%
 *   19,200    25               16                19,512       +1.62%
 *    9,600    51               16                9,803        +2.08%
 *
 ********************************************************************************/

#define PERI_DIV_VALUE  3
#define UART_OVERSAMPLE 8

// PERIPHERAL DIVIDER NR

#define PERI_DIV_NR     7       // Select an unused 8-bit divider (0..7)

#include <stdio.h>
// Use PrintF_Start(); in your main initialization code to start the UART before using printf
void PrintF_Start(void);

void UART_WriteBlocking(const char *ptr);
