# BLE to Modbus RTU Gateway using ESP32

An Industrial **BLE to Modbus RTU Gateway** implemented using the **ESP32** and the **ESP-IDF** framework.

The gateway continuously scans Bluetooth Low Energy (BLE) advertisement packets from nearby wireless sensors without establishing a BLE connection. The received advertisement packets are decoded to extract sensor telemetry and are exposed to industrial automation systems through a **Modbus RTU Slave** over an RS485 interface.

Unlike traditional BLE gateways that require a BLE connection, this project operates entirely in **Observer Mode**, making it capable of receiving telemetry from multiple BLE sensors simultaneously with minimal latency and power consumption.

---

# Table of Contents

- Introduction
- Project Objective
- Features
- Technologies Used
- Supported Sensors
- System Overview
- BLE Advertisement Processing
- Software Architecture
- Firmware Workflow
- Dynamic Device Discovery
- Packet Parsing
- FreeRTOS Queue
- Holding Registers
- Modbus RTU Communication
- Project Structure
- Results
- Future Improvements

---

# Introduction

## ESP32

The ESP32 is a low-power, dual-core System-on-Chip (SoC) developed by Espressif Systems for embedded and IoT applications.

Some of its major features include:

- Dual-core Xtensa LX6 Processor
- Bluetooth Low Energy (BLE)
- Wi-Fi
- UART, SPI, I2C
- GPIO
- FreeRTOS Support
- Hardware Timers
- DMA
- Low Power Modes

Its integrated BLE controller and dual-core architecture make it an ideal platform for implementing industrial BLE gateways.

---

## ESP-IDF

ESP-IDF (Espressif IoT Development Framework) is the official software development framework for ESP32 devices.

This project makes use of:

- NimBLE Host Stack
- FreeRTOS
- UART Driver
- GPIO Driver
- Bluetooth Controller
- NVS Flash
- Logging Library (ESP_LOG)

---

# Project Objective

The objective of this project is to develop an industrial gateway capable of converting BLE advertisement data into Modbus RTU Holding Registers.

The gateway performs the following operations:

- Continuously scans BLE advertisements.
- Detects supported Datoms BLE sensors.
- Extracts the Slave ID from the sensor name advertisement.
- Maintains a MAC Address ↔ Slave ID mapping.
- Parses telemetry advertisements.
- Extracts sensor parameters.
- Transfers decoded telemetry through a FreeRTOS Queue.
- Updates Modbus Holding Registers.
- Responds to Modbus RTU Function Code 03 requests over RS485.

The gateway does **not** establish a BLE connection with any sensor.

Instead, it passively receives advertisement packets and converts them into Modbus Holding Registers accessible by PLCs, HMIs and SCADA systems.

---

# Features

## BLE Features

- Passive BLE Advertisement Scanning
- Connectionless BLE Communication
- Multiple Sensor Support
- Dynamic Device Discovery
- MAC Address Based Device Identification
- Automatic Slave ID Detection
- Continuous Background Scanning

---

## Gateway Features

- BLE → Modbus RTU Conversion
- Dynamic Holding Register Allocation
- Automatic Register Updates
- FreeRTOS Queue Based Communication
- RS485 Communication
- Modbus RTU Slave
- CRC Validation
- UART Based Communication
- Modular Software Architecture

---

## Software Features

- ESP-IDF Framework
- NimBLE Host Stack
- FreeRTOS
- Multi-Core Task Execution
- Modular Design
- SOLID-Inspired Architecture
- Easy Sensor Integration
- Easily Extendable

---

# Technologies Used

## Hardware

- ESP32
- MAX485 RS485 Transceiver
- TopFly Tech BLE Sensors

---

## Software

- ESP-IDF
- NimBLE
- FreeRTOS
- UART Driver
- GPIO Driver
- NVS Flash

---

## Communication Protocols

- Bluetooth Low Energy (BLE)
- Modbus RTU
- RS485

---

# Supported BLE Sensors

Currently, the gateway supports the following BLE sensors.

| Sensor | Supported Parameters |
|---------|----------------------|
| TopFly Tech T-SENSE | Temperature, Door Status, Battery Voltage, RSSI |
| TopFly Tech T-ONE | Temperature, Humidity, Battery Voltage, RSSI |

The modular architecture allows additional BLE sensors to be integrated by implementing their advertisement parser.

---

# System Overview

```
                    BLE Sensors
                          │
                          │
            BLE Advertisement Packets
                          │
                          ▼
                +------------------+
                |      ESP32       |
                |------------------|
                | NimBLE Scanner   |
                | Packet Parser    |
                | Register Manager |
                | Modbus RTU Slave |
                +------------------+
                          │
                    UART + RS485
                          │
                          ▼
              PLC / HMI / SCADA System
```

