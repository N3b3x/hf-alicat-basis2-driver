# ESP32 Alicat BASIS-2 examples (ESP32-S3 default)

ESP-IDF examples for the **Alicat BASIS-2** mass-flow meter / controller
talking Modbus-RTU over RS-485 to an ESP32-S3 UART.

**Documentation:** [Docs home](../../docs/index.md) · [Hardware setup](../../docs/hardware_setup.md) · [Examples guide](../../docs/examples.md) · [ESP-IDF UART (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html)

## Submodule (required for `./scripts/build_app.sh`)

Initialize the shared tooling submodule once:

```bash
# From repository root
git submodule update --init --recursive
```

This checks out `examples/esp32/scripts` ([hf-espidf-project-tools](https://github.com/N3b3x/hf-espidf-project-tools))
so you can build and flash from `examples/esp32/` in this driver repo or
from any consuming repository that vendors the same layout.

## Wiring (default configuration)

| Signal              | ESP32-S3 (default) |
| ------------------- | ------------------ |
| RS-485 TX (DI)      | **GPIO17**         |
| RS-485 RX (RO)      | **GPIO18**         |
| RS-485 DE / RE      | **GPIO8**          |
| Logic supply (3V3)  | 3.3 V to transceiver |
| Common GND          | shared with bus    |

Edit the `kTxGpio` / `kRxGpio` / `kRtsDeRe` constants in the example
source file if you use different pins. Set `kRtsDeRe = -1` if your
transceiver pair manages DE/RE on its own (e.g. dual-channel
auto-direction transceivers).

## Build with scripts

```bash
cd examples/esp32
./scripts/build_app.sh list
./scripts/build_app.sh alicat_basis2_minimal_example Debug
./scripts/flash_app.sh flash_monitor alicat_basis2_minimal_example Debug
```

## Build with idf.py

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
├── components/hf_alicat_basis2/     # wraps driver headers + generated version path
├── main/
│   ├── CMakeLists.txt
│   ├── alicat_basis2_minimal_example.cpp
│   └── alicat_basis2_full_example.cpp
└── scripts/                         # git submodule → hf-espidf-project-tools
```
