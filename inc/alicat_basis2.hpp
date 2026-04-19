/**
 * @file alicat_basis2.hpp
 * @brief Hardware-agnostic driver for Alicat BASIS-2 mass-flow meters and
 *        controllers (B-Series / BC-Series).
 *
 * @details
 *   - Bus : Modbus-RTU over an RS-232 / RS-485 multidrop UART link.
 *   - Slaves: up to 247 BASIS-2 instruments share a single bus; each is
 *             addressed by its 1..247 Modbus address (settable at runtime).
 *   - Transport: any byte-level UART exposed via the CRTP
 *               `alicat_basis2::UartInterface` adapter.
 *
 *   The driver intentionally focuses on the **production** subset of the
 *   datasheet — the operations a host typically needs in a real
 *   measurement loop:
 *
 *     - Single-burst read of every instantaneous data register so the live
 *       cross-check between digital and analog can use one round-trip.
 *     - Setpoint command (with setpoint-source switching).
 *     - Tare, autotare, totalizer reset, totalizer batch.
 *     - Gas selection.
 *     - Full instrument identity (firmware, serial, units, full-scale).
 *     - Communication watchdog.
 *     - Baud / Modbus-address / ASCII unit-id reconfiguration (with the
 *       safety dance the datasheet warns about for baud changes).
 *     - Factory restore.
 *     - Address discovery sweep so a fresh bench can be enumerated even
 *       when the operator doesn't know how the instruments were last
 *       programmed.
 *
 * @par Thread safety
 *   The driver is **not** internally synchronised. When several threads
 *   share the same bus the host must serialise transactions externally
 *   (a single mutex around every public call is sufficient). See
 *   `examples/esp32/` for one such pattern.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include "alicat_basis2_modbus.hpp"
#include "alicat_basis2_registers.hpp"
#include "alicat_basis2_types.hpp"
#include "alicat_basis2_uart_interface.hpp"

#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace alicat_basis2 {

/**
 * @brief Main BASIS-2 driver.
 *
 * @tparam UartT  Concrete CRTP adapter inheriting `UartInterface<UartT>`.
 *
 * The driver instance corresponds to *one Modbus address* on the shared
 * RS-485 bus. Multiple driver instances can share the same UART adapter
 * for different addresses; the host is responsible for serialising bus
 * access (the hf-core handler wraps this in an RtosMutex).
 */
template <typename UartT>
class Driver {
public:
    /**
     * @brief Construct a driver bound to one bus + one slave address.
     * @param uart      Reference to a CRTP adapter; must outlive Driver.
     * @param address   Modbus address (1..247) of this slave. Use
     *                  `kBroadcastAddress` (0) for unaddressed broadcast
     *                  writes (response-less).
     * @param timeout_ms Per-transaction response timeout (datasheet
     *                  default ~200 ms is safe).
     */
    Driver(UartT& uart, uint8_t address,
           uint16_t timeout_ms = kDefaultTimeoutMs) noexcept
        : uart_(uart), address_(address), timeout_ms_(timeout_ms) {}

    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;

    /// Update the per-transaction timeout (ms).
    void SetTimeoutMs(uint16_t ms) noexcept { timeout_ms_ = ms; }
    uint16_t GetTimeoutMs() const noexcept  { return timeout_ms_; }

    /// Switch the slave address this driver targets at runtime.
    void SetAddress(uint8_t addr) noexcept { address_ = addr; }
    uint8_t GetAddress() const noexcept    { return address_; }

    //==========================================================================
    // INSTANTANEOUS DATA — single Modbus burst
    //==========================================================================

