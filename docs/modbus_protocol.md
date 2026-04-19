---
layout: default
title: "📡 Modbus protocol"
nav_order: 3
parent: "📚 Documentation"
permalink: /docs/modbus_protocol/
---

# Modbus-RTU protocol

The driver implements the **Modbus-RTU** subset that the Alicat BASIS-2
firmware actually understands, per *DOC-MANUAL-BASIS2 Rev 4*:

| Function code | Name                       | Driver entry point          |
| ------------- | -------------------------- | --------------------------- |
| 0x03          | Read Holding Registers     | Internal — used by every read API |
| 0x06          | Write Single Register      | Internal — used by every single-write API |
| 0x10          | Write Multiple Registers   | Internal — used by 32-bit writes (setpoint, batch, ramp) |

All other Modbus function codes are intentionally not implemented because
the BASIS-2 returns an exception for them.

## Frame format

```
┌──────────┬──────────┬─────────────────────────────┬──────────┐
│ slave id │  fn code │  payload (≥ 2 bytes)        │  CRC-16  │
│  1 byte  │  1 byte  │  e.g. address+count or      │  2 bytes │
│  1..247  │ 03/06/10 │  byte_count+data            │ low,high │
└──────────┴──────────┴─────────────────────────────┴──────────┘
```

CRC-16 polynomial **0xA001** (Modbus standard, LSB first), seeded at
**0xFFFF**, transmitted **little-endian** at the end of every frame.
Implementation lives in `inc/alicat_basis2_modbus.hpp` (`Crc16`,
`AppendCrc`, `CheckCrc`).

The Modbus-RTU spec also mandates a ≥ **3.5-character** idle gap between
back-to-back frames. ESP-IDF's `UART_MODE_RS485_HALF_DUPLEX` mode handles
the DE/RE timing automatically; the driver itself never needs to know
about RS-485.

## Register map highlights

The full address table lives in `inc/alicat_basis2_registers.hpp` (one
constant per register, names mirror the datasheet). The most useful are:

### Instantaneous data (one Modbus burst, 10 registers)

| Reg  | Name                  | Decoded as                                  |
| ---- | --------------------- | ------------------------------------------- |
| 2100 | Selected gas          | `Gas` enum (Air / Ar / CO2 / N2 / O2 / …)   |
| 2101 | Status bitfield       | MOV / TOV / OVR / HLD / VTM bits            |
| 2102 | Temperature (centi-°C)| `value / 100` → °C                          |
| 2103 | Instantaneous flow    | `value / 10^decimals` → user units          |
| 2104–2105 | Total volume      | 32-bit unsigned, scaled like flow           |
| 2106 | Setpoint readback     | scaled like flow                            |
| 2107 | Valve drive (centi-%) | `value / 100` → %                           |
| 2108–2109 | Batch remaining   | 32-bit, scaled like flow                    |

`Driver::ReadInstantaneous()` pulls all of these in one Modbus
transaction and returns an `InstantaneousData` struct with everything
already decoded.

### Identity (read once at boot)

| Reg   | Name             | Decoded as                       |
| ----- | ---------------- | -------------------------------- |
| 25    | Firmware version | packed `(a,b,c) = 256a+16b+c`    |
| 26–31 | Serial number    | 12 ASCII chars (2 per register)  |
| 45    | Modbus address   | 1..247                           |
| 46    | ASCII unit id    | `'A'..'Z'`                       |
| 47–48 | Full-scale flow  | unsigned 32-bit; `value / 1000` in user units |
| 49    | Flow units code  | `FlowUnits` enum (SCCM, SLPM, …) |

### Setpoint (controllers only)

| Reg       | Name                  | Notes                              |
| --------- | --------------------- | ---------------------------------- |
| 2053–2054 | Setpoint command      | Signed 32-bit, value × 1000 = user units |
| 514       | Comm watchdog (ms)    | 0–5000; 0 disables                 |
| 515       | Autotare              | 1 enable / 0 disable               |
| 516       | Setpoint source       | `0=Analog 1=DigitalSaved 2=DigitalUnsaved` |
| 524–525   | Max setpoint ramp     | %/ms × 10⁷                          |

> ⚠️ The datasheet warns that `DigitalSaved` (source = 1) writes to
> non-volatile memory on every change. Use `DigitalUnsaved` (source = 2)
> when the setpoint changes more often than once every few minutes.

### Configuration

| Reg | Name                       | Notes                                  |
| --- | -------------------------- | -------------------------------------- |
| 21  | Baud rate                  | 0=4800 1=9600 2=19200 3=38400 4=57600 5=115200 |
| 39  | Tare command               | write `0xAA55`                         |
| 52  | Reference temperature      | `value / 100` → °C, 0..30              |
| 53  | Totalizer reset            | write `0xAA55`                         |
| 54  | Totalizer overflow mode    | 0..3 (saturate / reset, with/without OVR) |
| 55  | Flow averaging (ms)        | 0..2500 — IIR-style averaging on the chip |
| 56  | Command protocol           | 0=BASIS 2 / 1=BASIS 1                  |
| 80  | Factory restore command    | write `0x5214`                         |

## Exception handling

When the slave doesn't implement a register or rejects a write, it
replies with a Modbus exception (function code OR'd with `0x80`). The
driver maps this to `DriverError::ModbusException`. A `Timeout` means
the slave never responded (wrong address, baud mismatch, broken bus).

Every API call returns `DriverResult<T>`:

```cpp
auto rc = meter.ReadInstantaneous();
if (!rc.ok()) {
    switch (rc.error) {
        case alicat_basis2::DriverError::Timeout:        /* bus / address */ break;
        case alicat_basis2::DriverError::BadCrc:         /* line noise */    break;
        case alicat_basis2::DriverError::ModbusException:/* meter NACKed */  break;
        default: /* see ToString(rc.error) */                                break;
    }
}
```

**Next:** [Hardware setup →](hardware_setup.md)
