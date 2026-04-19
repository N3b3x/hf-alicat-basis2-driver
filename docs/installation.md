---
layout: default
title: "📥 Installation"
nav_order: 1
parent: "📚 Documentation"
permalink: /docs/installation/
---

# Installation

The driver is **header-only**. There is nothing to build, link or install
in the traditional sense — adding the `inc/` folder to your include path
is enough. The CMake glue exists to:

- Generate `alicat_basis2_version.h` from the project version.
- Provide an `INTERFACE` target (`hf::alicat_basis2`) that consumers can
  `target_link_libraries` against.

## Requirements

- **C++17** (the driver uses `if constexpr`, `std::array`, `std::string_view`).
- **CMake ≥ 3.16** for the in-tree `CMakeLists.txt`.
- A **byte-level UART** transport (any project / RTOS — provide a CRTP
  adapter, see [Quick start](quickstart.md)).
- For ESP-IDF builds: ESP-IDF **≥ 5.4** is recommended; the example
  components target ESP-IDF 5.5 (set in `examples/esp32/app_config.yml`).

## Layout

```
hf-alicat-basis2-driver/
├── inc/                                     ← public headers
│   ├── alicat_basis2.hpp                    ← Driver<UartT> class
│   ├── alicat_basis2_uart_interface.hpp     ← CRTP serial transport base
│   ├── alicat_basis2_modbus.hpp             ← Frame + CRC helpers
│   ├── alicat_basis2_registers.hpp          ← Holding-register addresses
│   ├── alicat_basis2_types.hpp              ← Result / data structures
│   └── alicat_basis2_version.h.in           ← Version header template
├── cmake/
│   ├── hf_alicat_basis2_build_settings.cmake
│   └── hf_alicat_basis2Config.cmake.in
├── examples/esp32/                          ← ESP-IDF example projects
├── docs/                                    ← This documentation
├── _config/                                 ← Doxygen + Jekyll site config
├── .github/workflows/                       ← CI for lint, docs, examples
├── CMakeLists.txt
├── LICENSE                                  ← MIT
└── README.md
```

## Vendoring as a Git submodule (recommended)

```bash
git submodule add https://github.com/N3b3x/hf-alicat-basis2-driver.git \
    third_party/hf-alicat-basis2-driver
```

Then in your CMake project:

```cmake
add_subdirectory(third_party/hf-alicat-basis2-driver)
target_link_libraries(my_app PRIVATE hf::alicat_basis2)
```

## Vendoring as an ESP-IDF component

Use the wrapper component already shipped under `examples/esp32/components/hf_alicat_basis2/`
or copy it next to your own project:

```cmake
# components/hf_alicat_basis2/CMakeLists.txt
set(HF_ALICAT_BASIS2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../third_party/hf-alicat-basis2-driver"
    CACHE PATH "Path to hf-alicat-basis2-driver root")
include("${HF_ALICAT_BASIS2_ROOT}/cmake/hf_alicat_basis2_build_settings.cmake")

idf_component_register(
    INCLUDE_DIRS ${HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS}
    REQUIRES ${HF_ALICAT_BASIS2_IDF_REQUIRES}
)
```

## Verifying the version header

After CMake configure you should see a generated header at
`<build>/.../hf_alicat_basis2_generated/alicat_basis2_version.h`
containing:

```c
#define HF_ALICAT_BASIS2_VERSION_MAJOR 1
#define HF_ALICAT_BASIS2_VERSION_MINOR 0
#define HF_ALICAT_BASIS2_VERSION_PATCH 0
#define HF_ALICAT_BASIS2_VERSION_STRING "1.0.0"
```

If you don't see it, confirm that `cmake/hf_alicat_basis2_build_settings.cmake`
was included by your build (it is automatically when you `add_subdirectory(...)`
the driver).

**Next:** [Quick start →](quickstart.md)