The ESP32 continuously scans BLE advertisements, extracts sensor telemetry and stores the latest values into Modbus Holding Registers.

A Modbus Master periodically polls these registers to retrieve real-time sensor data.

---

# Why BLE Advertisement Scanning?

Instead of establishing BLE connections, the gateway operates entirely in **Observer Mode**.

This approach offers several advantages:

- Simultaneous monitoring of multiple BLE sensors.
- No BLE connection overhead.
- Lower power consumption.
- Faster telemetry acquisition.
- Reduced firmware complexity.
- Better scalability for industrial deployments.

Since all supported sensors periodically broadcast their telemetry through advertisement packets, a BLE connection is unnecessary for data acquisition.

<!-- Part 2  -->

# Bluetooth Low Energy (BLE)

Bluetooth Low Energy (BLE) is a wireless communication protocol designed for low-power embedded devices.

Unlike Classic Bluetooth, BLE allows devices to periodically broadcast small packets of data called **Advertising Packets**, enabling nearby devices to receive information without establishing a connection.

This project operates the ESP32 as a **BLE Observer**, continuously listening to advertisement packets transmitted by nearby BLE sensors.

---

# BLE Roles

BLE devices can operate in different roles.

| Role | Description |
|------|-------------|
| Broadcaster | Transmits advertisement packets without accepting connections. |
| Observer | Listens to advertisement packets without establishing connections. |
| Peripheral | Accepts BLE connections and exposes GATT services. |
| Central | Initiates BLE connections with peripherals. |

In this project,

- The **TopFly BLE Sensors** operate as **Broadcasters**.
- The **ESP32** operates as an **Observer**.

No BLE connection is ever established.

---

# BLE Advertisement Packet

A BLE advertisement consists of multiple **Advertising Data (AD) Structures**.

Each structure contains:

```
+---------+------+--------------------+
| Length  | Type |       Data         |
+---------+------+--------------------+
```

where

- **Length** specifies the size of the AD structure.
- **Type** identifies the type of data.
- **Data** contains the actual payload.

Multiple AD structures are concatenated together to form a complete advertisement packet.

---

# Common Advertisement Data Types

| Type | Description |
|------|-------------|
| 0x01 | Flags |
| 0x03 | Complete List of 16-bit Service UUIDs |
| 0x08 | Shortened Local Name |
| 0x09 | Complete Local Name |
| 0x16 | Service Data |
| 0xFF | Manufacturer Specific Data |

The gateway scans every advertisement and parses the required AD structures.

---

# Advertisement Types Used in this Project

The Datoms BLE sensors periodically transmit two different advertisement packets.

## 1. Name Advertisement

This advertisement contains the sensor name.

Example

```
Datoms_1

Datoms_2

Datoms_54
```

This packet is used only to determine the logical Slave ID assigned to the sensor.

Example advertisement:

```
02 01 06
03 03 AA FE
09 09 44 61 74 6F 6D 73 5F 32
```

which decodes as

| Field | Meaning |
|------|---------|
| 02 01 06 | BLE Flags |
| 03 03 AA FE | 16-bit Service UUID |
| 09 09 | Complete Local Name |
| 44 61 74 6F 6D 73 5F 32 | "Datoms_2" |

The gateway extracts:

- Sensor Name
- Slave ID

and stores

```
MAC Address

↓

Slave ID
```

for future telemetry packets.

---

## 2. Telemetry Advertisement

The telemetry advertisement contains the actual sensor measurements.

Example

```
18 16 AA FE
08
12
20
08
00
D3 4C 99 CD 66 31
00 0C 15
64
09 76
FF
00
01
00
```

Unlike the Name Advertisement, the telemetry packet **does not contain the Slave ID**.

Instead, the ESP32 identifies the sensor using its BLE MAC Address and retrieves the corresponding Slave ID from the previously stored mapping.

---

# Dynamic Device Discovery

Each sensor periodically broadcasts both advertisements.

```
Name Advertisement

↓

Datoms_2

↓

Extract Slave ID

↓

Store

MAC ↔ Slave ID



Telemetry Advertisement

↓

Same MAC

↓

Lookup Slave ID

↓

Decode Telemetry
```

This mechanism allows sensors to be added or replaced without modifying the firmware.

---

# Supported Telemetry

## TopFly T-SENSE

The gateway extracts:

- Temperature
- Door Status
- Battery Voltage
- RSSI

---

## TopFly T-ONE

The gateway extracts:

- Temperature
- Humidity
- Battery Voltage
- RSSI

Both packet formats are converted into a common internal representation before being sent to the Modbus subsystem.

---

# Advertisement Processing

Every received advertisement is processed by the GAP callback.

