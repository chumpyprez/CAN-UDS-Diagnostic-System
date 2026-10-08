# CAN-Based UDS Diagnostic System

A two-node embedded automotive diagnostic communication project implementing **Unified Diagnostic Services (UDS) ReadDataByIdentifier (Service 0x22) over CAN** using two NXP LPC1768 microcontrollers.

One LPC1768 operates as the **Tester/Client** and the other as the **ECU/Server**. The Tester periodically requests the ECU Vehicle Identification Number (VIN) and can request the ECU Hardware Number through an external GPIO interrupt. The ECU processes the requested Data Identifier (DID) and returns a UDS positive response over CAN.

> **Repository note:** The original development directory and standalone source files were no longer available after a system reset. The `tester.c` and `ecu.c` files in this repository are reconstructed from the original project report. The `gg.uvprojx` file is the recovered Keil project configuration. The implementation and project behavior documented here are based on the original report.

## Project Overview

The system demonstrates:

- CAN communication between two LPC1768 boards
- UDS Service `0x22` — ReadDataByIdentifier
- Periodic diagnostic request using a Timer0 interrupt
- External-interrupt-triggered diagnostic request
- UART-based diagnostic logging
- Direct register-level LPC1768 peripheral configuration
- CAN frame construction and parsing
- Tester/ECU request-response communication

## System Architecture

```text
                  CAN BUS
       ┌──────────────────────────────┐
       │                              │
       │       Request / Response     │
       │                              │
┌──────▼──────┐                  ┌────▼───────┐
│   TESTER    │                  │    ECU     │
│   LPC1768   │                  │  LPC1768   │
│             │                  │            │
│ Timer0      │                  │ CAN RX     │
│ GPIO P2.13  │                  │ UDS 0x22   │
│ CAN1 TX     │─────────────────►│ DID decode │
│ UART0       │◄─────────────────│ CAN2 TX    │
└─────────────┘                  └────────────┘
       │                              │
       └──────────── UART ────────────┘
                      │
                      ▼
                 PC Terminal
```

## Hardware

| Component | Role |
|---|---|
| NXP LPC1768 #1 | Tester / Client |
| NXP LPC1768 #2 | ECU / Server |
| CAN interface | Communication between Tester and ECU |
| UART | Diagnostic/status output to PC |
| Push button / GPIO P2.13 | Hardware-number request trigger |

## Software / Development Environment

- Language: **C**
- MCU: **NXP LPC1768**
- CPU: **ARM Cortex-M3**
- IDE: **Keil µVision 4**
- Firmware programming: **Flash Magic**
- Communication: **CAN + UART**
- Diagnostic protocol: **UDS**

The two firmware programs were developed and compiled separately. The generated HEX files were programmed onto the respective LPC1768 boards using Flash Magic.

## UDS Functionality

The implementation uses:

### Service 0x22 — ReadDataByIdentifier

The Tester sends an 8-byte CAN frame containing:

```text
Byte 0 : 0x03   PCI / request length
Byte 1 : 0x22   ReadDataByIdentifier
Byte 2 : 0xF1   DID high byte
Byte 3 : 0x90 / 0x13
Byte 4-7 : 0x00 padding
```

### Supported Data Identifiers

| DID | Description | Trigger |
|---|---|---|
| `F190` | Vehicle Identification Number (VIN) | Timer |
| `F113` | ECU Hardware Number | GPIO interrupt |

## CAN Message Flow

### 1. VIN Request

The Tester periodically sends:

```text
CAN ID: 0x7DF
DATA:   03 22 F1 90 00 00 00 00
```

The ECU recognizes DID `F190` and sends:

```text
CAN ID: 0x7E8
DATA:   06 62 F1 90 45 43 55 00
```

The response data represents:

```text
45 43 55 → "ECU"
```

### 2. ECU Hardware Number Request

When GPIO `P2.13` is activated, the Tester sends:

```text
CAN ID: 0x7DF
DATA:   03 22 F1 13 00 00 00 00
```

