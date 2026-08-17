/********************************************************************************
 *    ___             _   _     _            
 *   / _ \ _ __   ___| |_| |__ (_)_ __ __  __
 *  | | | | '_ \ / _ \ __| '_ \| | '_ \\ \/ /
 *  | |_| | | | |  __/ |_| | | | | | | |>  < 
 *   \___/|_| |_|\___|\__|_| |_|_|_| |_/_/\_\
 *
 ********************************************************************************
 *
 * Copyright (c) 2025 Onethinx BV <info@onethinx.com>
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
 * Created by:   Rolf Nooteboom
 *
 * Library to use with the Onethinx Core LoRaWAN module.
 * For a description please see:
 *      https://github.com/onethinx/OnethinxCoreAPI
 *
 ********************************************************************************/

#ifndef USE_OLD_CORE_API
#include "OTX18_Core.h"
#include "cy_ipc_pipe.h"
#include "cy_syspm.h"
#include "cy_gpio.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 *            Shared Variables
 ****************************************************************************/

/* CyPipe defines */
#define CY_IPC_CYPIPE_CHAN_MASK_EP0     (uint32_t)(0x0001ul << CY_IPC_CHAN_CYPIPE_EP0)
#define CY_IPC_CYPIPE_CHAN_MASK_EP1     (uint32_t)(0x0001ul << CY_IPC_CHAN_CYPIPE_EP1)
#define CY_IPC_CYPIPE_INTR_MASK         (uint32_t)( CY_IPC_CYPIPE_CHAN_MASK_EP0 | CY_IPC_CYPIPE_CHAN_MASK_EP1 )

/****************************************************************************
 *            CoreArguments Structure
 * This structure is used for communication with the core stack.
 * Do not modify its members in user code.
 ****************************************************************************/
// Initialize CoreArguments structure for use with M0+ IPC calls
volatile CoreArguments_t CoreArguments = {
    .ConfigurationPtr = 0      /**< Pointer to core configuration (initialized later) */
};

IpcMsgs_t IpcMsgs =
{
    {
        /* IPC structure to be sent to CM0  */
        .ClientId         = 1,                             /**< IPC_CM4_TO_CM0_CLIENT_ID = 1  */
        .UserCode         = 0,
        .IntrMask         = CY_IPC_CYPIPE_INTR_MASK,
        .CoreArgumentsPtr = &CoreArguments
    },
    {
        /* IPC structure to be received from CM0  */
        .ClientId         = 0,                             /**< IPC_CM0_TO_CM4_CLIENT_ID = 0  */
        .UserCode         = 0,
        .IntrMask         = CY_IPC_CYPIPE_INTR_MASK,
        .CoreArgumentsPtr = &CoreArguments
    }
};

/****************************************************************************
 *            Internal Functions
 ****************************************************************************/

volatile uint32_t CallBackDone;    /**< Flag to indicate IPC callback completion */

/**
 * @brief  Message callback for CM4.
 *
 * @param  Msg  Pointer to the message received (unused).
 */
void CM4_MessageCallback(uint32_t *Msg)
{
    (void)Msg;   /**< Suppress unused parameter warning */
}

/**
 * @brief  Callback to release the IPC call.
 */
void CM4_ReleaseCallback(void)
{
    CallBackDone = 1;    /**< Set callback flag to indicate release */
}

/**
 * @brief  Communication function with the core.
 *
 * @param  Function     Core function identifier.
 * @param  WaitMode     Wait mode for the call.
 * @return              CoreStatus_t  Status returned by the core.
 */
