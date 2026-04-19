---
layout: default
title: "📖 API reference"
nav_order: 6
parent: "📚 Documentation"
permalink: /docs/api_reference/
---

# API reference

The driver lives entirely in the `alicat_basis2` namespace; everything
relevant to consumers is in five public headers.

## `Driver<UartT>` (alicat_basis2.hpp)

Templated on the CRTP serial adapter. One `Driver` instance corresponds
to **one Modbus slave** on the bus.

### Construction

```cpp
Driver(UartT& uart, uint8_t address,
       uint16_t timeout_ms = kDefaultTimeoutMs);
```

- `uart` — reference to a `UartInterface<>` adapter (must outlive the driver).
- `address` — Modbus address `1..247`, or `kBroadcastAddress` (0) for
  unaddressed broadcast writes.
- `timeout_ms` — per-transaction response timeout (default 200 ms).

### Live data

| Method                           | Returns                                  |
| -------------------------------- | ---------------------------------------- |
| `ReadInstantaneous(decimals=3)`  | `DriverResult<InstantaneousData>`        |
| `ReadMeasurement()`              | `DriverResult<MeasurementData>`          |
| `ReadIdentity()`                 | `DriverResult<InstrumentIdentity>`       |

### Setpoint / control (controllers only)

| Method                                       | Notes                                    |
| -------------------------------------------- | ---------------------------------------- |
| `SetSetpoint(float user_units, decimals=3)`  | Datasheet: regs 2053-4, value × 1000     |
| `SetSetpointSource(SetpointSource)`          | Analog / DigitalSaved / DigitalUnsaved   |
| `SetCommWatchdogMs(uint16_t)`                | 0–5000, 0 disables                       |
| `SetMaxSetpointRamp(uint32_t pct_per_ms_x_10e7)` | regs 524-5                            |

### Configuration

| Method                                                   |
| -------------------------------------------------------- |
| `Tare()`                                                 |
| `SetAutotareEnabled(bool)`                               |
| `SetGas(Gas)`                                            |
| `ResetTotalizer()`                                       |
| `SetTotalizerLimitMode(TotalizerLimitMode)`              |
| `SetTotalizerBatch(uint32_t value_scaled)`               |
| `SetFlowAveragingMs(uint16_t ms)`                        |
| `SetReferenceTemperatureC(float c)`                      |
| `ConfigureMeasurementTrigger(uint16_t trigger_bits)`     |
| `StartMeasurementSamples(uint16_t num_samples)`          |

### Bus reconfiguration

| Method                                  | Notes                                 |
| --------------------------------------- | ------------------------------------- |
| `SetModbusAddress(uint8_t)`             | Driver retargets new address on success |
| `SetAsciiUnitId(char)`                  | `'A'..'Z'`                            |
| `SetBaudRate(BaudRate)`                 | See datasheet warning                 |
| `FactoryRestore()`                      | Magic write `0x5214` → register 80    |
| `DiscoverPresentAddresses(...)`         | Sweep 1..247                          |

### Raw Modbus primitives

For advanced use:

```cpp
DriverError ReadHoldingRegisters(uint16_t reg, uint16_t* out, uint8_t count);
DriverResult<void> WriteSingleRegister(uint16_t reg, uint16_t value);
DriverResult<void> WriteMultipleRegisters(uint16_t reg,
                                          const uint16_t* regs,
                                          uint8_t count);
```

## `UartInterface<Derived>` (alicat_basis2_uart_interface.hpp)

CRTP base that the host extends. Required methods:

```cpp
void write(const uint8_t* data, std::size_t length) noexcept;
std::size_t read(uint8_t* out, std::size_t max,
                 std::uint32_t timeout_ms) noexcept;
void flush_rx() noexcept;
```

Optional method:

```cpp
void delay_ms_impl(std::uint32_t ms) noexcept;   // SFINAE-detected
```

If `delay_ms_impl` is missing the driver's `delay_ms()` becomes a no-op,
which is fine for ESP-IDF where `uart_read_bytes` already enforces idle
gaps via the timeout argument.

## Result types (alicat_basis2_types.hpp)

```cpp
template <typename T>
struct DriverResult {
    T            value{};
    DriverError  error{DriverError::None};
    bool ok() const noexcept;
    explicit operator bool() const noexcept;
};

template <> struct DriverResult<void> { DriverError error; bool ok() const; };
```

Errors are stable enumerators (see `enum class DriverError`).

## Decoded data structures (alicat_basis2_types.hpp)

| Struct                | Purpose                                                      |
| --------------------- | ------------------------------------------------------------ |
| `InstantaneousData`   | Decoded snapshot of regs 2100..2109                          |
| `MeasurementData`     | Decoded measurement-window result (regs 4201..4214)          |
| `InstrumentIdentity`  | Firmware / serial / address / units / full-scale             |
| `InstrumentStatus`    | Live-status helper derived from `InstantaneousData` flags    |

## Constants and helpers

| Symbol                                | Use                                          |
| ------------------------------------- | -------------------------------------------- |
| `kBroadcastAddress` (0)               | Unaddressed broadcast writes                 |
| `kMaxModbusAddress` (247)             |                                              |
| `kMagicTare` (`0xAA55`)               | Value written to regs 39 / 53                |
| `kMagicFactoryRestore` (`0x5214`)     | Value written to reg 80                      |
| `kDefaultTimeoutMs` (200)             | Default per-transaction timeout              |
| `BaudRateToBps(BaudRate)`             | Convert enum → integer bits/s                |
| `GasShortName(Gas)`                   | Datasheet short name (`"N2"`, `"O2"`, …)     |

**Next:** [Examples →](examples.md)