    /**
     * @brief Read all instantaneous registers (2100..2109) in one burst.
     *
     * @details This is the canonical "live data" call — one round-trip on
     *          the bus, decoded into engineering units using the
     *          `flow_decimals` factor read once from the instrument.
     *
     * @param flow_decimals  The factory-set number of decimals reported on
     *                       this instrument's flow register; pulled at
     *                       init time via `ReadIdentity()`.
     */
    DriverResult<InstantaneousData> ReadInstantaneous(uint8_t flow_decimals = 3) noexcept {
        std::array<uint16_t, 10> regs{};
        const auto rc = ReadHoldingRegisters(reg::SelectedGas, regs.data(),
                                             static_cast<uint8_t>(regs.size()));
        if (rc != DriverError::None) return DriverResult<InstantaneousData>::failure(rc);

        InstantaneousData out{};
        out.gas = static_cast<Gas>(regs[0] & 0xFFU);

        const uint16_t status = regs[1];
        out.mass_flow_overrange      = (status & 0x0001U) != 0;
        out.temperature_overrange    = (status & 0x0002U) != 0;
        out.totalizer_overrange      = (status & 0x0004U) != 0;
        out.valve_held               = (status & 0x0008U) != 0;
        out.valve_thermal_management = (status & 0x0010U) != 0;

        out.raw_temp_2102 = static_cast<int16_t>(regs[2]);
        out.temperature_c = static_cast<float>(out.raw_temp_2102) * 0.01f;

        out.raw_flow_2103 = static_cast<int16_t>(regs[3]);
        out.flow = ScaleFlow(out.raw_flow_2103, flow_decimals);

        out.raw_total_volume_2104_2105 =
            (static_cast<uint32_t>(regs[4]) << 16) | regs[5];
        out.total_volume = ScaleFlow(static_cast<int32_t>(out.raw_total_volume_2104_2105),
                                     flow_decimals);

        out.raw_setpoint_2053_2054 = static_cast<int16_t>(regs[6]);
        out.setpoint = ScaleFlow(out.raw_setpoint_2053_2054, flow_decimals);

        out.valve_drive_pct = static_cast<float>(static_cast<int16_t>(regs[7])) * 0.01f;

        out.raw_batch_remaining_2108_2109 =
            (static_cast<uint32_t>(regs[8]) << 16) | regs[9];
        out.batch_remaining = ScaleFlow(static_cast<int32_t>(out.raw_batch_remaining_2108_2109),
                                        flow_decimals);

        return DriverResult<InstantaneousData>::success(out);
    }

    //==========================================================================
    // INSTRUMENT IDENTITY (slow registers; read once at init)
    //==========================================================================

    /**
     * @brief Pull the static identification of the instrument.
     *
     * Reads firmware version (reg 25), serial number (regs 26..31),
     * Modbus address (reg 45), ASCII unit id (reg 46), full-scale flow
     * in user units (regs 47..48) and the flow-units code (reg 49).
     */
    DriverResult<InstrumentIdentity> ReadIdentity() noexcept {
        InstrumentIdentity id{};

        // Firmware version
        std::array<uint16_t, 1> fw{};
        if (auto rc = ReadHoldingRegisters(reg::FirmwareVersion, fw.data(), 1);
            rc != DriverError::None) {
            return DriverResult<InstrumentIdentity>::failure(rc);
        }
        id.firmware_version_raw = fw[0];
        id.fw_major = static_cast<uint8_t>((fw[0] >> 8) & 0xFFU);
        id.fw_minor = static_cast<uint8_t>((fw[0] >> 4) & 0x0FU);
        id.fw_patch = static_cast<uint8_t>(fw[0] & 0x0FU);

        // Serial number
        std::array<uint16_t, reg::SerialNumberCount> sn{};
        if (auto rc = ReadHoldingRegisters(reg::SerialNumberStart, sn.data(),
                                           reg::SerialNumberCount);
            rc != DriverError::None) {
            return DriverResult<InstrumentIdentity>::failure(rc);
        }
        for (std::size_t i = 0; i < reg::SerialNumberCount; ++i) {
            id.serial_number[i * 2]     = static_cast<char>((sn[i] >> 8) & 0xFFU);
            id.serial_number[i * 2 + 1] = static_cast<char>(sn[i] & 0xFFU);
        }
        id.serial_number[12] = '\0';

        // Address + unit id + flow units
        std::array<uint16_t, 5> tail{};  // 45..49
        if (auto rc = ReadHoldingRegisters(reg::ModbusAddress, tail.data(),
                                           static_cast<uint8_t>(tail.size()));
            rc != DriverError::None) {
            return DriverResult<InstrumentIdentity>::failure(rc);
        }
        id.modbus_address = static_cast<uint8_t>(tail[0] & 0xFFU);
        id.ascii_unit_id  = static_cast<char>(tail[1] & 0xFFU);
        id.full_scale_flow_raw =
            (static_cast<uint32_t>(tail[2]) << 16) | tail[3];
        id.flow_units = static_cast<FlowUnits>(tail[4] & 0xFFU);
        return DriverResult<InstrumentIdentity>::success(id);
    }

    //==========================================================================
    // SETPOINT (controllers only)
    //==========================================================================