CoreStatus_t CoreComm(CoreFunctions_e Function, WaitMode_e WaitMode)
{
    SystemErrors_e             SystemError = SYSTEM_OK;
    cy_en_ipc_pipe_status_t    PipeStatus;
    
    CoreArguments.Function = Function;
    if (CoreArguments.Status.System.IsBusy)
    {
        SystemError = SYSTEM_BUSY_ERROR;
    }
    else
    {
        CoreArguments.Status.System.IsBusy = true;
        CallBackDone = 0;
        while ((CoreArguments.Status.System.IsSleeping) && ((CPUSS->CM0_STATUS & 3) == 0)) {}  /**< Wait if system is sleeping but CM0 status is not updated */
        PipeStatus = Cy_IPC_Pipe_SendMessage(  CY_IPC_EP_CYPIPE_CM0_ADDR, 
                                                CY_IPC_EP_CYPIPE_CM4_ADDR, 
                                                (void *)&IpcMsgs.forCM0, 
                                                CM4_ReleaseCallback);
        if (PipeStatus != CY_IPC_PIPE_SUCCESS)
        {
            SystemError = SYSTEM_IPC_ERROR;
        }
        else
        {
            while (!CallBackDone) {}   /**< Wait until IPC call is finalized */
            if (WaitMode != M4_NO_WAIT)
            {
                switch (WaitMode)
                {
                    case M4_WAIT_SLEEP:
                        Cy_SysPm_Sleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
                        break;
                    case M4_WAIT_DEEP_SLEEP:
                        Cy_SysPm_DeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
                        break;
                    default:
                        break;
                }
                while (CoreArguments.Status.System.IsBusy) {}  /**< Wait until core is ready */
            }
        }
    }
    
    if (SystemError != SYSTEM_OK)
    {
        CoreArguments.Status.System.ErrorStatus = SystemError;
    }
    return CoreArguments.Status;
}

/****************************************************************************
 *            Public Functions
 ****************************************************************************/

/**
 * @brief  Initialize the core.
 *
 * @param  coreConfigurationPtr  Pointer to core configuration.
 * @return CoreStatus_t          Core status after initialization.
 */
CoreStatus_t OTX18_Init(CoreConfiguration_t *coreConfigurationPtr)
{
    /* Register callback for CM4 responses */
    Cy_IPC_Pipe_RegisterCallback(CY_IPC_EP_CYPIPE_ADDR, CM4_MessageCallback, CY_IPC_EP_CYPIPE_CM0_ADDR); 
    /* Initialize shared pointers */
    IpcMsgs.forCM0.CoreArgumentsPtr   = &CoreArguments;
    IpcMsgs.fromCM0.CoreArgumentsPtr  = &CoreArguments;
    CoreArguments.ConfigurationPtr    = coreConfigurationPtr;
    /* Force current function to quit */
    CoreArguments.Status.System.BreakCurrentFunction = true;
    CoreArguments.Status.System.IsBusy               = false;
    CoreArguments.Status.System.Version              = apiVersion;
    CoreComm(CORE_FUNCTION_INIT, M4_WAIT_ACTIVE);
    if (CoreArguments.Status.System.ErrorStatus == SYSTEM_VERSION_MATCH_ERROR)
    {
        while (1) {}    /**< Hang if version mismatch detected */
    }
    return CoreArguments.Status;
}

/**
 * @brief  Reset the core.
 *
 * @return CoreStatus_t  Core status after reset.
 */
CoreStatus_t OTX18_Reset(void)
{
    /* Force current function to quit */
    CoreArguments.Status.System.BreakCurrentFunction = true;
    CoreArguments.Status.System.IsBusy               = false;
    return CoreComm(CORE_FUNCTION_RESET, M4_WAIT_ACTIVE);
}

/**
 * @brief  Join the LoRaWAN network.
 *
 * @param  WaitMode     Wait mode for the join procedure.
 * @return              CoreStatus_t  Core status after join.
 */
CoreStatus_t OTX18_LW_Join(WaitMode_e WaitMode)
{
    CoreArguments.Arg1 = 0;
    CoreArguments.Arg2 = 0;
    CoreArguments.Arg3 = 0;
    CoreArguments.Arg4 = 0;
    return CoreComm(CORE_FUNCTION_LW_JOIN, WaitMode);
}

/**
 * @brief  Retrieve core information.
 *
 * @param  CoreInfo  Pointer to a coreInfo_t structure.
 * @return CoreStatus_t  Core status after retrieval.
 */
