---
layout: default
title: "HardFOC Alicat BASIS-2 Driver"
description: "Hardware-agnostic C++17 driver for Alicat BASIS-2 mass-flow meters and controllers (B-Series & BC-Series)"
nav_order: 1
permalink: /
---

# HF-Alicat-BASIS2 Driver

**Header-only C++17 driver for Alicat BASIS-2 mass-flow meters and controllers (B-Series & BC-Series) over Modbus-RTU**

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![CI](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/esp32-examples-build-ci.yml/badge.svg?branch=main)](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/esp32-examples-build-ci.yml)
[![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://n3b3x.github.io/hf-alicat-basis2-driver/)

## 📚 Table of Contents
1. [Overview](#-overview)
2. [Features](#-features)
3. [Quick Start](#-quick-start)
4. [Installation](#-installation)
5. [API Reference](#-api-reference)
6. [Examples](#-examples)
7. [Documentation](#-documentation)
8. [Contributing](#-contributing)
9. [License](#-license)

## 📦 Overview

> **📖 [📚🌐 Live Complete Documentation](https://n3b3x.github.io/hf-alicat-basis2-driver/)** —
> Interactive guides, hardware diagrams, and step-by-step tutorials.

**HF-Alicat-BASIS2** is a header-only C++17 driver for the **Alicat BASIS-2** mass-flow
meters and controllers (B-Series & BC-Series). It implements the Modbus-RTU register
set documented in *DOC-MANUAL-BASIS2 Rev 4* (May 2025) over any byte-level UART
transport via a CRTP serial adapter — **zero dynamic allocation, zero exceptions, zero
virtual dispatch**.

The driver intentionally focuses on the **Modbus-RTU** path because it scales to
multidrop RS-485 with up to 247 instruments on a single MCU UART. The ASCII protocol
is documented for completeness; if you need it, build it on top of the same CRTP
`UartInterface` adapter.

### Bus topology

```text
   ┌────────────┐   RS-232 / RS-485
   │  MCU UART  ├── (DE/RE auto-toggled by ESP-IDF UART RS-485 mode)
   └─────┬──────┘
         │ shared bus
   ┌─────┴───────────────────────────────────────────────┐
   │                                                     │
┌──┴───┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ...  ┌──────┐
│ B-2  │  │ B-2  │  │ B-2  │  │ B-2  │  │ B-2  │       │ B-2  │
│ #01  │  │ #02  │  │ #03  │  │ #04  │  │ #05  │       │ #247 │
└──────┘  └──────┘  └──────┘  └──────┘  └──────┘       └──────┘
```

Each instrument carries a **Modbus address** (1..247) and an **ASCII unit_id** (`A`..`Z`).
Out of the factory **all instruments default to address 1 / unit `A`**. See
[Hardware setup](docs/hardware_setup.md) for the first-bring-up procedure.

## ✨ Features

- ✅ **Header-only C++17**: no static library required for the driver itself
- ✅ **Hardware Agnostic**: CRTP `UartInterface<>` adapter, zero virtual dispatch
- ✅ **Allocation-free / Exception-free**: safe for ISR-adjacent and freestanding builds
- ✅ **Modbus-RTU Coverage**: full register map per *DOC-MANUAL-BASIS2 Rev 4* (May 2025)
- ✅ **Multidrop RS-485**: up to 247 instruments on a single MCU UART
- ✅ **Bus discovery sweep** (`DiscoverPresentAddresses()`) for multi-baud bring-up
- ✅ **Full instantaneous data frame** read (flow, temp, pressure, drive, totalizer in one shot)
- ✅ **Setpoint, watchdog, ramp, tare, autotare, gas selection, totalizer batch**
- ✅ **Sampled measurement window** (regs 4201..4214) for triggered batch acquisition
- ✅ **Stable error domain**: `DriverError` / `DriverResult<T>` (no exceptions)
- ✅ **ESP-IDF Ready**: IDF component wrapper + minimal & full RS-485 examples

## 🚀 Quick Start

```cpp
#include "alicat_basis2.hpp"
#include "alicat_basis2_uart_interface.hpp"

// 1. Implement the UART interface (see docs/quickstart.md)
class MyUart : public alicat_basis2::UartInterface<MyUart> {
public:
    void write(const uint8_t* data, std::size_t len) noexcept;
    std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept;
    void flush_rx() noexcept;
};

// 2. Create driver instance bound to a Modbus address
MyUart uart;
alicat_basis2::Driver<MyUart> meter(uart, /*addr=*/1);

// 3. Read identity + live data
auto id = meter.ReadIdentity();
if (id.ok()) {
    printf("FW %u.%u.%u  SN=%s\n",
           id.value.fw_major, id.value.fw_minor, id.value.fw_patch,
           id.value.serial_number);
}

if (auto live = meter.ReadInstantaneous(); live.ok()) {
    printf("flow=%.4f  T=%.2f C  drive=%.1f%%\n",
           static_cast<double>(live.value.flow),
           static_cast<double>(live.value.temperature_c),
           static_cast<double>(live.value.valve_drive_pct));
}
```

For detailed setup, see [Installation](docs/installation.md) and [Quick Start Guide](docs/quickstart.md).

## 🔧 Installation

1. **Clone or copy** the driver files into your project (header-only — no build step)
2. **Implement the UART interface** for your platform (see [Platform Integration](docs/quickstart.md))
3. **Include the header** in your code:
   ```cpp
   #include "alicat_basis2.hpp"
   ```
4. Compile with a **C++17** or newer compiler

For detailed installation instructions, see [docs/installation.md](docs/installation.md).

## 📖 API Reference

### Live data & identity

| Method | Description |
|--------|-------------|
| `ReadInstantaneous()` | Read full instantaneous data frame (flow, T, P, drive, totalizer) |
| `ReadIdentity()` | Firmware, serial number, address, units, full-scale |

### Setpoint & ramp control (BC-Series)

| Method | Description |
|--------|-------------|
| `SetSetpoint(value)` | Setpoint command (regs 2053-4) |
| `SetSetpointSource(src)` | Choose analog / saved / unsaved source (reg 516) |
| `SetMaxSetpointRamp(ms)` | Maximum setpoint ramp (regs 524-5) |
| `SetCommWatchdogMs(ms)` | Communication watchdog (reg 514) |

### Calibration & gas

| Method | Description |
|--------|-------------|
| `Tare()` | Trigger tare (reg 39) |
| `SetAutotareEnabled(en)` | Auto-tare on/off (reg 515) |
| `SetGas(g)` | Active gas (reg 2100) |
| `SetReferenceTemperatureC(T)` | Reference temperature (reg 52) |
| `SetFlowAveragingMs(ms)` | Flow averaging window (reg 55) |

### Totalizer

| Method | Description |
|--------|-------------|
| `ResetTotalizer()` | Totalizer reset (reg 53) |
| `SetTotalizerLimitMode(mode)` | Totalizer overflow mode (reg 54) |
| `SetTotalizerBatch(volume)` | Totalizer batch volume (regs 521-2) |

### Sampled measurement window

| Method | Description |
|--------|-------------|
| `ConfigureMeasurementTrigger(cfg)` | Measurement triggering (reg 4200) |
| `StartMeasurementSamples()` + `ReadMeasurement()` | Sampled window (regs 4201..4214) |

### Bus configuration & discovery

| Method | Description |
|--------|-------------|
| `SetModbusAddress(addr)` | Modbus address (reg 45) |
| `SetAsciiUnitId(id)` | ASCII unit id (reg 46) |
| `SetBaudRate(baud)` | Baud rate (reg 21) — see hardware setup |
| `FactoryRestore()` | Factory restore (reg 80) |
| `DiscoverPresentAddresses()` | Bus discovery sweep (1..247), multi-baud |

For complete API documentation, see [docs/api_reference.md](docs/api_reference.md).

## 📊 Examples

| Example | Description |
|---------|-------------|
| **[alicat_basis2_minimal_example.cpp](examples/esp32/main/alicat_basis2_minimal_example.cpp)** | Smallest possible read loop (~80 LOC) |
| **[alicat_basis2_full_example.cpp](examples/esp32/main/alicat_basis2_full_example.cpp)** | Full feature exercise: setpoint, ramp, tare, totalizer (~180 LOC) |

For ESP32 build instructions, see the [examples/esp32](examples/esp32/) directory.

Detailed example walkthroughs are available in [docs/examples.md](docs/examples.md).

## 📚 Documentation

Complete documentation is available in the [docs directory](docs/index.md):

| Guide | Description |
|-------|-------------|
| [📥 Installation](docs/installation.md) | Header-only integration, CMake glue, generated headers |
| [🚀 Quick Start](docs/quickstart.md) | CRTP adapter implementation + first read |
| [🔌 Hardware Setup](docs/hardware_setup.md) | RS-485 wiring, JST-GH pinout, first-bring-up procedure |
| [📡 Modbus Protocol](docs/modbus_protocol.md) | Function codes, frame layout, CRC, exception handling |
| [🧩 CMake Integration](docs/cmake_integration.md) | `add_subdirectory()`, FetchContent, IDF component |
| [📖 API Reference](docs/api_reference.md) | Driver class, types, error codes, register map |
| [🧪 Examples](docs/examples.md) | Walkthrough of the ESP-IDF example apps |
| [🛠️ Troubleshooting](docs/troubleshooting.md) | Bus discovery flow, multi-baud normalisation, common errors |
| [📄 Datasheet](docs/datasheet/DOC-MANUAL-BASIS2.pdf) | Bundled official manual (Rev 4, May 2025) |

## 🔗 References

| Resource | Link |
|----------|------|
| Alicat BASIS-2 product page | <https://www.alicat.com/product/basis2-mass-flow-meters-controllers/> |
| BASIS-2 manual (DOC-MANUAL-BASIS2 Rev 4) | [docs/datasheet/](docs/datasheet/) |
| Modbus over Serial Line v1.02 | <https://modbus.org/specs.php> |
| ESP-IDF UART RS-485 mode | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html#rs485-mode> |
| C++17 language reference | <https://en.cppreference.com/w/cpp/17> |

## 🤝 Contributing

Pull requests and suggestions are welcome! Please follow the existing code style and include tests for new features.

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

The driver is **not** affiliated with or endorsed by Alicat Scientific. The register
map and command syntax are implemented from the public *DOC-MANUAL-BASIS2 Rev 4*
(May 2025).
