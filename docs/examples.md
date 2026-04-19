---
layout: default
title: "🧪 Examples"
nav_order: 7
parent: "📚 Documentation"
permalink: /docs/examples/
---

# Examples

Two ESP-IDF projects ship under `examples/esp32/`:

| App                                | Source file                              | Purpose                                              |
| ---------------------------------- | ---------------------------------------- | ---------------------------------------------------- |
| `alicat_basis2_minimal_example`    | `alicat_basis2_minimal_example.cpp`      | Smallest-possible read loop                          |
| `alicat_basis2_full_example`       | `alicat_basis2_full_example.cpp`         | Discovery + identity + configuration walkthrough + measurement window + periodic instantaneous loop |

Both target the **ESP32-S3** by default and use `UART1` on
TX=GPIO17 / RX=GPIO18 with the DE/RE pin on GPIO8. Edit the constants
at the top of the source file for your wiring.

## Build with the project tools (recommended)

The example tree includes the [`hf-espidf-project-tools`](https://github.com/N3b3x/hf-espidf-project-tools)
submodule under `examples/esp32/scripts/`:

```bash
git submodule update --init --recursive

cd examples/esp32
./scripts/build_app.sh list
./scripts/build_app.sh alicat_basis2_minimal_example Debug
./scripts/flash_app.sh flash_monitor alicat_basis2_minimal_example Debug
```

## Build with `idf.py` directly

```bash
cd examples/esp32
idf.py set-target esp32s3
idf.py -B build \
  "-DAPP_TYPE=alicat_basis2_full_example" \
  "-DBUILD_TYPE=Debug" \
  "-DAPP_SOURCE_FILE=alicat_basis2_full_example.cpp" \
  build
idf.py -B build flash monitor
```

## Layout

```
examples/esp32/
├── CMakeLists.txt
├── app_config.yml
├── sdkconfig.defaults
├── components/hf_alicat_basis2/    # wraps the driver as an IDF component
├── main/
│   ├── CMakeLists.txt
│   ├── alicat_basis2_minimal_example.cpp
│   └── alicat_basis2_full_example.cpp
└── scripts/                        # git submodule → hf-espidf-project-tools
```

## What the comprehensive example demonstrates

1. UART1 brought up in `UART_MODE_RS485_HALF_DUPLEX`.
2. `DiscoverPresentAddresses()` sweep reporting which addresses are alive.
3. `ReadIdentity()` printing firmware / serial / unit-id / units / full-scale.
4. Configuration walkthrough — `SetGas`, `SetReferenceTemperatureC`,
   `SetFlowAveragingMs`, `SetTotalizerLimitMode`, `SetCommWatchdogMs`,
   `SetSetpointSource(DigitalUnsaved)`, `SetSetpoint(2.5)`, `Tare`,
   `ResetTotalizer`.
5. Measurement-window demo — `ConfigureMeasurementTrigger`,
   `StartMeasurementSamples(200)`, `ReadMeasurement` after the window.
6. Steady-state 1 Hz `ReadInstantaneous()` loop.

**Next:** [Troubleshooting →](troubleshooting.md)