CoreStatus_t OTX18_GetInfo(CoreInfo_t *coreInfo)
{
    CoreArguments.Arg1 = (uint32_t) coreInfo;
    return CoreComm(CORE_FUNCTION_GET_INFO, M4_WAIT_ACTIVE);
}

/**
 * @brief  Send a LoRaWAN message.
 *
 * @param  BufferPtr    Pointer to the message Buffer.
 * @param  Length       Length of the message.
 * @param  WaitMode     Wait mode for the transmission.
 * @return              CoreStatus_t  Core status after sending.
 */
CoreStatus_t OTX18_LW_Send(uint8_t *BufferPtr, uint8_t Length, WaitMode_e WaitMode)
{
    CoreArguments.Arg1 = (uint32_t) BufferPtr;
    CoreArguments.Arg2 = Length;
    return CoreComm(CORE_FUNCTION_LW_SEND, WaitMode);
}

/**
 * @brief  Send a LoRaWAN MAC command.
 *
 * @param  BufferPtr    Pointer to the message Buffer.
 * @param  Length       Length of the message.
 * @param  WaitMode     Wait mode for the transmission.
 * @param  MACcmd       MAC command identifier.
 * @return              CoreStatus_t  Core status after sending.
 */
CoreStatus_t OTX18_LW_SendMac(uint8_t *BufferPtr, uint8_t Length, WaitMode_e WaitMode, MacCmd_e MacCmd)
{
    CoreArguments.Arg1 = (uint32_t) BufferPtr;
    CoreArguments.Arg2 = Length;
    CoreArguments.Arg3 = MacCmd;
    return CoreComm(CORE_FUNCTION_LW_SEND_MAC, WaitMode);
}

/**
 * @brief  Retrieve received LoRaWAN message data.
 *
 * @param  RXdata   Pointer to the receive Buffer.
 * @param  Length   Maximum Length of the message (to prevent buffer write overflow).
 * @return          CoreStatus_t  Core status after retrieval.
 */
CoreStatus_t OTX18_GetRXdata(uint8_t *RXdata, uint8_t MaxLength)
{
    CoreArguments.Arg1 = (uint32_t) RXdata;
    CoreArguments.Arg2 = MaxLength;
    CoreComm(CORE_FUNCTION_GET_RX_DATA, M4_WAIT_ACTIVE);
    return CoreArguments.Status;
}

/**
 * @brief  Retrieves received LoRaWAN MAC command flags.
 *
 * Copies the current MAC command status into @p ReceivedMacCmds and
 * clears Status.Mac.MacCmdReceived after reading.
 * @note   Implemented since core version 0001.D312.
 *
 * @param[out] ReceivedMacCmds  Pointer to structure receiving MAC command flags.
 * @return CoreStatus_t  Status of the retrieval.
 */
CoreStatus_t OTX18_GetMacCmd(ReceivedMacCmd_t *ReceivedMacCmds)
{
    CoreComm(CORE_FUNCTION_GET_RX_MAC_CMD, M4_WAIT_ACTIVE);
    ReceivedMacCmds->Value = CoreArguments.Arg1;
    return CoreArguments.Status;
}

/**
 * @brief  Get the current core status.
 *
 * @return CoreStatus_t  The current core status.
 */
CoreStatus_t OTX18_GetStatus(void)
{
    return CoreArguments.Status;
}

/**
 * @brief  Retrieve the current error status.
 *
 * @return ErrorStatus_t  Error status (combined from parameters, radio, MAC, system).
 */
ErrorStatus_t OTX18_GetError(void)
{
    ErrorStatus_t ErrorStatus;
    ErrorStatus.ParamErrors  = CoreArguments.Status.Parameters.ErrorStatus;
    ErrorStatus.RadioErrors  = CoreArguments.Status.Radio.ErrorStatus;
    ErrorStatus.MacErrors    = CoreArguments.Status.Mac.ErrorStatus;
    ErrorStatus.SystemErrors = CoreArguments.Status.System.ErrorStatus;
    return ErrorStatus;
}