    /**
     * @brief Set the digital setpoint (controllers only).
     * @param value_in_user_units Real setpoint, e.g. 12.5 SLPM.
     * @param flow_decimals       Number of decimals for this instrument
     *                            (call `ReadIdentity` once and cache).
     */
    DriverResult<void> SetSetpoint(float value_in_user_units, uint8_t flow_decimals = 3) noexcept {
        const int32_t scaled = static_cast<int32_t>(
            value_in_user_units * Pow10(flow_decimals + 0));  // datasheet: × 1000 always
        // The datasheet specifies value/1000 for the scaled register regardless of decimals.
        const int32_t raw = static_cast<int32_t>(value_in_user_units * 1000.0f);
        std::array<uint16_t, 2> regs = {
            static_cast<uint16_t>((raw >> 16) & 0xFFFFU),
            static_cast<uint16_t>(raw & 0xFFFFU),
        };
        (void)scaled;
        return WriteMultipleRegisters(reg::SetpointCommand, regs.data(), 2);
    }

    DriverResult<void> SetSetpointSource(SetpointSource src) noexcept {
        return WriteSingleRegister(reg::SetpointSource, static_cast<uint16_t>(src));
    }

    /// 0..5000 ms (0 disables); see datasheet "Communication Watchdog".
    DriverResult<void> SetCommWatchdogMs(uint16_t ms) noexcept {
        if (ms > 5000) return DriverResult<void>::failure(DriverError::InvalidParameter);
        return WriteSingleRegister(reg::CommWatchdogMs, ms);
    }

    DriverResult<void> SetMaxSetpointRamp(uint32_t pct_per_ms_x_10e7) noexcept {
        std::array<uint16_t, 2> regs = {
            static_cast<uint16_t>((pct_per_ms_x_10e7 >> 16) & 0xFFFFU),
            static_cast<uint16_t>(pct_per_ms_x_10e7 & 0xFFFFU),
        };
        return WriteMultipleRegisters(reg::MaxSetpointRamp, regs.data(), 2);
    }

    //==========================================================================
    // TARING / AUTOTARE
    //==========================================================================

    DriverResult<void> Tare() noexcept {
        return WriteSingleRegister(reg::TareCommand, kMagicTare);
    }

    DriverResult<void> SetAutotareEnabled(bool enabled) noexcept {
        return WriteSingleRegister(reg::AutotareEnable, enabled ? 1U : 0U);
    }

    //==========================================================================
    // GAS SELECTION
    //==========================================================================

    DriverResult<void> SetGas(Gas g) noexcept {
        return WriteSingleRegister(reg::SelectedGas, static_cast<uint16_t>(g));
    }

    //==========================================================================
    // TOTALIZER
    //==========================================================================

    DriverResult<void> ResetTotalizer() noexcept {
        return WriteSingleRegister(reg::TotalizerResetCommand, kMagicTare);
    }

    DriverResult<void> SetTotalizerLimitMode(TotalizerLimitMode mode) noexcept {
        return WriteSingleRegister(reg::TotalizerLimitMode, static_cast<uint16_t>(mode));
    }

    /// Set the totalizer batch volume (controllers only) in instrument units.
    DriverResult<void> SetTotalizerBatch(uint32_t value_scaled) noexcept {
        std::array<uint16_t, 2> regs = {
            static_cast<uint16_t>((value_scaled >> 16) & 0xFFFFU),
            static_cast<uint16_t>(value_scaled & 0xFFFFU),
        };
        return WriteMultipleRegisters(reg::TotalizerBatchVolume, regs.data(), 2);
    }

    //==========================================================================
    // FLOW PROCESSING
    //==========================================================================

    DriverResult<void> SetFlowAveragingMs(uint16_t ms) noexcept {
        if (ms > 2500) return DriverResult<void>::failure(DriverError::InvalidParameter);
        return WriteSingleRegister(reg::FlowAveragingMs, ms);
    }

    DriverResult<void> SetReferenceTemperatureC(float c) noexcept {
        if (c < 0.0f || c > 30.0f) return DriverResult<void>::failure(DriverError::InvalidParameter);
        const uint16_t raw = static_cast<uint16_t>(c * 100.0f);
        return WriteSingleRegister(reg::ReferenceTemperature, raw);
    }

    //==========================================================================
    // MEASUREMENT WINDOW (datasheet "Configure Measurement Triggering")
    //==========================================================================

    DriverResult<void> ConfigureMeasurementTrigger(uint16_t trigger_bits) noexcept {
        return WriteSingleRegister(reg::MeasurementTriggerMode, trigger_bits);
    }

    DriverResult<void> StartMeasurementSamples(uint16_t num_samples) noexcept {
        return WriteSingleRegister(reg::MeasurementSampleCount, num_samples);
    }