```
BLE Advertisement

↓

BLE_GAP_EVENT_DISC

↓

Parse Advertisement Fields

↓

Datoms Advertisement?

      │
  Yes │ No
      │
      ▼

Ignore Packet



Name Advertisement ?

      │
 ┌────┴────┐
 │         │
YES       NO
 │         │
 ▼         ▼

Extract     Parse
Slave ID    Telemetry

 │          │

 ▼          ▼

Update      Create
Device Map  sensor_packet_t

      │

      ▼

FreeRTOS Queue
```

---

# Why Two Advertisements?

Separating the Name Advertisement from the Telemetry Advertisement provides several advantages.

- The Slave ID is transmitted only when required.
- Telemetry packets remain compact.
- BLE advertisement size remains within protocol limits.
- Sensor identity and telemetry remain independent.
- Additional telemetry fields can be added without changing the naming scheme.

The gateway combines both advertisements using the common BLE MAC Address, ensuring that each telemetry packet is associated with the correct Modbus Slave ID.

---

# Packet Validation

Before processing any advertisement, the gateway performs several checks.

- Advertisement Length
- Advertisement Type
- Datoms Name Prefix
- Supported Service UUID
- Supported Packet Format
- Known Device Lookup
- Packet Length Validation

Only valid advertisements are forwarded for telemetry decoding.

<!-- Part 3 -->

# Software Architecture

The firmware has been designed using a modular architecture inspired by the **SOLID design principles**.

Each software module is responsible for a single functionality and communicates with other modules through well-defined interfaces.

This improves readability, maintainability and allows new BLE sensors or Modbus functionality to be integrated with minimal modifications.

---

# Project Structure

```
main
│
├── include
│   ├── common.h
│   ├── scan.h
│   ├── gap.h
│   ├── parser.h
│   ├── registers.h
│   └── modbus.h
│
├── src
│   ├── main.c
│   ├── common.c
│   ├── scan.c
│   ├── gap.c
│   ├── parser.c
│   ├── registers.c
│   └── modbus.c
│
├── CMakeLists.txt
│
└── README.md
```

---

# Module Responsibilities

## main.c

Responsible for:

- Application Entry Point
- NVS Initialization
- Bluetooth Controller Initialization
- NimBLE Host Initialization
- UART Initialization
- Queue Creation
- FreeRTOS Task Creation

---

## scan.c

Responsible for:

- BLE Scan Configuration
- Scan Parameters
- Starting BLE Scan
- Restarting Scan

---

## gap.c

Responsible for:

- GAP Event Callback
- Receiving Advertisement Packets
- Detecting Datoms Devices
- Passing Advertisements to the Parser

---

## parser.c

Responsible for:

- Parsing Name Advertisements
- Parsing Telemetry Advertisements
- Extracting Slave ID
- Extracting Temperature
- Extracting Humidity
- Extracting Door Status
- Extracting Battery Voltage
- Creating sensor_packet_t
- Sending Data to Queue

---

## registers.c

Responsible for:

- Dynamic Register Allocation
- Holding Register Updates
- Register Lookup
- Register Printing
- Device Table Management

---

## modbus.c

Responsible for:

- UART Reception
- CRC Verification
- Modbus Request Processing
- Building RTU Response
- UART Transmission

---

## common.c

Responsible for:

- Utility Functions
- Hex Dump
- MAC Printing
- Common Helper Functions

---

# Overall Software Architecture

```
                    +----------------+
                    |    main.c      |
                    +-------+--------+
                            |
        +-------------------+-------------------+
        |                   |                   |
        ▼                   ▼                   ▼
    scan.c              modbus.c           common.c
        │
        ▼
    gap.c
        │
        ▼
   parser.c
        │
        ▼
 FreeRTOS Queue
        │
        ▼
 registers.c
        │
        ▼
 Holding Registers
        │
        ▼
   Modbus Response
```

---

# Multi-Core Architecture

The ESP32 contains two Xtensa processor cores.

To improve responsiveness, BLE scanning and Modbus communication execute independently.

```
                    ESP32

        +----------------------------+
        |          Core 0            |
        |----------------------------|
        |                            |
        | NimBLE Host Task           |
        | BLE Advertisement Scan     |
        | GAP Callback               |
        | Packet Parser              |
        | Queue Send                 |
        +----------------------------+


        +----------------------------+
        |          Core 1            |
        |----------------------------|
        |                            |
        | Modbus Task                |
        | Queue Receive              |
        | Holding Registers          |
        | UART Receive               |
        | Modbus Response            |
        +----------------------------+
```

Running BLE and Modbus on separate cores prevents UART communication from blocking BLE advertisement reception.

---

# Firmware Workflow