/**
 * @brief                   Put the core into sleep mode.
 *
 * @param  SleepConfig      Pointer to sleep configuration.
 * @return                  CoreStatus_t  Core status after sleep call.
 */
CoreStatus_t OTX18_Sleep(SleepConfig_t *SleepConfig)
{
    /* Debug: SWD pins may become high-Z, halting debugging */
    CoreArguments.Arg1 = (uint32_t) SleepConfig;
    CoreComm(CORE_FUNCTION_SLEEP, M4_NO_WAIT);
    if (CoreArguments.Status.System.ErrorStatus != SYSTEM_OK)
    {
        return CoreArguments.Status;
    }
    if (SleepConfig->SleepMode >= MODE_HIBERNATE)
    {
        while (1) {}   /**< System will reset after hibernate */
    }
    // Optionally wait for IPC call finalization (commented out)
    // while (!callBackDone) {}
    if (SleepConfig->SleepMode == MODE_DEEP_SLEEP)
    {
        Cy_SysPm_DeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);   /**< Wait until wake-up interrupt */
    }
    else if (SleepConfig->SleepMode == MODE_SLEEP)
    {
        Cy_SysPm_Sleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
    while (CoreArguments.Status.System.IsBusy) {}   /**< Wait until core is ready */
    return CoreArguments.Status;
}

/**
 * @brief  Set the date and time.
 *
 * @param  Time         Pointer to a TimeStamp (uint32_t) or DateTime structure.
 * @param  IsDateTime   Indicates if the time is DateTime (true) or TimeStamp (false).
 * @return              CoreStatus_t  Core status after setting.
 */
CoreStatus_t OTX18_SetTime(void *Time, bool IsDateTime)
{
    CoreArguments.Arg1 = (uint32_t) Time;
    CoreArguments.Arg2 = IsDateTime? 0 : 1;
    return CoreComm(CORE_FUNCTION_SET_DATE_TIME, M4_WAIT_ACTIVE);
}

/**
 * @brief  Get the current date and time.
 *
 * @param  Time         [out] Pointer to a TimeStamp (uint32_t) or DateTime structure to be filled.
 * @param  IsDateTime   Indicates if the time is DateTime (true) or TimeStamp (false).
 * @return              CoreStatus_t  Core status after retrieval.
 */
CoreStatus_t OTX18_GetTime(void *Time, bool IsDateTime)
{
    CoreArguments.Arg1 = (uint32_t) Time;
    CoreArguments.Arg2 = IsDateTime? 0 : 1;
    return CoreComm(CORE_FUNCTION_GET_DATE_TIME, M4_WAIT_ACTIVE);
}

/**
 * @brief  Read from EEPROM memory (currently we have 8 blocks of 256 bytes, can be expanded, contact Onethinx).
 *
 * @param  Buffer   Pointer to the Buffer for read data.
 * @param  Block    Block number to read from.
 * @param  Length   Number of bytes to read (0 = 256 bytes).
 * @return          CoreStatus_t  Core status after reading.
 */
CoreStatus_t OTX18_EepromRead(uint8_t *Buffer, uint8_t Block, uint8_t Length)
{
    CoreArguments.Arg1 = (uint32_t) Buffer;
    CoreArguments.Arg2 = Block;
    CoreArguments.Arg3 = Length;
    return CoreComm(CORE_FUNCTION_EEPROM_READ, M4_WAIT_ACTIVE);
}

/**
 * @brief  Write to EEPROM memory (currently we have 8 blocks of 256 bytes, can be expanded, contact Onethinx).
 *
 * @param  Buffer   Pointer to the data to be written.
 * @param  Block    Block number to write to.
 * @param  Length   Number of bytes to write (0 = 256 bytes).
 * @return          CoreStatus_t  Core status after writing.
 */
CoreStatus_t OTX18_EepromWrite(uint8_t *Buffer, uint8_t Block, uint8_t Length)
{
    CoreArguments.Arg1 = (uint32_t) Buffer;
    CoreArguments.Arg2 = Block;
    CoreArguments.Arg3 = Length;
    return CoreComm(CORE_FUNCTION_EEPROM_WRITE, M4_WAIT_ACTIVE);
}

