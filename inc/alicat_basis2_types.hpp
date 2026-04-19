/**
 * @file alicat_basis2_types.hpp
 * @brief Public type definitions for the Alicat BASIS-2 driver.
 *
 * @details This file is the canonical, hardware-agnostic representation of
 *          everything the BASIS-2 instrument can publish or accept:
 *
 *            - Driver result domain (`DriverError`, `DriverResult`).
 *            - Calibrated gas table (datasheet "Active Gas").
 *            - Engineering-unit table (datasheet "Engineering Units").
 *            - Baud-rate ladder.
 *            - Setpoint-source taxonomy.
 *            - Decoded data frames (`InstantaneousData`, `MeasurementData`,
 *              `InstrumentStatus`).
 *
 *          Everything is expressed in plain types (no exceptions, no STL
 *          allocators) so the driver compiles cleanly on bare-metal MCUs.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace alicat_basis2 {

//==============================================================================
// DRIVER RESULT TYPES
//==============================================================================

/**
 * @brief Driver-level error codes.
 *
 * Every public Driver entry point returns one of these in its result type.
 * The values are stable and may be used directly in CAN/MQTT payloads.
 */
enum class DriverError : uint8_t {
    None = 0,            ///< Operation succeeded.
    NotInitialized,      ///< Driver not initialized yet.
    InvalidParameter,    ///< Out-of-range argument.
    InvalidAddress,      ///< Modbus address outside 1..247.
    BufferTooSmall,      ///< Provided buffer is too small for the response.
    SerialError,         ///< Serial transport-layer error (HW failure).
    Timeout,             ///< No response within the configured timeout.
    BadCrc,              ///< CRC-16 mismatch on received Modbus frame.
    InvalidFrame,        ///< Frame too short, wrong function code, etc.
    ModbusException,     ///< Slave returned a Modbus exception PDU.
    UnsupportedRequest,  ///< Slave does not implement this register/command.
    ResponseMismatch,    ///< Echo doesn't match request (function 6/16).
    AsciiBadResponse,    ///< ASCII reply not parseable.
};

/// Convert a DriverError to a stable human-readable string (no allocation).
constexpr std::string_view ToString(DriverError e) noexcept {
    switch (e) {
        case DriverError::None:               return "None";
        case DriverError::NotInitialized:     return "NotInitialized";
        case DriverError::InvalidParameter:   return "InvalidParameter";
        case DriverError::InvalidAddress:     return "InvalidAddress";
        case DriverError::BufferTooSmall:     return "BufferTooSmall";
        case DriverError::SerialError:        return "SerialError";
        case DriverError::Timeout:            return "Timeout";
        case DriverError::BadCrc:             return "BadCrc";
        case DriverError::InvalidFrame:       return "InvalidFrame";
        case DriverError::ModbusException:    return "ModbusException";
        case DriverError::UnsupportedRequest: return "UnsupportedRequest";
        case DriverError::ResponseMismatch:   return "ResponseMismatch";
        case DriverError::AsciiBadResponse:   return "AsciiBadResponse";
    }
    return "?";
}

/// Lightweight result wrapper. Aborts on `value()` if `!ok()`.
template <typename T>
struct DriverResult {
    T            value{};
    DriverError  error{DriverError::None};

    constexpr bool ok() const noexcept { return error == DriverError::None; }
    constexpr explicit operator bool() const noexcept { return ok(); }

    static constexpr DriverResult success(T v) noexcept { return {v, DriverError::None}; }
    static constexpr DriverResult failure(DriverError e) noexcept { return {T{}, e}; }
};

template <>
struct DriverResult<void> {
    DriverError  error{DriverError::None};
    constexpr bool ok() const noexcept { return error == DriverError::None; }
    constexpr explicit operator bool() const noexcept { return ok(); }
    static constexpr DriverResult success() noexcept { return {DriverError::None}; }
    static constexpr DriverResult failure(DriverError e) noexcept { return {e}; }
};

//==============================================================================
// CALIBRATED GASES (datasheet "Active Gas")
//==============================================================================

/**
 * @brief Index into the BASIS-2 calibrated gas table.
 *
 * The instrument is factory-calibrated for these 9 gases; the value is what
 * the device expects on register 2100 / `GS` ASCII command.
 */
enum class Gas : uint8_t {
    Air         = 0,
    Argon       = 1,
    CarbonDioxide = 2,
    Nitrogen    = 3,
    Oxygen      = 4,
    NitrousOxide = 5,
    Hydrogen    = 6,
    Helium      = 7,
    Methane     = 8,
};