After power-up, the firmware executes the following sequence.

```
Power ON

↓

app_main()

↓

Initialize NVS

↓

Initialize Bluetooth Controller

↓

Initialize NimBLE

↓

Initialize UART

↓

Create Queue

↓

Create Modbus Task

↓

Start BLE Scan
```

After initialization, the firmware continuously waits for BLE advertisements and Modbus requests.

---

# Complete Data Flow

The following diagram illustrates the complete gateway workflow.

```
BLE Sensor

↓

Advertisement Packet

↓

NimBLE Scanner

↓

GAP Callback

↓

Advertisement Parser

↓

Extract Telemetry

↓

sensor_packet_t

↓

FreeRTOS Queue

↓

Holding Register Manager

↓

Holding Registers

↓

UART Request

↓

Modbus RTU Slave

↓

UART Response

↓

PLC / HMI / SCADA
```

---

# FreeRTOS Queue

The gateway follows a **Producer–Consumer** architecture.

```
BLE Parser

↓

Producer

↓

FreeRTOS Queue

↓

Consumer

↓

Modbus Task
```

The BLE subsystem continuously produces decoded sensor packets.

The Modbus task consumes these packets and updates the Holding Registers.

Using a queue decouples BLE processing from UART communication, preventing one subsystem from blocking the other.

---

# Why Use a Queue?

Updating Holding Registers directly inside the GAP callback would tightly couple BLE reception with Modbus communication.

Using a queue provides:

- Thread-safe communication
- Independent BLE and Modbus execution
- Better scalability
- Improved maintainability
- Reduced packet loss

This architecture allows BLE advertisements to continue being received even while the Modbus task is transmitting UART responses.

---

# Common Sensor Packet

After decoding a BLE advertisement, both supported sensor types are converted into a common internal representation.

```c
typedef struct
{
    uint8_t slave_id;

    uint16_t temperature;

    uint16_t humidity_or_door;

    uint16_t battery_mv;

    int8_t rssi;

} sensor_packet_t;
```

The queue transfers this structure from the BLE task to the Modbus task.

Using a common packet format allows new BLE sensors to be integrated without modifying the Holding Register or Modbus implementations.

---

# Inter-Module Communication

The modules communicate using well-defined interfaces.

```
main.c

↓

scan.c

↓

gap.c

↓

parser.c

↓

sensor_packet_t

↓

FreeRTOS Queue

↓

registers.c

↓

holding_registers[]

↓

modbus.c

↓

UART Driver
```

Each module performs one specific task, reducing coupling between software components and making the firmware easier to maintain and extend.

<!-- Part 4 -->

# Dynamic Device Discovery

Unlike traditional Modbus gateways where Slave IDs are statically configured, this gateway dynamically discovers BLE sensors during runtime.

Each supported sensor periodically broadcasts two different advertisement packets.

1. Name Advertisement
2. Telemetry Advertisement

The gateway combines both advertisements using the BLE MAC Address.

---

# Name Advertisement

The Name Advertisement contains the logical Slave ID assigned to the sensor.

Example

```
Datoms_1

Datoms_2

Datoms_54
```

Example advertisement

```
02 01 06
03 03 AA FE
09 09 44 61 74 6F 6D 73 5F 32
```

which represents

```
Datoms_2
```

The parser extracts

- Device Name
- Slave ID

and stores

```
MAC Address

↓

Slave ID
```

inside the device table.

---

# Device Table

The gateway maintains an internal table containing every discovered sensor.

```c
typedef struct
{
    uint8_t mac[6];

    uint8_t slave_id;

} datoms_device_t;
```

Example

| MAC Address | Slave ID |
|-------------|----------|
| D3:4C:99:CD:66:31 | 2 |
| E1:06:9E:32:55:39 | 1 |
| 84:72:A4:33:11:8A | 54 |

The MAC Address uniquely identifies every BLE sensor.

Whenever another advertisement is received from the same MAC, the gateway immediately knows which Slave ID it belongs to.

---

# Why Use the MAC Address?

Telemetry advertisements do **not** contain the sensor name.

Instead,

```
Name Advertisement

↓

Datoms_2

↓

Store

MAC

↓

Slave ID
```

Later,

```
Telemetry Advertisement

↓

Extract MAC

↓

Lookup Device Table

↓

Retrieve Slave ID
```

This avoids transmitting the sensor name inside every telemetry advertisement and keeps BLE packets compact.

---

# Telemetry Advertisement

Telemetry advertisements contain the actual sensor measurements.

For example,

### T-SENSE

- Temperature
- Door Status
- Battery Voltage
- RSSI

### T-ONE

- Temperature
- Humidity
- Battery Voltage
- RSSI

The parser identifies the packet format, extracts the required fields and creates a common sensor packet.