/**
 * @brief  Enable or disable protection of the device.
 *
 * This function controls the protection state of the device by passing an
 * protect/unlock code to the core. 
 *
 * @param  UnlockCode
 *         Protection control code.
 *         - 0x0000 : Disable protection (unprotected, default)
 *         - Other  : Enable protection with code
 *
 * @return CoreStatus_t
 *         Core status returned after the protect/unprotect operation.
 */
CoreStatus_t OTX18_Protect(uint32_t UnlockCode)
{
    CoreArguments.Arg1 = UnlockCode;
    CoreArguments.Arg2 = 0;
    return CoreComm(CORE_FUNCTION_PROTECT, M4_WAIT_ACTIVE);
}


// CoreStatus_t OTX18_L_RX(RadioParams_t * RadioParams, RadioState_t * RadioState, uint8_t * payload, uint8_t payloadSize, uint16_t timeOutMS, WaitMode_e waitMode)
// {
// 	CoreArguments.Arg1 = (uint32_t) RadioParams;
// 	CoreArguments.Arg2 = (uint32_t) RadioState;
// 	CoreArguments.Arg3 = (uint32_t) payload;
// 	CoreArguments.Arg4 = (payloadSize << 16) | timeOutMS; 
// 	CoreStatus_t CoreStatus = CoreComm((CoreFunctions_e) CORE_FUNCTION_L_RX, waitMode);
// 	CoreStatus.Mac.BytesToRead = CoreArguments.Arg4;
// 	return CoreStatus;
// }
CoreStatus_t OTX18_L_RX(RadioParams_t *RadioParams, RadioState_t *RadioState, uint8_t *payload, uint8_t payloadSize, uint16_t timeOutMS, uint8_t syncFlags, uint32_t syncDelayUs, WaitMode_e waitMode)
{
	static RadioPacketArgs_t rxArgs;
	CoreArguments.Arg1 = (uint32_t)RadioParams;
	CoreArguments.Arg2 = (uint32_t)RadioState;
	CoreArguments.Arg3 = (uint32_t)payload;

	if (CoreArguments.Status.System.Version > 0x0001D315) { // New function selector only for newer stack
		rxArgs.payloadSize  = payloadSize;
		rxArgs.syncFlags    = syncFlags;
		rxArgs.timeOutMS    = timeOutMS;
		rxArgs.syncDelayUs  = syncDelayUs;
		CoreArguments.Arg4 = (uint32_t)&rxArgs;
	}
	else {
		CoreArguments.Arg4 = ((uint32_t)payloadSize << 16) | timeOutMS;     // Old compatability may exist till new stack version
	}
	CoreStatus_t CoreStatus = CoreComm((CoreFunctions_e)CORE_FUNCTION_L_RX, waitMode);
	CoreStatus.Mac.BytesToRead = CoreArguments.Arg4;
	return CoreStatus;
}

// CoreStatus_t OTX18_L_TX(RadioParams_t * RadioParams, RadioState_t * RadioState, uint8_t * payload, uint8_t payloadSize, uint16_t timeOutMS, WaitMode_e waitMode)
// {
// 	CoreArguments.Arg1 = (uint32_t) RadioParams;
// 	CoreArguments.Arg2 = (uint32_t) RadioState;
// 	CoreArguments.Arg3 = (uint32_t) payload;
// 	CoreArguments.Arg4 = (payloadSize << 16) | timeOutMS; 
// 	return CoreComm((CoreFunctions_e) CORE_FUNCTION_L_TX, waitMode);
// }

