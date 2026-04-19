# hf-alicat-basis2-driver

Hardware-agnostic C++17 driver for **Alicat BASIS-2 mass-flow meters and
controllers** (B-Series and BC-Series). Implements the Modbus-RTU register
set documented in *DOC-MANUAL-BASIS2 Rev 4* (May 2025) over any byte-level
UART transport.

The driver is the same shape as the rest of the HardFOC `hf-*-driver`
family (see `hf-mcp9700-driver`, `hf-tle92466ed-driver`, `hf-ads7952-driver`):

- Header-only, allocation-free, exception-free.
- Generic CRTP serial adapter (`UartInterface<>`) → zero virtual dispatch.
- Single `Driver<UartT>` template parameterised on the adapter type.
- Stable `DriverError` / `DriverResult<T>` result domain.

## Repository layout

```
inc/
  alicat_basis2.hpp                 ← Driver<UartT> class
  alicat_basis2_uart_interface.hpp  ← CRTP base for transport adapters
  alicat_basis2_modbus.hpp          ← CRC-16 + frame builders/parsers
  alicat_basis2_registers.hpp       ← Holding-register address constants
  alicat_basis2_types.hpp           ← Gas / FlowUnits / BaudRate / data structs
  alicat_basis2_version.h.in        ← Version header template
cmake/
  hf_alicat_basis2_build_settings.cmake
  hf_alicat_basis2Config.cmake.in
CMakeLists.txt
README.md
LICENSE
```

## Datasheet coverage

| Datasheet feature | Driver entry point |
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
| Baud rate (reg 21) | `SetBaudRate()` *(see warning below)* |
| Factory restore (reg 80) | `FactoryRestore()` |
| Bus discovery sweep (1..247) | `DiscoverPresentAddresses()` |

The driver intentionally focuses on the **Modbus-RTU** path because it's the
only one that scales to multidrop RS-485 with up to 247 instruments on a
single MCU UART. The ASCII protocol is documented for completeness; if you
need it, build it on top of the same CRTP `UartInterface` adapter — the
parsing and formatting are simple textual operations.

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

Each instrument carries:
- A **Modbus address** (1..247) used as the slave id on the bus.
- An **ASCII unit_id** (`A`..`Z`) used by the legacy ASCII protocol.

Out of the factory **all instruments default to address 1 / unit `A`**.
The `DiscoverPresentAddresses()` sweep + `SetModbusAddress()` /
`SetAsciiUnitId()` can be used at first bring-up to enumerate and program
a fresh bench one instrument at a time (connect them physically one at a
time, change address, then connect the next).

## Wiring an adapter (ESP-IDF example)

```cpp
#include "alicat_basis2.hpp"
#include "BaseUart.h"

class AlicatUartEspAdapter
    : public alicat_basis2::UartInterface<AlicatUartEspAdapter> {
public:
    explicit AlicatUartEspAdapter(BaseUart& uart) : uart_(uart) {}

    void write(const uint8_t* data, std::size_t len) noexcept {
        uart_.Write(data, static_cast<hf_u16_t>(len));
    }
    std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept {
        // BaseUart::Read returns the number actually read; treat <=0 as 0.
        const auto rc = uart_.Read(out, static_cast<hf_u16_t>(max), timeout_ms);
        return (rc == hf_uart_err_t::UART_SUCCESS) ? max : 0;
    }
    void flush_rx() noexcept { (void)uart_.FlushRx(); }
    void delay_ms_impl(uint32_t ms) noexcept { vTaskDelay(pdMS_TO_TICKS(ms)); }

private:
    BaseUart& uart_;
};

AlicatUartEspAdapter adapter{uart};
alicat_basis2::Driver<AlicatUartEspAdapter> meter(adapter, /*addr=*/1);

auto id = meter.ReadIdentity();
if (id.ok()) {
    ESP_LOGI("BASIS2", "FW %u.%u.%u  SN %s  units=%s",
             id.value.fw_major, id.value.fw_minor, id.value.fw_patch,
             id.value.serial_number,
             alicat_basis2::ToString(id.value.flow_units).data());
}

auto live = meter.ReadInstantaneous();
if (live.ok()) {
    ESP_LOGI("BASIS2", "%.2f °C  %.3f flow  setpoint=%.3f  drive=%.1f%%",
             live.value.temperature_c, live.value.flow,
             live.value.setpoint, live.value.valve_drive_pct);
}
```

## License

See `LICENSE` (MIT).
