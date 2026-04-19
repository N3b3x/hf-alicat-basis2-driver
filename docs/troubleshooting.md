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

## Filing an issue

Please attach:

- Driver version (`HF_ALICAT_BASIS2_VERSION_STRING`).
- ESP-IDF version (or other RTOS + compiler).
- Output of `Driver::ReadIdentity()` (firmware version + serial).
- A raw byte trace of the failing transaction.

GitHub: <https://github.com/N3b3x/hf-alicat-basis2-driver/issues>