CoreStatus_t OTX18_L_TX(RadioParams_t *RadioParams, RadioState_t *RadioState, uint8_t *payload, uint8_t payloadSize, uint16_t timeOutMS, uint8_t syncFlags, uint32_t syncDelayUs, WaitMode_e waitMode)
{
	static RadioPacketArgs_t txArgs;

	CoreArguments.Arg1 = (uint32_t)RadioParams;
	CoreArguments.Arg2 = (uint32_t)RadioState;
	CoreArguments.Arg3 = (uint32_t)payload;

	if (CoreArguments.Status.System.Version > 0x0001D315) {
		txArgs.payloadSize = payloadSize;
		txArgs.syncFlags   = syncFlags;
		txArgs.timeOutMS   = timeOutMS;
		txArgs.syncDelayUs = syncDelayUs;
		CoreArguments.Arg4 = (uint32_t)&txArgs;
	}
	else {
		CoreArguments.Arg4 = ((uint32_t)payloadSize << 16) | timeOutMS;
	}
	return CoreComm((CoreFunctions_e)CORE_FUNCTION_L_TX, waitMode);
}

/**
 * @brief Initiates an extended LoRaWAN join procedure with customizable RX2 delay, RX and TX frequencies, and wait mode.
 *
 * This function allows configuring additional parameters for joining the LoRaWAN network, such as the RX2 delay in milliseconds, 
 * RX and TX frequencies, and the wait mode. 
 *
 * @param  RX1delayMs   Delay in milliseconds for RX1 join window (0 = default).
 * @param  RX2freq      Frequency in 100Hz steps for the RX2 window (0 = default).
 * @param  RX2dataRate  Datarate for the RX2 window (0 = default).
 * @param  TXfreq       Frequency in 100Hz steps for the uplink transmission (0 = default).
 * @param  waitMode     Wait mode for OTX LoRaWAN stack while joining.
 * @return              CoreStatus_t Status of the join process (e.g., success or failure).
 */
CoreStatus_t OTX18_Join_Ext(uint16_t RX1delayMs, uint32_t RX2freq, RadioDataRate_e RX2dataRate, uint32_t TXfreq, WaitMode_e waitMode)
{
	CoreArguments.Arg1 = RX1delayMs;
	CoreArguments.Arg2 = RX2freq;
	CoreArguments.Arg3 = 0x80000000 | RX2dataRate;
	CoreArguments.Arg4 = TXfreq;
	return CoreComm(CORE_FUNCTION_LW_JOIN, waitMode);
}

/**
 * @brief Retrieves the timestamp for the next possible uplink
 *
 * This function requests and retrieves the timestamp for the next available uplink opportunity.
 *
 * @param  UplinkInfo   [out] Pointer to the uplink-info structure where the uplink info will be stored.
 * @param  IsDateTime   Indicates if the time is DateTime (true) or TimeStamp (false).
 * @return              CoreStatus_t The status of the request, indicating success or failure.
 */
CoreStatus_t   OTX18_LW_GetUplinkInfo(UplinkInfo_t* UplinkInfo, bool IsDateTime)
{
    CoreArguments.Arg1 = (uint32_t) UplinkInfo;
    CoreArguments.Arg2 = IsDateTime? 0 : 1;
    return CoreComm(CORE_FUNCTION_LW_GET_UPLINK_INFO, M4_WAIT_ACTIVE);
}

/**
 * @brief Gets a random value from the core.
 *
 * This function gets a random value with a specified bitwidth.
 *
 * @param  RandomValue  [out] Pointer to the register where the random value will be stored (make sure to use a 32-bit aligned pointer).
 * @param  BitSize      Size of the random value in bits.
 * @return              CoreStatus_t The status of the request, indicating success or failure.
 */
CoreStatus_t   OTX18_GetRandom(uint32_t* RandomValue, uint8_t BitSize)
{
    CoreArguments.Arg1 = (uint32_t) RandomValue;
    CoreArguments.Arg2 = BitSize;
    CoreComm(CORE_FUNCTION_GET_RANDOM, M4_WAIT_ACTIVE);
    return CoreArguments.Status;
}