```c
typedef struct
{
    uint8_t slave_id;

    uint16_t temperature;

    uint16_t humidity_or_door;

    uint16_t battery_mv;

    int8_t rssi;

} sensor_packet_t;
```

The Slave ID is retrieved from the device table using the BLE MAC Address.

---

# Queue Processing

After decoding the advertisement, the parser places the sensor packet into a FreeRTOS Queue.

```
Advertisement

↓

Parser

↓

sensor_packet_t

↓

Queue
```

The queue separates BLE reception from Modbus processing.

---

# Queue Reception

The Modbus task continuously waits for new telemetry.

```
Queue

↓

xQueueReceive()

↓

sensor_packet_t

↓

Update Holding Registers
```

The queue ensures that BLE packet reception is never blocked while the Modbus task is servicing UART requests.

---

# Holding Registers

The gateway stores the latest telemetry from every discovered sensor inside Modbus Holding Registers.

Each sensor occupies **five consecutive registers**.

| Register Offset | Description |
|-----------------|-------------|
| 0 | Slave ID |
| 1 | Temperature |
| 2 | Humidity / Door Status |
| 3 | Battery Voltage |
| 4 | RSSI |

---

# Holding Register Layout

```
Registers

+-----+---------+
| R0  | SlaveID |
+-----+---------+
| R1  | Temp    |
+-----+---------+
| R2  | Hum/Door|
+-----+---------+
| R3  | Battery |
+-----+---------+
| R4  | RSSI    |
+-----+---------+

+-----+---------+
| R5  | SlaveID |
+-----+---------+
| R6  | Temp    |
+-----+---------+
| R7  | Hum/Door|
+-----+---------+
| R8  | Battery |
+-----+---------+
| R9  | RSSI    |
+-----+---------+
```

Every newly discovered sensor occupies one complete block of five registers.

---

# Dynamic Register Allocation

Holding Registers are allocated dynamically.

Whenever telemetry is received,

```
Receive sensor_packet_t

↓

Find Slave ID

↓

Already Exists ?

      │
 ┌────┴─────┐
 │          │
YES        NO
 │          │
 ▼          ▼

Update    Allocate
Existing  Next Free Block

↓

Update Telemetry

↓

Return
```

This mechanism ensures that

- Existing sensors update their current register values.
- Newly discovered sensors automatically receive the next available register block.
- Duplicate register entries are avoided.

---

# Register Update Algorithm

The register update process follows the algorithm shown below.

```
Receive Sensor Packet

↓

Search Holding Registers

↓

Slave Found ?

     │
 ┌───┴────┐
 │        │
YES      NO
 │        │
 ▼        ▼

Update   Find Empty Block

            │

            ▼

     Store Slave ID

            │

            ▼

Update Temperature

↓

Update Humidity / Door Status

↓

Update Battery Voltage

↓

Update RSSI
```

---

# Example

Suppose two sensors are discovered.

```
Datoms_1

↓

Temperature = 26.30°C

Humidity = 51%

Battery = 3000 mV

RSSI = -71
```

```
Datoms_2

↓

Temperature = 24.22°C

Door = OPEN

Battery = 3093 mV

RSSI = -46
```

The Holding Registers become

```
================================================

Slave 1

Temp      : 0x0A46

Hum/Door  : 0x0033

Battery   : 0x0BB8

RSSI      : 0xFFB9

------------------------------------------------

Slave 2

Temp      : 0x0976

Hum/Door  : 0x0001

Battery   : 0x0C15

RSSI      : 0xFFD2

================================================
```

These registers are continuously updated whenever new BLE telemetry is received and are subsequently served to Modbus RTU Masters upon request.

---

# Advantages of the Design

The implemented architecture provides several advantages.

- Dynamic BLE sensor discovery
- No hardcoded Slave IDs
- Automatic MAC ↔ Slave ID association
- Dynamic Holding Register allocation
- Efficient memory utilization
- Queue-based task synchronization
- Easy integration of additional BLE sensors
- Minimal coupling between BLE and Modbus modules
- Scalable architecture suitable for industrial deployments

<!-- Part 5 -->

# Modbus RTU Communication

The ESP32 operates as a **Modbus RTU Slave**, allowing external Modbus Masters such as PLCs, HMIs and SCADA systems to access the latest BLE sensor telemetry.

Communication between the ESP32 and the Modbus Master is performed over an RS485 interface using the UART peripheral.

The gateway currently supports:

- Function Code 03 (Read Holding Registers)

Additional Modbus Function Codes can be integrated without modifying the BLE subsystem.

---

# Why Modbus RTU?

Modbus RTU is one of the most widely used industrial communication protocols.

Some advantages include:

