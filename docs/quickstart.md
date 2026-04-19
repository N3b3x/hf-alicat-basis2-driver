---
layout: default
title: "🚀 Quick start"
nav_order: 2
parent: "📚 Documentation"
permalink: /docs/quickstart/
---

# Quick start

The whole driver is parameterised on a CRTP adapter that owns the UART
transport. Implementing the adapter takes ~20 lines on any platform.

## 1. Implement the CRTP adapter

```cpp
#include "alicat_basis2_uart_interface.hpp"

class MyUart : public alicat_basis2::UartInterface<MyUart> {
public:
    void write(const std::uint8_t* data, std::size_t len) noexcept;
    std::size_t read(std::uint8_t* out, std::size_t max,
                     std::uint32_t timeout_ms) noexcept;
    void flush_rx() noexcept;

    // Optional. The driver calls delay_ms() internally when it needs
    // the Modbus-RTU 3.5-character idle gap. If your platform has a
    // sleep primitive, expose it via delay_ms_impl(); otherwise skip it
    // and the call becomes a no-op.
    void delay_ms_impl(std::uint32_t ms) noexcept;
};
```

`write` must block until every byte is accepted by the underlying serial
driver. `read` should pull up to `max` bytes within the timeout and return
the number actually written into `out` (returning 0 on timeout is
expected and not an error condition).

## 2. Construct the driver

```cpp
#include "alicat_basis2.hpp"

MyUart uart;
alicat_basis2::Driver<MyUart> meter(uart, /*modbus_address=*/1);
```

## 3. Read the identity once at boot

```cpp
auto id = meter.ReadIdentity();
if (id.ok()) {
    printf("FW %u.%u.%u  serial=%s  units=%s  full-scale=%lu\n",
           id.value.fw_major, id.value.fw_minor, id.value.fw_patch,
           id.value.serial_number,
           alicat_basis2::ToString(id.value.flow_units).data(),
           static_cast<unsigned long>(id.value.full_scale_flow_raw));
}
```

## 4. Periodically pull live data

```cpp
auto live = meter.ReadInstantaneous();
if (live.ok()) {
    printf("%s  T=%.2f C  flow=%.4f  total=%.4f  drive=%.1f%%\n",
           alicat_basis2::GasShortName(live.value.gas).data(),
           static_cast<double>(live.value.temperature_c),
           static_cast<double>(live.value.flow),
           static_cast<double>(live.value.total_volume),
           static_cast<double>(live.value.valve_drive_pct));
}
```

`ReadInstantaneous()` issues a single Modbus burst over registers
**2100..2109**, decoding gas, status bits, temperature, flow,
totalizer, setpoint, valve drive and batch remaining in one round-trip.

## 5. Common control operations

```cpp
meter.Tare();
meter.SetGas(alicat_basis2::Gas::Nitrogen);
meter.SetReferenceTemperatureC(25.0f);
meter.SetCommWatchdogMs(2000);
meter.SetSetpointSource(alicat_basis2::SetpointSource::DigitalUnsaved);
meter.SetSetpoint(2.5f);   // controllers only
```

## 6. Bus discovery (first power-up)

When you don't know which addresses are programmed on the bus, use
`DiscoverPresentAddresses()`:

```cpp
std::array<std::uint8_t, 32> bitmap{};
auto rc = meter.DiscoverPresentAddresses(bitmap.data(), bitmap.size());
if (rc.ok()) {
    for (uint16_t a = 1; a <= 247; ++a) {
        if (bitmap[a / 8] & (1u << (a % 8))) {
            printf("addr=%u present\n", a);
        }
    }
}
```

This sweeps the bus reading register 25 (firmware version) at every
address and records who replies.

**Next:** [Modbus protocol →](modbus_protocol.md)