    DriverResult<MeasurementData> ReadMeasurement() noexcept {
        std::array<uint16_t, 14> regs{};  // 4201..4214
        const auto rc = ReadHoldingRegisters(reg::MeasurementSampleCount, regs.data(),
                                             static_cast<uint8_t>(regs.size()));
        if (rc != DriverError::None) return DriverResult<MeasurementData>::failure(rc);

        MeasurementData out{};
        out.num_samples_collected = regs[0];                // 4201 actually holds sample count we wrote
        out.min_temperature_raw   = static_cast<int16_t>(regs[1]);
        out.max_temperature_raw   = static_cast<int16_t>(regs[2]);
        out.min_flow_raw          = static_cast<int16_t>(regs[3]);
        out.max_flow_raw          = static_cast<int16_t>(regs[4]);
        out.avg_temperature_raw   = static_cast<int16_t>(regs[6]);
        out.avg_flow_raw          = static_cast<int16_t>(regs[7]);
        out.prev_avg_flow_raw     =
            (static_cast<int32_t>(static_cast<int16_t>(regs[12])) << 16) | regs[13];
        out.min_temperature_c = static_cast<float>(out.min_temperature_raw) * 0.01f;
        out.max_temperature_c = static_cast<float>(out.max_temperature_raw) * 0.01f;
        out.avg_temperature_c = static_cast<float>(out.avg_temperature_raw) * 0.01f;
        return DriverResult<MeasurementData>::success(out);
    }

    //==========================================================================
    // BUS / IDENTITY RECONFIGURATION
    //==========================================================================

    /**
     * @brief Change the instrument's Modbus address.
     * @note  After the slave acknowledges, this driver instance switches
     *        its target address automatically.
     */
    DriverResult<void> SetModbusAddress(uint8_t new_address) noexcept {
        if (new_address < 1 || new_address > kMaxModbusAddress) {
            return DriverResult<void>::failure(DriverError::InvalidAddress);
        }
        const auto rc = WriteSingleRegister(reg::ModbusAddress, new_address);
        if (rc.ok()) address_ = new_address;
        return rc;
    }

    /**
     * @brief Change the instrument's ASCII unit-id ('A'..'Z').
     */
    DriverResult<void> SetAsciiUnitId(char id) noexcept {
        if (id < 'A' || id > 'Z') {
            return DriverResult<void>::failure(DriverError::InvalidParameter);
        }
        return WriteSingleRegister(reg::AsciiUnitId, static_cast<uint16_t>(id));
    }

    /**
     * @brief Change the serial baud rate (instrument register 21).
     *
     * @warning The instrument switches to the new baud immediately after
     *          ack'ing the write. The host must reconfigure its UART to
     *          match before the next transaction. Per the datasheet this
     *          is a fundamentally racy operation; the host should:
     *
     *            1. Issue this command,
     *            2. **Drain the response on the OLD baud**,
     *            3. Reconfigure its UART to the NEW baud,
     *            4. (Optional) Issue an identity read to confirm.
     */
    DriverResult<void> SetBaudRate(BaudRate br) noexcept {
        return WriteSingleRegister(reg::BaudRate, static_cast<uint16_t>(br));
    }

    DriverResult<void> FactoryRestore() noexcept {
        return WriteSingleRegister(reg::FactoryRestoreCommand, kMagicFactoryRestore);
    }

    //==========================================================================
    // BUS DISCOVERY
    //==========================================================================

    /**
     * @brief Probe the entire bus to find which addresses respond.
     *
     * @param[out] present  Bitmap; bit N → instrument at address N.
     * @param      probe_timeout_ms  Per-address timeout (defaults to short).
     * @param      first_addr / last_addr  Sweep window (default full 1..247).
     *
     * @details Issues a Function-03 read of register 25 (firmware version)
     *          to every address in `[first_addr, last_addr]`. Restores
     *          this driver's saved address on exit.
     */
    DriverResult<uint8_t> DiscoverPresentAddresses(uint8_t* present_bitmap,
                                                   std::size_t bitmap_bytes,
                                                   uint16_t probe_timeout_ms = 30,
                                                   uint8_t  first_addr = 1,
                                                   uint8_t  last_addr  = kMaxModbusAddress) noexcept {
        if (!present_bitmap || bitmap_bytes < 32) {
            return DriverResult<uint8_t>::failure(DriverError::BufferTooSmall);
        }
        std::memset(present_bitmap, 0, bitmap_bytes);

        const uint8_t saved_addr     = address_;
        const uint16_t saved_timeout = timeout_ms_;
        timeout_ms_ = probe_timeout_ms;

        uint8_t found = 0;
        for (uint16_t a = first_addr; a <= last_addr; ++a) {
            address_ = static_cast<uint8_t>(a);
            std::array<uint16_t, 1> dummy{};
            if (ReadHoldingRegisters(reg::FirmwareVersion, dummy.data(), 1) ==
                DriverError::None) {
                present_bitmap[a / 8] |= static_cast<uint8_t>(1U << (a % 8));
                ++found;
            }
        }

        address_    = saved_addr;
        timeout_ms_ = saved_timeout;
        return DriverResult<uint8_t>::success(found);
    }