The ECU responds:

```text
CAN ID: 0x7E8
DATA:   06 62 F1 13 41 42 31 32
```

The response data represents:

```text
41 42 31 32 → "AB12"
```

## Tester Operation

The Tester initializes:

- System clock
- UART0
- CAN1
- Timer0
- GPIO interrupt on P2.13

### Periodic request

Timer0 is configured to generate an event every 3 seconds. The interrupt sets:

```c
sendPeriodicFlag = 1;
```

The main loop then sends the VIN request.

### External interrupt

A falling-edge GPIO interrupt on P2.13 sends the ECU Hardware Number request.

The Tester also prints diagnostic status messages through UART.

## ECU Operation

The ECU continuously checks CAN2 for received messages.

When a message is received, it:

1. Reads the CAN identifier and data.
2. Prints the received request over UART.
3. Checks the UDS Service Identifier.
4. Checks the requested DID.
5. Builds the corresponding positive response.
6. Sends the response using CAN2.

The positive response uses SID:

```text
0x22 + 0x40 = 0x62
```

## Repository Structure

```text
CAN-UDS-Diagnostic-System/
│
├── README.md
│
├── tester/
│   └── tester.c
│
├── ecu/
│   └── ecu.c
│
├── keil/
│   └── gg.uvprojx
│
├── documentation/
│   └── project-report.docx
│
├── diagrams/
│   └── system-architecture.png
│
├── images/
│   └── README-images.txt
│
└── LICENSE
```

## Source Files

### `tester/tester.c`

Implements the Tester/Client node:

- Timer interrupt
- GPIO external interrupt
- CAN1 transmission
- UDS request construction
- UART output

### `ecu/ecu.c`

Implements the ECU/Server node:

- CAN2 reception
- UDS request parsing
- DID recognition
- UDS positive responses
- CAN2 transmission
- UART output

## Important Implementation Details

The CAN message is represented using:

```c
typedef struct {
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
} CAN_msg;
```

This allows CAN identifiers, payload length, and the 8-byte CAN data field to be handled together.

The implementation also directly accesses LPC1768 peripheral registers for CAN, UART, Timer0, GPIO interrupts, clock configuration, and NVIC interrupt enable.

## Testing

The project was implemented on two physical LPC1768 boards.

The documented test sequence was:

1. Power on Tester and ECU.
2. Establish CAN connection.
3. Connect UART outputs to the PC.
4. Flash the respective firmware.
5. Allow the Tester timer to generate the VIN request.
6. Verify the ECU receives the request.
7. Verify the ECU sends the VIN response.
8. Press the Tester GPIO button.
9. Verify the Hardware Number request.
10. Verify the ECU response.
11. Confirm the returned values through the UART terminal.

## Future Enhancements

The original project identified the following possible enhancements:

- Implement UDS Security Access Service `0x27` with seed-key authentication.
- Support multiple ECUs with functional addressing.
- Add Diagnostic Trouble Code management using services `0x19` and `0x14`.

## Limitations

This project is a compact demonstration of UDS communication over CAN rather than a complete production automotive diagnostic stack.

The implementation currently focuses on:

- Single-frame diagnostic requests
- Service `0x22`
- Two DIDs
- One Tester and one ECU
- Basic request/response handling

It does not represent a complete ISO 14229 implementation.

## Learning Outcomes

This project provided practical experience with:

- Embedded C
- ARM Cortex-M3 programming
- LPC1768 peripheral registers
- CAN communication
- UDS diagnostic messaging
- Interrupt handling
- Timers
- GPIO
- UART
- Embedded firmware development
- Automotive diagnostic communication

## Author

**BM Gagan**

Electronics and Communication Engineering  
PES University

---

### Note on the recovered project

The original source files were developed as standalone firmware programs in Keil µVision 4 and compiled into HEX files for programming the two LPC1768 boards. The original project directory was subsequently lost. The source files included in this repository were reconstructed from the preserved project report and are provided for documentation/portfolio purposes.