- Simple frame format
- Low communication overhead
- Reliable CRC protection
- Wide industrial adoption
- Easy integration with PLCs and HMIs

Since most industrial automation equipment supports Modbus RTU, it provides an ideal interface for exposing BLE sensor telemetry.

---

# RS485 Interface

The ESP32 communicates with the Modbus Master through an RS485 transceiver.

```
               ESP32

        UART TX ---------> DI

        UART RX <--------- RO

        GPIO ------------> DE / RE


                MAX485

        A ---------------------- RS485 Bus

        B ---------------------- RS485 Bus
```

During transmission,

- DE is enabled.
- UART transmits the response frame.
- Transmission completion is awaited.
- DE is disabled.
- UART returns to receive mode.

---

# Modbus RTU Request

The Modbus Master periodically polls the gateway.

The request frame has the following format.

```
+-----------+-----------+---------------+-----------+------+
| Slave ID  | Function  | Start Address | Quantity  | CRC  |
+-----------+-----------+---------------+-----------+------+
```

Example

```
02 03 00 00 00 05 CRC
```

which represents

```
Slave ID      = 2

Function Code = 03

Start Address = 0

Quantity       = 5 Registers
```

---

# Request Processing

Whenever a UART frame is received, the following sequence is executed.

```
UART RX

↓

Receive Request

↓

Validate Length

↓

CRC Verification

↓

CRC Valid ?

      │
 ┌────┴────┐
 │         │
NO        YES
 │         │
 ▼         ▼

Ignore   Continue
Request  Processing
```

Requests with invalid CRC values are discarded immediately.

This prevents corrupted UART frames from affecting the Holding Registers.

---

# CRC Verification

The gateway verifies every received Modbus request before processing it.

```
Received Frame

↓

Extract Received CRC

↓

Calculate CRC

↓

Compare

↓

Match ?

     │
 ┌───┴────┐
 │        │
YES      NO
 │        │
 ▼        ▼

Process   Ignore Frame
Request
```

Only requests with a valid CRC are accepted.

---

# Function Code Processing

After CRC validation, the gateway extracts

- Slave ID
- Function Code
- Starting Address
- Number of Registers

```
Request

↓

Read Slave ID

↓

Read Function Code

↓

Function = 03 ?

      │
 ┌────┴────┐
 │         │
NO        YES
 │         │
 ▼         ▼

Ignore   Read Registers
```

Currently, Function Code 03 is supported.

---

# Register Lookup

After identifying the requested Slave ID,

the gateway searches the Holding Register table.

```
Slave ID

↓

Find Register Block

↓

Block Found ?

      │
 ┌────┴────┐
 │         │
NO        YES
 │         │
 ▼         ▼

Ignore   Read Registers
```

The corresponding register values are copied into the response buffer.

---

# Response Construction

The response frame follows the Modbus RTU specification.

```
+-----------+-----------+------------+----------------------+------+
| Slave ID  | Function  | Byte Count | Register Data        | CRC  |
+-----------+-----------+------------+----------------------+------+
```

Example

```
02
03
08

09 76
00 01
0C 15
FF D2

CRC
```

where

```
09 76

↓

Temperature

--------------------

00 01

↓

Door Status

--------------------

0C 15

↓

Battery Voltage

--------------------

FF D2

↓

RSSI
```

---

# Response Flow

```
Holding Registers

↓

Copy Requested Registers

↓

Create Response Frame

↓

Append CRC

↓

UART TX
```

The Modbus Master receives the latest BLE telemetry exactly as standard Holding Registers.

---

# Modbus Processing Flow

The complete request-response sequence is shown below.

```
                UART Receive

                      │

                      ▼

             Validate Frame Length

                      │

                      ▼

               CRC Verification

                      │

            ┌─────────┴─────────┐

            │                   │

        CRC Invalid         CRC Valid

            │                   │

            ▼                   ▼

      Ignore Request      Parse Request

                                │

                                ▼

                    Function Code = 03 ?

                                │

                      ┌─────────┴─────────┐

                      │                   │

                     No                  Yes

                      │                   │

                      ▼                   ▼

                Ignore Request     Find Slave

                                        │

                                        ▼

                               Read Holding Registers

                                        │

                                        ▼

                               Build Response Frame

                                        │

                                        ▼

                                  Calculate CRC

                                        │

                                        ▼

                                 UART Transmission
```

---

# Example Communication

The following example illustrates a complete transaction.

## Request

```
02 03 00 00 00 05 CRC
```

Meaning

```
Slave ID = 2

Read 5 Holding Registers

Starting Address = 0
```

---

## Response

```
02
03
08

09 76

00 01

0C 15

FF D2

CRC
```

Decoded as