constexpr std::string_view GasShortName(Gas g) noexcept {
    switch (g) {
        case Gas::Air:           return "Air";
        case Gas::Argon:         return "Ar";
        case Gas::CarbonDioxide: return "CO2";
        case Gas::Nitrogen:      return "N2";
        case Gas::Oxygen:        return "O2";
        case Gas::NitrousOxide:  return "N2O";
        case Gas::Hydrogen:      return "H2";
        case Gas::Helium:        return "He";
        case Gas::Methane:       return "CH4";
    }
    return "?";
}

//==============================================================================
// ENGINEERING UNITS (datasheet "Engineering Units")
//==============================================================================

/**
 * @brief Flow-unit identifier read from instrument register 49.
 *
 * The numeric value matches the datasheet table verbatim.
 */
enum class FlowUnits : uint8_t {
    SCCM = 0, NCCM = 1, SLPM = 2, NLPM = 3,
    SmL_s = 4, NmL_s = 5, SmL_m = 6, NmL_m = 7,
    SL_h = 8, NL_h = 9,
    SCCS = 10, NCCS = 11,
    Sm3_h = 12, Nm3_h = 13,
    Sm3_d = 14, Nm3_d = 15,
    SCIM = 16, SCFM = 17, SCFH = 18, SCFD = 19,
    Unknown = 0xFF,
};

constexpr std::string_view ToString(FlowUnits u) noexcept {
    switch (u) {
        case FlowUnits::SCCM:  return "SCCM";
        case FlowUnits::NCCM:  return "NCCM";
        case FlowUnits::SLPM:  return "SLPM";
        case FlowUnits::NLPM:  return "NLPM";
        case FlowUnits::SmL_s: return "SmL/s";
        case FlowUnits::NmL_s: return "NmL/s";
        case FlowUnits::SmL_m: return "SmL/m";
        case FlowUnits::NmL_m: return "NmL/m";
        case FlowUnits::SL_h:  return "SL/h";
        case FlowUnits::NL_h:  return "NL/h";
        case FlowUnits::SCCS:  return "SCCS";
        case FlowUnits::NCCS:  return "NCCS";
        case FlowUnits::Sm3_h: return "Sm3/h";
        case FlowUnits::Nm3_h: return "Nm3/h";
        case FlowUnits::Sm3_d: return "Sm3/d";
        case FlowUnits::Nm3_d: return "Nm3/d";
        case FlowUnits::SCIM:  return "SCIM";
        case FlowUnits::SCFM:  return "SCFM";
        case FlowUnits::SCFH:  return "SCFH";
        case FlowUnits::SCFD:  return "SCFD";
        case FlowUnits::Unknown: return "?";
    }
    return "?";
}

//==============================================================================
// SERIAL CONFIGURATION
//==============================================================================

/**
 * @brief Baud-rate codes for register 21 (datasheet "Changing the Baud Rate").
 *
 * The numeric value is what the instrument expects on the wire; the
 * `BaudRateToBps` helper converts it to a real bits-per-second number for
 * driver-side serial-port reconfiguration.
 */
enum class BaudRate : uint8_t {
    Bps_4800   = 0,
    Bps_9600   = 1,
    Bps_19200  = 2,
    Bps_38400  = 3,   ///< Factory default.
    Bps_57600  = 4,
    Bps_115200 = 5,
};

constexpr uint32_t BaudRateToBps(BaudRate b) noexcept {
    switch (b) {
        case BaudRate::Bps_4800:   return 4800;
        case BaudRate::Bps_9600:   return 9600;
        case BaudRate::Bps_19200:  return 19200;
        case BaudRate::Bps_38400:  return 38400;
        case BaudRate::Bps_57600:  return 57600;
        case BaudRate::Bps_115200: return 115200;
    }
    return 38400;
}

/**
 * @brief Setpoint source — register 516 (controllers only).
 *
 * `Analog`           — 0–5 V or 4–20 mA on pin 1 controls the setpoint;
 *                      digital `S` / register-2053 writes are ignored.
 * `DigitalSaved`     — most recent digital setpoint is persisted to flash
 *                      and used at next power-up. *Avoid frequent writes*
 *                      (the datasheet warns of NVM wear).
 * `DigitalUnsaved`   — digital setpoint is volatile (lost on power loss).
 *                      Recommended whenever the setpoint changes faster
 *                      than once per few minutes.
 */
enum class SetpointSource : uint8_t {
    Analog         = 0,
    DigitalSaved   = 1,
    DigitalUnsaved = 2,
};

/**
 * @brief Totalizer overflow behaviour — register 54.
 */