void OTX18_Debug(bool debugLedsOn, uint32_t * coreStatePNT)
{
	if (debugLedsOn)		// Debug LEDs should be connected to P12_4 and P12_5
	{
		Cy_GPIO_Pin_FastInit(P12_4_PORT, P12_4_NUM, CY_GPIO_DM_STRONG, 1UL, HSIOM_SEL_GPIO);
		Cy_GPIO_Pin_FastInit(P12_5_PORT, P12_5_NUM, CY_GPIO_DM_STRONG, 1UL, HSIOM_SEL_GPIO);
	}
	CoreArguments.Arg1 = debugLedsOn? 1:0;
	CoreArguments.Arg2 = (uint32_t) coreStatePNT;
	CoreArguments.Arg3 = 0;
	CoreArguments.Arg4 = 0; 
	CoreComm((CoreFunctions_e) CORE_FUNCTION_DEBUG, M4_NO_WAIT);
}

/**
 * @brief Performs AES-ECB encryption or decryption using the core.
 *
 * @param  DirMode    AES direction: CY_CRYPTO_ENCRYPT or CY_CRYPTO_DECRYPT.
 * @param  KeySize    AES key size: CY_CRYPTO_KEY_AES_128 or CY_CRYPTO_KEY_AES_256.
 * @param  Key        Pointer to the AES key. Must be 32-bit aligned.
 * @param  DataIn     Pointer to input data. Size must be a multiple of 16 bytes.
 * @param  DataOut    Pointer to output buffer. Must hold at least Size bytes.
 * @param  Size       Number of bytes to process. Must be a multiple of 16.
 * @return            CoreStatus_t The status of the request, indicating success or failure.
 */
static CoreStatus_t OTX18_AES_ECB_Crypt(crypto_dir_mode_t DirMode,
	crypto_aes_key_length_t KeySize, uint32_t *Key,
	uint8_t *DataIn, uint8_t *DataOut, uint16_t Size)
{
	CoreArguments.Arg1 = DirMode | (KeySize << 4) | ((uint32_t)Size << 16);
	CoreArguments.Arg2 = (uint32_t)Key;
	CoreArguments.Arg3 = (uint32_t)DataIn;
	CoreArguments.Arg4 = (uint32_t)DataOut;
	CoreComm(CORE_FUNCTION_AES_ECB_CRYPT, M4_WAIT_ACTIVE);
	return CoreArguments.Status;
}

/**
 * @brief Encrypts data using AES-128 ECB through the core.
 *
 * @param  Key      Pointer to the 128-bit AES key. Must be 32-bit aligned.
 * @param  DataIn   Pointer to input data. Size must be a multiple of 16 bytes.
 * @param  DataOut  Pointer to output buffer. Must hold at least Size bytes.
 * @param  Size     Number of bytes to encrypt. Must be a multiple of 16.
 * @return          CoreStatus_t The status of the request, indicating success or failure.
 */
CoreStatus_t OTX18_AES128_ECB_Encrypt(uint32_t *Key, uint8_t *DataIn,
	uint8_t *DataOut, uint16_t Size)
{
	return OTX18_AES_ECB_Crypt(CRYPTO_ENCRYPT, CRYPTO_KEY_AES_128,
		Key, DataIn, DataOut, Size);
}

/**
 * @brief Decrypts data using AES-128 ECB through the core.
 *
 * @param  Key      Pointer to the 128-bit AES key. Must be 32-bit aligned.
 * @param  DataIn   Pointer to input data. Size must be a multiple of 16 bytes.
 * @param  DataOut  Pointer to output buffer. Must hold at least Size bytes.
 * @param  Size     Number of bytes to decrypt. Must be a multiple of 16.
 * @return          CoreStatus_t The status of the request, indicating success or failure.
 */
CoreStatus_t OTX18_AES128_ECB_Decrypt(uint32_t *Key, uint8_t *DataIn,
	uint8_t *DataOut, uint16_t Size)
{
	return OTX18_AES_ECB_Crypt(CRYPTO_DECRYPT, CRYPTO_KEY_AES_128,
		Key, DataIn, DataOut, Size);
}