| Register | Value | Description |
|-----------|-------|-------------|
| R1 | 0x0976 | Temperature |
| R2 | 0x0001 | Door Status |
| R3 | 0x0C15 | Battery Voltage |
| R4 | 0xFFD2 | RSSI |

---

# Error Handling

The Modbus subsystem handles the following conditions.

- Invalid UART Frame
- Invalid Frame Length
- CRC Failure
- Unsupported Function Code
- Unknown Slave ID
- Invalid Register Address
- Register Range Overflow

Whenever an invalid request is detected, the gateway safely ignores the request without affecting BLE scanning or Holding Register updates.

---

# Advantages of the Implementation

The implemented Modbus RTU subsystem provides

- Standard Modbus RTU communication
- Reliable CRC validation
- Efficient UART communication
- Dynamic register lookup
- Automatic response generation
- Separation from BLE processing
- Thread-safe operation through queue-based synchronization
- Easy integration with industrial PLCs, HMIs and SCADA systems

<!-- Part 6 -->



# Source Code Organization

The firmware has been organized into independent software modules following a modular design approach.

Each module is responsible for a single functionality and communicates with other modules through clearly defined interfaces.

```
ble_modbus_gateway

│

├── include

│   ├── common.h

│   ├── scan.h

│   ├── gap.h

│   ├── parser.h

│   ├── registers.h

│   └── modbus.h

│

├── src

│   ├── main.c

│   ├── common.c

│   ├── scan.c

│   ├── gap.c

│   ├── parser.c

│   ├── registers.c

│   └── modbus.c

│

├── README.md

└── CMakeLists.txt
```

---

# Module Description

| Module | Responsibility |
|---------|----------------|
| **main.c** | System Initialization, Queue Creation, Task Creation |
| **scan.c** | BLE Scan Configuration |
| **gap.c** | GAP Event Callback |
| **parser.c** | Advertisement Parsing and Telemetry Extraction |
| **registers.c** | Device Table and Holding Register Management |
| **modbus.c** | UART Communication and Modbus RTU Slave |
| **common.c** | Common Utility Functions |

---

# Design Decisions

## Why BLE Advertisements Instead of GATT?

The supported TopFly sensors continuously broadcast telemetry through BLE advertisements.

Using advertisement scanning offers several advantages.

- No BLE connection establishment.
- Faster telemetry acquisition.
- Multiple sensors can be monitored simultaneously.
- Lower processor utilization.
- Lower power consumption.

Therefore, the gateway operates entirely as a BLE Observer.

---

## Why Use MAC Address for Identification?

Each sensor transmits two advertisements.

The Name Advertisement contains

```
Datoms_<SlaveID>
```

whereas the Telemetry Advertisement contains only sensor measurements.

Since both advertisements originate from the same BLE MAC Address, the gateway stores

```
MAC Address

↓

Slave ID
```

This mapping allows telemetry packets to be associated with the correct Modbus Slave ID.

---

## Why Dynamic Register Allocation?

Instead of assigning fixed Holding Registers to predefined Slave IDs, the gateway dynamically allocates register blocks.

Benefits include

- No firmware modifications when adding sensors.
- Efficient use of Holding Registers.
- Automatic sensor discovery.
- Runtime scalability.

---

## Why Use a Queue?

BLE advertisements may arrive while the Modbus task is transmitting UART responses.

To prevent BLE reception from being blocked, decoded telemetry is first placed into a FreeRTOS Queue.

```
BLE

↓

Queue

↓

Modbus
```

This implements a Producer–Consumer architecture and ensures independent execution of BLE and Modbus subsystems.

---

## Why Separate Tasks Across CPU Cores?

The ESP32 contains two processor cores.

The firmware assigns

Core 0

- BLE Scanner
- GAP Events
- Advertisement Parsing

Core 1

- Queue Reception
- Holding Register Updates
- Modbus RTU Communication

This separation improves responsiveness and prevents UART communication from delaying BLE packet reception.

---

# Testing

The gateway was tested using

- TopFly Tech T-SENSE
- TopFly Tech T-ONE
- Modbus Poll
- USB to RS485 Converter

Successful verification included

- BLE Advertisement Reception
- Name Advertisement Parsing
- Telemetry Parsing
- MAC ↔ Slave ID Mapping
- Queue Transfer
- Holding Register Updates
- Function Code 03 Requests
- CRC Validation
- UART Communication

---

# Sample Holding Registers

```
==================================================

Slave 1

Temp      : 0x0924

Hum/Door  : 0x0033

Battery   : 0x0BB8

RSSI      : 0xFFD5

--------------------------------------------------

Slave 2

Temp      : 0x0976

Hum/Door  : 0x0001

Battery   : 0x0C15

RSSI      : 0xFFD2

==================================================
```