enum class TotalizerLimitMode : uint8_t {
    SaturateNoError  = 0,
    ResetNoError     = 1,
    SaturateWithOvr  = 2,
    ResetWithOvr     = 3,
};

/**
 * @brief Measurement-trigger bitmask — register 4200 (controllers only).
 *
 * Combine with bit-or; pass to `StartMeasurementSession()` /
 * `ConfigureMeasurementTrigger()`.
 */
namespace MeasurementTrigger {
constexpr uint16_t OnDigitalSetpointChange  = 1U << 0;
constexpr uint16_t OnHoldPercentChange      = 1U << 1;
constexpr uint16_t OnAverageFlowRead        = 1U << 2;
}  // namespace MeasurementTrigger

//==============================================================================
// DECODED DATA FRAMES
//==============================================================================

/**
 * @brief Decoded snapshot of registers 2100..2109 (one Modbus burst).
 *
 * Every numeric field is expressed in engineering units (V or mA already
 * scaled, flow scaled by the datasheet's `value / 1000` rule). The raw
 * 16-/32-bit register values are also kept so the user can validate or log.
 */
struct InstantaneousData {
    Gas       gas{Gas::Air};
    bool      mass_flow_overrange{false};      ///< MOV.
    bool      temperature_overrange{false};    ///< TOV.
    bool      totalizer_overrange{false};      ///< OVR.
    bool      valve_held{false};               ///< HLD.
    bool      valve_thermal_management{false}; ///< VTM.
    float     temperature_c{0.0f};             ///< Register 2102 / 100.
    float     flow{0.0f};                      ///< Register 2103 (× scale below).
    float     total_volume{0.0f};              ///< Registers 2104-5 (× scale below).
    float     setpoint{0.0f};                  ///< Register 2106 (× scale below).
    float     valve_drive_pct{0.0f};           ///< Register 2107 / 100.
    float     batch_remaining{0.0f};           ///< Registers 2108-9 (× scale below).

    int32_t   raw_setpoint_2053_2054{0};
    int16_t   raw_flow_2103{0};
    int16_t   raw_temp_2102{0};
    uint32_t  raw_total_volume_2104_2105{0};
    uint32_t  raw_batch_remaining_2108_2109{0};
};

/**
 * @brief Decoded measurement-window result (registers 4200..4214).
 */
struct MeasurementData {
    uint16_t  num_samples_collected{0};
    int16_t   min_temperature_raw{0};
    int16_t   max_temperature_raw{0};
    int16_t   min_flow_raw{0};
    int16_t   max_flow_raw{0};
    int16_t   avg_temperature_raw{0};
    int16_t   avg_flow_raw{0};
    int32_t   prev_avg_flow_raw{0};

    float min_temperature_c{0.0f};
    float max_temperature_c{0.0f};
    float avg_temperature_c{0.0f};
    float avg_flow{0.0f};
};

/**
 * @brief Static identification of an instrument (read once at init).
 */
struct InstrumentIdentity {
    uint16_t firmware_version_raw{0};   ///< Register 25 — pack(a,b,c) = 256a + 16b + c.
    uint8_t  fw_major{0}, fw_minor{0}, fw_patch{0};
    char     serial_number[13]{};       ///< Registers 26-31 (12 chars + NUL).
    uint8_t  modbus_address{0};         ///< Register 45.
    char     ascii_unit_id{'A'};        ///< Register 46.
    FlowUnits flow_units{FlowUnits::Unknown};
    uint32_t full_scale_flow_raw{0};    ///< Registers 47-48; flow_units / 1000.
};

/// Carries the global instrument health derived from `InstantaneousData`.
struct InstrumentStatus {
    bool any_error() const noexcept {
        return mass_flow_overrange || temperature_overrange ||
               totalizer_overrange  || valve_thermal_management;
    }
    bool mass_flow_overrange{false};
    bool temperature_overrange{false};
    bool totalizer_overrange{false};
    bool valve_held{false};
    bool valve_thermal_management{false};
};

//==============================================================================
// CONFIGURATION CONSTANTS
//==============================================================================

inline constexpr uint8_t  kBroadcastAddress     = 0;
inline constexpr uint8_t  kMaxModbusAddress     = 247;
inline constexpr uint16_t kMagicTare            = 0xAA55;  // register 39 / 53.
inline constexpr uint16_t kMagicFactoryRestore  = 0x5214;  // register 80.

inline constexpr uint16_t kDefaultTimeoutMs     = 200;
inline constexpr uint16_t kInterFrameDelayMs    = 5;

}  // namespace alicat_basis2