/**
 * @brief Encrypts data using AES-256 ECB through the core.
 *
 * @param  Key      Pointer to the 256-bit AES key. Must be 32-bit aligned.
 * @param  DataIn   Pointer to input data. Size must be a multiple of 16 bytes.
 * @param  DataOut  Pointer to output buffer. Must hold at least Size bytes.
 * @param  Size     Number of bytes to encrypt. Must be a multiple of 16.
 * @return          CoreStatus_t The status of the request, indicating success or failure.
 */
CoreStatus_t OTX18_AES256_ECB_Encrypt(uint32_t *Key, uint8_t *DataIn,
	uint8_t *DataOut, uint16_t Size)
{
	return OTX18_AES_ECB_Crypt(CRYPTO_ENCRYPT, CRYPTO_KEY_AES_256,
		Key, DataIn, DataOut, Size);
}

/**
 * @brief Decrypts data using AES-256 ECB through the core.
 *
 * @param  Key      Pointer to the 256-bit AES key. Must be 32-bit aligned.
 * @param  DataIn   Pointer to input data. Size must be a multiple of 16 bytes.
 * @param  DataOut  Pointer to output buffer. Must hold at least Size bytes.
 * @param  Size     Number of bytes to decrypt. Must be a multiple of 16.
 * @return          CoreStatus_t The status of the request, indicating success or failure.
 */
CoreStatus_t OTX18_AES256_ECB_Decrypt(uint32_t *Key, uint8_t *DataIn,
	uint8_t *DataOut, uint16_t Size)
{
	return OTX18_AES_ECB_Crypt(CRYPTO_DECRYPT, CRYPTO_KEY_AES_256,
		Key, DataIn, DataOut, Size);
}

/**
 * @brief Calculates a SHA-256 hash using the core.
 *
 * @param  DataIn  Pointer to input data.
 * @param  Size    Number of bytes to hash.
 * @param  Hash    Pointer to 32-byte output buffer. Must be 32-bit aligned.
 * @return         CoreStatus_t The status of the request, indicating success or failure.
 */
CoreStatus_t OTX18_SHA256_Calc(uint8_t *DataIn, uint16_t Size, uint8_t Hash[32])
{
	CoreArguments.Arg1 = CRYPTO_MODE_SHA256 | ((uint32_t)Size << 16);
	CoreArguments.Arg2 = 0;
	CoreArguments.Arg3 = (uint32_t)DataIn;
	CoreArguments.Arg4 = (uint32_t)Hash;
	CoreComm(CORE_FUNCTION_SHA_CALC, M4_WAIT_ACTIVE);
	return CoreArguments.Status;
}

/*!
 * Write opcode and read / write subsequent bytes
 * [IN]     *OpcParam           Pointer to opcode
 * [IN]     opcParamSize        Opcode size
 * [IN/OUT] *buf                Buffer to read and/or write to
 * [IN]     size                Size the buffer
 * [IN]     SX126X_RWmode_e             Mode: SPI_READ or SPI_WRITE or SPI_READWRITE
**/
CoreStatus_t   OTX18_SX126xReadWrite(uint8_t *OpcParam, uint8_t OpcParamSize, uint8_t *Buf, uint8_t Size, SX126X_RWmode_e SX126X_RWmode)
{
    CoreArguments.Arg1 = (uint32_t) OpcParam;
	CoreArguments.Arg2 = (uint32_t) Buf;
	CoreArguments.Arg3 = OpcParamSize << 16 | Size;
	CoreArguments.Arg4 = SX126X_RWmode; 
	return CoreComm((CoreFunctions_e) CORE_FUNCTION_SX126X, M4_WAIT_ACTIVE);
}

// Use the Unlock function before using any other extended functions. Unlocking may void LoRa Alliance Certification by Similarity.
void OTX18_Unlock()
{
	CoreArguments.Arg1 = 0x4B1D;
	CoreComm((CoreFunctions_e) CORE_FUNCTION_UNLOCK, M4_WAIT_ACTIVE);
}

#endif // USE_OLD_CORE_API
/* [] END OF FILE */
