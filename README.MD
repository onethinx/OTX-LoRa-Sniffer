# Onethinx LoRa Packet Sniffer

A simple bare-LoRa packet sniffer / protocol debugging application for the Onethinx module.

The application continuously listens for LoRa packets and outputs received packet information over a high-speed 3.3 V UART connection. It is intended as a development and debugging tool for inspecting RF traffic, timing and signal quality.

The current example is configured for **868 MHz, SF7, BW125, CR 4/5**, variable-length packets and the private LoRa sync word. The firmware uses the Onethinx Core in unlocked mode to access bare LoRa functionality rather than LoRaWAN.

## Features

- Continuous bare-LoRa reception
- Up to 255-byte packets
- Packet data output as hexadecimal
- RSSI and SNR reporting
- LoRa Time-on-Air calculation
- Microsecond-resolution receive timestamps
- Time difference between received packets
- High-speed UART output for logging to a PC

For every successfully received packet, the firmware calculates timing information and outputs the timestamp, time since the previous packet, Time-on-Air, packet length, RSSI, SNR and packet contents.

Example output:

```text
[12.345.678 - 0.250.123] TOA=41.216 RX len=12 rssi=-72.5 snr=8.25 | 01 02 03 04 ...
```

## Default LoRa Configuration

| Parameter | Setting |
|---|---|
| Frequency | 868 MHz |
| Spreading Factor | SF7 |
| Bandwidth | 125 kHz |
| Coding Rate | 4/5 |
| Preamble | 8 symbols |
| Header | Variable length |
| Maximum payload | 255 bytes |
| CRC | Off |
| IQ | Normal |
| Sync word | LoRa private sync word |

These settings are defined in `RadioParams` and can be changed for the protocol or RF network being monitored.

## Serial Port

The debug output is available on a **3.3 V TTL UART**.

Serial terminal settings:

```text
Baud rate : 1,000,000
Data bits : 8
Parity    : None
Stop bits : 1
Flow ctrl : None
```

In short: **1,000,000 baud, 8-N-1**

> **Important:** This is a 3.3 V logic-level serial interface. Do not connect it directly to an RS-232 port.

## UART Connections

| Onethinx pin | Function | Connect to |
|---|---|---|
| `P10_0` | OTX RX | PC / USB-UART TX |
| `P10_1` | OTX TX | PC / USB-UART RX |
| GND | Ground | PC / USB-UART GND |

For simply monitoring the sniffer output, only these connections are required:

```text
Onethinx P10_1 (OTX TX)  --->  USB-UART RX
Onethinx GND             --->  USB-UART GND
```

Connect `P10_0` to the USB-UART TX pin if communication from the PC to the Onethinx module is required.

Use a **3.3 V USB-to-serial adapter** and open the corresponding serial device in a terminal application at 1 Mbaud.

The firmware sends each completed log line through the UART using `UART_WriteBlocking()`.

## Operation

After startup the firmware:

1. Initializes the GPIO and system clocks.
2. Starts the serial debug interface.
3. Initializes the Onethinx Core.
4. Unlocks the Core for bare-LoRa operation.
5. Starts continuous LoRa reception.
6. Prints information for each correctly received packet.

The Core unlock is required because the application uses non-LoRaWAN radio functionality. The source also notes that regulatory compliance for frequency, transmit power, duty cycle and other applicable requirements remains the user's responsibility.

## Changing the RF Settings

Edit the `RadioParams` structure in the source code:

```c
RadioParams_t RadioParams =
{
    .Frequency = 8680000,      // Frequency in 100 Hz steps
    .TXpower = PWR_MAX,
    .RXboost = true,
    .PacketType = PACKET_TYPE_LORA,
    ...
};
```

The frequency is specified in **100 Hz steps**, so:

```text
8680000 = 868.0000 MHz
9150000 = 915.0000 MHz
```

Make sure the selected RF parameters and frequency are permitted in your region.

## License

Copyright (c) 2019-2022 Onethinx BV.

This project is released under the MIT License. See the source files and/or `LICENSE` file for the complete license text.