---

# Sample Modbus Communication

## Request

```
02 03 00 00 00 05 CRC
```

---

## Response

```
02
03
08

09 76

00 01

0C 15

FF D2

CRC
```

The response was successfully decoded by Modbus Poll.

---

# Current Features

✔ BLE Advertisement Scanner

✔ Dynamic Device Discovery

✔ Dynamic Slave ID Assignment

✔ MAC Address Mapping

✔ TopFly T-SENSE Support

✔ TopFly T-ONE Support

✔ Queue Based Processing

✔ Dynamic Holding Registers

✔ Modbus RTU Slave

✔ CRC Validation

✔ UART over RS485

✔ Function Code 03

---

# Future Improvements

The current implementation can be extended to support additional industrial features.

Planned improvements include

- Function Code 04 (Read Input Registers)
- Function Code 06 (Write Single Register)
- Function Code 16 (Write Multiple Registers)
- BLE Device Whitelisting
- Sensor Timeout Detection
- Non-Volatile Device Storage
- Configurable Register Mapping
- OTA Firmware Updates
- MQTT Gateway Support
- Wi-Fi Connectivity
- Web Configuration Interface
- BLE Configuration Mode
- Additional BLE Sensor Support

---

# Lessons Learned

The development of this gateway provided practical experience with

- ESP-IDF Development
- BLE GAP Advertisement Scanning
- NimBLE Host Stack
- BLE Advertisement Parsing
- Connectionless BLE Communication
- Dynamic Device Discovery
- FreeRTOS
- Producer–Consumer Architecture
- Queue Based Inter-Task Communication
- Multi-Core Programming
- UART Driver
- RS485 Communication
- Modbus RTU Protocol
- CRC Generation and Verification
- Holding Register Management
- Modular Embedded Software Design

---

# Conclusion

This project demonstrates the implementation of a complete **BLE to Modbus RTU Gateway** using the ESP32 and ESP-IDF.

The gateway continuously scans BLE advertisements from supported wireless sensors without establishing BLE connections, dynamically discovers devices, extracts sensor telemetry, updates Modbus Holding Registers and serves the latest data to Modbus RTU Masters over an RS485 interface.

The modular software architecture, queue-based task synchronization, dynamic register allocation and MAC-based device identification make the gateway scalable, maintainable and suitable for industrial automation applications.

The implemented design can be extended to support additional BLE sensors, Modbus function codes and communication interfaces while preserving the existing software architecture.

# 🚀 Docker Setup & Compilation Instructions

This project runs entirely inside an isolated **Docker container** using a standardized development environment image (`espressif/idf`).

By using Docker, the ESP-IDF toolchain, Python dependencies and build environment remain isolated from the host operating system, ensuring a consistent development workflow across different machines.

---

## 1. Prerequisites

Install **Docker Desktop** and ensure that the Docker daemon is running.

https://www.docker.com/products/docker-desktop/

---


## 2. Pull the latest image

Open a terminal inside the project directory.


```bash
docker pull espressif/idf:release-v5.4
```


## 3. Set the Target Device

Open a terminal inside the project directory.

Configure the target device.

```bash
docker run --rm -v "${PWD}:/project" -w /project espressif/idf:release-v5.4 idf.py set-target esp32
```

---

## 4. Build the Project

Compile the application.

```bash
docker run --rm -v "${PWD}:/project" -w /project espressif/idf:release-v5.4 idf.py build
```

After a successful build, the firmware binaries are generated inside the `build/` directory.

---

## 4. Install python libraries for flashing the device 


```bash
pip install esptool
```
```bash
pip install pyserial
```
---

## 5. Flash the ESP32

Connect the ESP32 through USB and determine the serial port.

Flash the firmware in other terminal outside container

```bash
python -m esptool `
--chip esp32 `
--port COM3 `
--baud 460800 `
write_flash `
0x1000 build\bootloader\bootloader.bin `
0x8000 build\partition_table\partition-table.bin `
0x10000 build\app.bin
```

Replace **COM3** with the appropriate serial port.

---

## 5. Open the Serial Monitor

To observe the decoded advertisement packets:

```bash
python -m serial.tools.miniterm COM3 115200
```

Terminate the serial monitor using

```text
Ctrl + ]
```

---


# Troubleshooting

| Problem | Possible Solution |
|----------|-------------------|
| Docker container does not start | Ensure Docker Desktop is running |
| ESP32 not detected | Verify USB cable and COM port |
| Build failure | Execute `idf.py fullclean` followed by `idf.py build` |
| No BLE advertisements received | Verify the sensor is powered and broadcasting |
| Sensor data not decoded | Confirm the sensor MAC address matches the parser configuration |

---
