---
layout: home
title: "🌬️ Alicat BASIS-2 Driver"
description: "Hardware-agnostic C++17 driver for Alicat BASIS-2 mass-flow meters and controllers"
nav_order: 1
permalink: /
---

# hf-alicat-basis2-driver

[![ESP32 build CI](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/esp32-examples-build-ci.yml/badge.svg)](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/esp32-examples-build-ci.yml)
[![C++ lint](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/ci-cpp-lint.yml/badge.svg)](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/ci-cpp-lint.yml)
[![Markdown lint](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/ci-markdown-lint.yml/badge.svg)](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/ci-markdown-lint.yml)
[![Docs publish](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/ci-docs-publish.yml/badge.svg)](https://github.com/N3b3x/hf-alicat-basis2-driver/actions/workflows/ci-docs-publish.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Header-only C++17 driver for **Alicat BASIS-2 mass-flow meters and
controllers** (B-Series and BC-Series). Implements the Modbus-RTU
register set documented in *DOC-MANUAL-BASIS2 Rev 4* (May 2025) over
any byte-level UART transport via a CRTP serial adapter.

The driver is the same shape as the rest of the `hf-*-driver` family:

- Header-only, allocation-free, exception-free.
- Generic CRTP serial adapter (`UartInterface<>`) → zero virtual dispatch.
- Single `Driver<UartT>` template parameterised on the adapter type.
- Stable `DriverError` / `DriverResult<T>` result domain.

📚 **[Documentation](https://github.com/N3b3x/hf-alicat-basis2-driver/tree/main/docs)** ·
🛠️ **[Examples](examples/esp32/README.md)** ·
📄 **[Datasheet (PDF)](docs/datasheet/DOC-MANUAL-BASIS2.pdf)** ·
🐛 **[Issues](https://github.com/N3b3x/hf-alicat-basis2-driver/issues)**

---

## Repository layout

```
hf-alicat-basis2-driver/
├── inc/                                     ← public headers
│   ├── alicat_basis2.hpp                    ← Driver<UartT> class
│   ├── alicat_basis2_uart_interface.hpp     ← CRTP base for transport adapters
│   ├── alicat_basis2_modbus.hpp             ← CRC-16 + frame builders/parsers
│   ├── alicat_basis2_registers.hpp          ← Holding-register address constants
│   ├── alicat_basis2_types.hpp              ← Gas / FlowUnits / BaudRate / data structs
│   └── alicat_basis2_version.h.in           ← Version header template
├── cmake/
│   ├── hf_alicat_basis2_build_settings.cmake
│   └── hf_alicat_basis2Config.cmake.in
├── examples/esp32/
│   ├── CMakeLists.txt + app_config.yml + sdkconfig.defaults
│   ├── components/hf_alicat_basis2/         ← IDF component wrapper
│   ├── main/
│   │   ├── alicat_basis2_minimal_example.cpp   (~80 LOC)
│   │   └── alicat_basis2_full_example.cpp      (~180 LOC)
│   └── scripts/                             ← submodule: hf-espidf-project-tools
├── docs/                                    ← Markdown docs (rendered by Jekyll)
│   ├── index.md / installation.md / quickstart.md
│   ├── modbus_protocol.md / hardware_setup.md / cmake_integration.md
│   ├── api_reference.md / examples.md / troubleshooting.md
│   └── datasheet/                           ← drop the official PDF here
├── _config/                                 ← Jekyll + Doxygen scaffolding
│   ├── Doxyfile + _config.yml + _layouts + _includes
│   ├── doxygen-extensions/doxygen-awesome-css   ← submodule
│   └── lint configs (.clang-format, .clang-tidy, .markdownlint.json, .yamllint)
├── .github/workflows/                       ← 8 CI workflows (lint / build / docs / release)
├── CMakeLists.txt
├── LICENSE                                  ← MIT
└── README.md
```

## Datasheet feature coverage

| Datasheet section | Driver entry point |
|-------------------|--------------------|
| Read full instantaneous data frame (regs 2100..2109) | `ReadInstantaneous()` |
| Identity (firmware, serial, address, units, full-scale) | `ReadIdentity()` |
| Setpoint command (regs 2053-4) | `SetSetpoint()` |
| Setpoint source (analog / saved / unsaved) (reg 516) | `SetSetpointSource()` |
| Communication watchdog (reg 514) | `SetCommWatchdogMs()` |
| Maximum setpoint ramp (regs 524-5) | `SetMaxSetpointRamp()` |
| Tare (reg 39) | `Tare()` |
| Autotare on / off (reg 515) | `SetAutotareEnabled()` |
| Active gas (reg 2100) | `SetGas()` |
| Totalizer reset (reg 53) | `ResetTotalizer()` |
| Totalizer overflow mode (reg 54) | `SetTotalizerLimitMode()` |
| Totalizer batch volume (regs 521-2) | `SetTotalizerBatch()` |
| Flow averaging (reg 55) | `SetFlowAveragingMs()` |
| Reference temperature (reg 52) | `SetReferenceTemperatureC()` |
| Measurement triggering (reg 4200) | `ConfigureMeasurementTrigger()` |
| Sampled measurement window (regs 4201..4214) | `StartMeasurementSamples()` + `ReadMeasurement()` |
| Modbus address (reg 45) | `SetModbusAddress()` |
| ASCII unit id (reg 46) | `SetAsciiUnitId()` |
| Baud rate (reg 21) | `SetBaudRate()` *(see hardware setup)* |
| Factory restore (reg 80) | `FactoryRestore()` |
| Bus discovery sweep (1..247) | `DiscoverPresentAddresses()` |

The driver intentionally focuses on the **Modbus-RTU** path because it
scales to multidrop RS-485 with up to 247 instruments on a single MCU
UART. The ASCII protocol is documented for completeness; if you need it,
build it on top of the same CRTP `UartInterface` adapter — the parsing
and formatting are simple textual operations.

## Bus topology

```
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

Each instrument carries a **Modbus address** (1..247) and an **ASCII
unit_id** (`A`..`Z`). Out of the factory **all instruments default to
address 1 / unit `A`**. See [Hardware setup](docs/hardware_setup.md) for
the first-bring-up procedure.

## Quick example

```cpp
#include "alicat_basis2.hpp"
#include "alicat_basis2_uart_interface.hpp"

class MyUart : public alicat_basis2::UartInterface<MyUart> {
public:
    void write(const uint8_t* data, std::size_t len) noexcept;
    std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept;
    void flush_rx() noexcept;
};

MyUart uart;
alicat_basis2::Driver<MyUart> meter(uart, /*addr=*/1);

auto id = meter.ReadIdentity();
if (id.ok()) {
    printf("FW %u.%u.%u  SN=%s  units=%s\n",
           id.value.fw_major, id.value.fw_minor, id.value.fw_patch,
           id.value.serial_number,
           alicat_basis2::ToString(id.value.flow_units).data());
}

if (auto live = meter.ReadInstantaneous(); live.ok()) {
    printf("flow=%.4f  T=%.2f C  drive=%.1f%%\n",
           static_cast<double>(live.value.flow),
           static_cast<double>(live.value.temperature_c),
           static_cast<double>(live.value.valve_drive_pct));
}
```

See [`docs/quickstart.md`](docs/quickstart.md) for the full guide and
[`examples/esp32/`](examples/esp32/) for ESP-IDF projects you can flash
directly.

## License

[MIT](LICENSE) — see file. The driver is **not** affiliated with or
endorsed by Alicat Scientific. The register map and command syntax are
implemented from the public *DOC-MANUAL-BASIS2* Rev 4 (May 2025).
