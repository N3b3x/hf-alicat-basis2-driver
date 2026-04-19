---
layout: default
title: "🧩 CMake integration"
nav_order: 5
parent: "📚 Documentation"
permalink: /docs/cmake_integration/
---

# CMake integration

The driver ships a complete CMake project (`CMakeLists.txt` +
`cmake/hf_alicat_basis2_build_settings.cmake`) so it can be consumed
three ways:

## 1. `add_subdirectory()`

```cmake
add_subdirectory(third_party/hf-alicat-basis2-driver)
target_link_libraries(my_app PRIVATE hf::alicat_basis2)
```

`hf::alicat_basis2` is an `INTERFACE` target — it only carries include
directories and the C++17 feature requirement.

## 2. Manual include of the build-settings file

When you don't want the driver's `CMakeLists.txt` to be processed (for
example inside an ESP-IDF component), include the build-settings
fragment directly:

```cmake
include("${HF_ALICAT_BASIS2_ROOT}/cmake/hf_alicat_basis2_build_settings.cmake")
target_include_directories(my_lib PRIVATE ${HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS})
target_compile_features(my_lib PRIVATE cxx_std_17)
```

## 3. ESP-IDF component wrapper

The example tree under `examples/esp32/components/hf_alicat_basis2/`
demonstrates the standard wrapper:

```cmake
set(HF_ALICAT_BASIS2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH
    "Path to hf-alicat-basis2-driver root")

include("${HF_ALICAT_BASIS2_ROOT}/cmake/hf_alicat_basis2_build_settings.cmake")

idf_component_register(
    INCLUDE_DIRS ${HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS}
    REQUIRES ${HF_ALICAT_BASIS2_IDF_REQUIRES}
)
```

`HF_ALICAT_BASIS2_IDF_REQUIRES` currently lists `driver` so the example
also pulls in ESP-IDF's UART driver. Add `esp_driver_uart` to your own
component if you need the modern split-driver headers.

## Variables exported by `hf_alicat_basis2_build_settings.cmake`

| Variable                              | Meaning                                                 |
| ------------------------------------- | ------------------------------------------------------- |
| `HF_ALICAT_BASIS2_TARGET_NAME`        | INTERFACE library name (`hf_alicat_basis2`)             |
| `HF_ALICAT_BASIS2_VERSION`            | `MAJOR.MINOR.PATCH`                                     |
| `HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS`| Public include paths (driver + generated version dir)   |
| `HF_ALICAT_BASIS2_SOURCE_FILES`       | Empty — the driver is header-only                       |
| `HF_ALICAT_BASIS2_IDF_REQUIRES`       | Component dependencies for ESP-IDF integration          |

## Generated header

The build-settings script runs `configure_file()` on
`inc/alicat_basis2_version.h.in` to produce
`<build>/.../hf_alicat_basis2_generated/alicat_basis2_version.h` so user
code can `#include "alicat_basis2_version.h"` for `HF_ALICAT_BASIS2_VERSION_*`
macros.

**Next:** [API reference →](api_reference.md)