    //==========================================================================
    // RAW MODBUS PRIMITIVES
    //==========================================================================

    DriverError ReadHoldingRegisters(uint16_t reg_addr, uint16_t* out, uint8_t count) noexcept {
        if (!out || count == 0 || count > 125) return DriverError::InvalidParameter;
        std::array<uint8_t, 8> tx{};
        const std::size_t tx_len = modbus::BuildReadHolding(address_, reg_addr, count,
                                                            tx.data(), tx.size());
        if (tx_len == 0) return DriverError::BufferTooSmall;

        uart_.flush_rx();
        uart_.write(tx.data(), tx_len);

        // Expected response: addr(1) + fn(1) + bytecount(1) + payload(2*count) + crc(2)
        const std::size_t need = 5 + 2U * count;
        std::array<uint8_t, 5 + 2 * 32> rx{};  // up to 32 regs per call
        if (need > rx.size()) return DriverError::BufferTooSmall;
        const std::size_t got = uart_.read(rx.data(), need, timeout_ms_);
        if (got == 0) return DriverError::Timeout;
        if (got < 5)  return DriverError::InvalidFrame;
        return modbus::ParseReadHolding(rx.data(), got, address_, count, out);
    }

    DriverResult<void> WriteSingleRegister(uint16_t reg_addr, uint16_t value) noexcept {
        std::array<uint8_t, 8> tx{};
        const std::size_t tx_len = modbus::BuildWriteSingle(address_, reg_addr, value,
                                                            tx.data(), tx.size());
        if (tx_len == 0) return DriverResult<void>::failure(DriverError::BufferTooSmall);

        uart_.flush_rx();
        uart_.write(tx.data(), tx_len);

        // Broadcast → no response.
        if (address_ == kBroadcastAddress) return DriverResult<void>::success();

        std::array<uint8_t, 8> rx{};
        const std::size_t got = uart_.read(rx.data(), 8, timeout_ms_);
        if (got == 0) return DriverResult<void>::failure(DriverError::Timeout);
        return DriverResult<void>{ modbus::ParseWriteSingleEcho(rx.data(), got,
                                                                tx.data(), tx_len) };
    }

    DriverResult<void> WriteMultipleRegisters(uint16_t reg_addr,
                                              const uint16_t* regs, uint8_t count) noexcept {
        if (!regs || count == 0 || count > 32) {
            return DriverResult<void>::failure(DriverError::InvalidParameter);
        }
        std::array<uint8_t, 9 + 32 * 2> tx{};
        const std::size_t tx_len = modbus::BuildWriteMultiple(address_, reg_addr, regs, count,
                                                              tx.data(), tx.size());
        if (tx_len == 0) return DriverResult<void>::failure(DriverError::BufferTooSmall);

        uart_.flush_rx();
        uart_.write(tx.data(), tx_len);

        if (address_ == kBroadcastAddress) return DriverResult<void>::success();

        std::array<uint8_t, 8> rx{};
        const std::size_t got = uart_.read(rx.data(), 8, timeout_ms_);
        if (got == 0) return DriverResult<void>::failure(DriverError::Timeout);
        return DriverResult<void>{ modbus::ParseWriteMultipleEcho(rx.data(), got,
                                                                  address_, reg_addr, count) };
    }

private:
    static float Pow10(int n) noexcept {
        float v = 1.0f;
        for (int i = 0; i < n; ++i) v *= 10.0f;
        return v;
    }

    /// Datasheet: flow registers carry the value scaled by the factory-set
    /// number of decimals. Caller passes that count.
    static float ScaleFlow(int32_t raw, uint8_t decimals) noexcept {
        return static_cast<float>(raw) / Pow10(decimals);
    }

    UartT&    uart_;
    uint8_t   address_;
    uint16_t  timeout_ms_;
};

}  // namespace alicat_basis2
