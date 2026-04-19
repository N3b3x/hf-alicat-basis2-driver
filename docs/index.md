---
layout: default
title: "📚 Documentation"
description: "Complete documentation for the HardFOC Alicat BASIS-2 driver"
nav_order: 2
parent: "HardFOC Alicat BASIS-2 Driver"
permalink: /docs/
has_children: true
---

# HF-Alicat-BASIS2 documentation

Welcome. This site mirrors the [`docs/`](https://github.com/N3b3x/hf-alicat-basis2-driver/tree/main/docs) folder in the repository and adds navigation, search, and diagrams for the **Alicat BASIS-2** mass-flow meter / controller (B-Series and BC-Series) driver.

> **Browse on GitHub:** [repository home](https://github.com/N3b3x/hf-alicat-basis2-driver) · [Issues](https://github.com/N3b3x/hf-alicat-basis2-driver/issues)

## Documentation structure

### Getting started

1. **[Installation](installation.md)** — Compiler / CMake requirements and include layout
2. **[Quick start](quickstart.md)** — Smallest CRTP serial adapter you need to read live data
3. **[Modbus protocol](modbus_protocol.md)** — Frame format, register map highlights, CRC

### Hardware & integration

4. **[Hardware setup — RS-485 bus](hardware_setup.md)** — Pinout, transceiver, multidrop wiring
5. **[CMake integration](cmake_integration.md)** — `hf_alicat_basis2_build_settings.cmake`, `hf::alicat_basis2` target

### Reference & examples

6. **[API reference](api_reference.md)** — `Driver<UartT>`, `UartInterface<>`, types, results
7. **[Examples](examples.md)** — ESP-IDF examples (minimal + comprehensive) and build/flash flow
8. **[Troubleshooting](troubleshooting.md)** — Bus discovery, baud changes, exception codes

### Manufacturer

9. **[Datasheet & links](datasheet/README.md)** — Official Alicat documentation and product pages

---

## Recommended reading order

1. [Installation](installation.md) — ensure the version header generation is wired
2. [Hardware setup](hardware_setup.md) — wire the RS-485 transceiver and chain instruments
3. [Quick start](quickstart.md) — implement `write` / `read` / `flush_rx` for your platform
4. [Modbus protocol](modbus_protocol.md) — understand the framing the driver speaks
5. [Examples](examples.md) — run the ESP32-S3 minimal or comprehensive project

---

## Visual overview

| Diagram | File |
| --- | --- |
| RS-485 multidrop topology (default example) | [`assets/basis2-rs485-topology.svg`](assets/basis2-rs485-topology.svg) |
| Modbus-RTU read-holding round-trip | [`assets/basis2-modbus-frame.svg`](assets/basis2-modbus-frame.svg) |

---

## Need help?

- **Build or flash issues:** [Troubleshooting](troubleshooting.md) and [Examples](examples.md)
- **Bus / wiring:** [Hardware setup](hardware_setup.md)
- **API details:** [API reference](api_reference.md) and the headers in [`inc/`](https://github.com/N3b3x/hf-alicat-basis2-driver/tree/main/inc) on GitHub

**Next:** [Installation →](installation.md)
