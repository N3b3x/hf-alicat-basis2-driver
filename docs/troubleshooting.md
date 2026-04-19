---
layout: default
title: "🛠️ Troubleshooting"
nav_order: 8
parent: "📚 Documentation"
permalink: /docs/troubleshooting/
---

# Troubleshooting

| Symptom | Likely cause | What to try |
| --- | --- | --- |
| Every call returns `DriverError::Timeout` | Bus / wiring / address mismatch | Run `DiscoverPresentAddresses()`. Check the RS-485 transceiver DE/RE pin and verify ESP-IDF is in `UART_MODE_RS485_HALF_DUPLEX`. Confirm the host UART baud matches the instrument (factory default 38400). |
| `DriverError::BadCrc` on every read | Termination missing or noisy bus | Add 120 Ω termination at both ends; keep T-stubs short; check ground reference. |
| `DriverError::ModbusException` from a meter | Wrote a controller-only register on a meter | Skip the call (or trap on the error code). |
| Setpoint writes ignored | Setpoint source set to `Analog` | `SetSetpointSource(SetpointSource::DigitalUnsaved)` first. |
| Setpoint changes lost across reboot | `DigitalUnsaved` is volatile by design | If you really need persistence, change source to `DigitalSaved`. ⚠️ Beware NVM wear (datasheet warning). |
| `SetBaudRate()` succeeds but next call times out | Forgot to reconfigure the host UART | After ack'ing the new baud, drain the response, reconfigure the host UART, then re-issue `ReadIdentity()` to confirm. |
| `Discover` returns 0 with multiple instruments wired | All slaves at the factory default address 1, talking on top of each other | Connect them one-by-one and `SetModbusAddress(N)` for each before adding the next. |
| Slow / sporadic timeouts under load | Inter-frame idle (3.5 char) violated | Implement `delay_ms_impl()` on your CRTP adapter so the driver can pause when needed. ESP-IDF's `uart_read_bytes` timeout already creates the gap, so this is almost never an issue on ESP32. |
| `flow` field looks 1000× too big or small | Decimals mismatch | The instrument's flow register is scaled by a factory-set decimal count. The driver defaults to 3 decimals; pass the right value to `ReadInstantaneous(decimals)` or to `SetSetpoint(value, decimals)` if your unit was calibrated differently. |

## Extra logging

When debugging on ESP-IDF, set the per-tag log level to **DEBUG**:

```cpp
esp_log_level_set("Basis2Min", ESP_LOG_DEBUG);
esp_log_level_set("Basis2Full", ESP_LOG_DEBUG);
```

…and instrument your CRTP adapter with `printf` / `ESP_LOGD` calls in
`write` and `read` to dump raw byte traces.

## Bus discovery and baud normalisation

`Driver::DiscoverPresentAddresses()` walks Modbus addresses 1..247 at the
host UART's **current baud only** — a device that was previously
configured to a different baud is invisible to it. To find every
instrument and bring them all to the same baud, layer the discovery in
two steps:

1. **Multi-baud discovery (host-driven).** For each baud rate the
   instrument can use (`4800/9600/19200/38400/57600/115200`):

   ```text
   for baud in {4800, 9600, 19200, 38400, 57600, 115200}:
       host_uart.set_baud(baud)
       sleep(25 ms)                        // settle / drain RX
       Driver::DiscoverPresentAddresses(...)
       record (addr, baud) for every responder
   ```

   This is exactly what `AlicatBasis2Handler::DiscoverAcrossBauds()` does
   in HardFOC's flux/vortex HALs — the host code only needs to provide a
   small `set_host_baud(uint32_t)` callback because the driver itself
   stays transport-agnostic.

2. **Bus normalisation.** For every `(addr, baud)` whose `baud != target`:

   - Retune the host UART to the device's current baud.
   - Address that slave and call `Driver::SetBaudRate(target)` — the
     device switches **instantly** after acking, so the response may be
     lost or garbled (datasheet calls this out).
   - Retune the host UART to `target`.
   - Verify with a quick `ReadIdentity()` at the new baud.

   `AlicatBasis2Handler::NormalizeBusBaud(target_bps, devices, set_host_baud)`
   wraps this loop and returns how many devices ended up verified at the
   target. Failed devices are reported in an optional bitmap.

### Two devices answering at the same address

Two BASIS-2 instruments wired to the same RS-485 segment with the same
Modbus address will collide — discovery will return either zero
responses or garbled CRC errors. **There is no software-only fix.** The
canonical workflow is:

1. Power up only one instrument.
2. `Driver::SetModbusAddress(N)` to give it a unique address.
3. Power-cycle / connect the next one.
4. Repeat.

Once every device has a unique address, multi-baud discovery + baud
normalisation can run unattended.

## Filing an issue

Please attach:

- Driver version (`HF_ALICAT_BASIS2_VERSION_STRING`).
- ESP-IDF version (or other RTOS + compiler).
- Output of `Driver::ReadIdentity()` (firmware version + serial).
- A raw byte trace of the failing transaction.

GitHub: <https://github.com/N3b3x/hf-alicat-basis2-driver/issues>
