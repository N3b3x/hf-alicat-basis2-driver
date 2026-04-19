/**
 * @file alicat_basis2_registers.hpp
 * @brief Modbus-RTU holding-register addresses for Alicat BASIS-2.
 *
 * @details Source of truth for every register address used by the driver.
 *          The names mirror the datasheet (DOC-MANUAL-BASIS2 Rev 4) so
 *          cross-referencing stays trivial.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cstdint>

namespace alicat_basis2 {
namespace reg {

//==============================================================================
// CONFIGURATION & FUNCTION REGISTERS
//==============================================================================

inline constexpr uint16_t BaudRate                   = 21;
inline constexpr uint16_t FirmwareVersion            = 25;
inline constexpr uint16_t SerialNumberStart          = 26;   // 6 regs (12 chars)
inline constexpr uint16_t SerialNumberCount          = 6;
inline constexpr uint16_t FlowSensorOffset           = 32;
inline constexpr uint16_t FullScaleFlowSccm          = 35;   // 2 regs
inline constexpr uint16_t TareCommand                = 39;   // write 0xAA55
inline constexpr uint16_t ModbusAddress              = 45;
inline constexpr uint16_t AsciiUnitId                = 46;
inline constexpr uint16_t FullScaleFlowUserUnits     = 47;   // 2 regs (value / 1000)
inline constexpr uint16_t FlowUnitsCode              = 49;
inline constexpr uint16_t TareSampleCount            = 51;
inline constexpr uint16_t ReferenceTemperature       = 52;   // value / 100 = °C
inline constexpr uint16_t TotalizerResetCommand      = 53;   // write 0xAA55
inline constexpr uint16_t TotalizerLimitMode         = 54;
inline constexpr uint16_t FlowAveragingMs            = 55;
inline constexpr uint16_t CommandProtocol            = 56;
inline constexpr uint16_t FactoryRestoreCommand      = 80;   // write 0x5214
inline constexpr uint16_t GasInfoTableStart          = 81;   // 5 regs per gas
inline constexpr uint8_t  GasInfoEntries             = 12;

//==============================================================================
// CONTROL REGISTERS
//==============================================================================

inline constexpr uint16_t CommWatchdogMs             = 514;
inline constexpr uint16_t AutotareEnable             = 515;
inline constexpr uint16_t SetpointSource             = 516;
inline constexpr uint16_t PgainPdf                   = 519;
inline constexpr uint16_t IgainPdf                   = 520;
inline constexpr uint16_t TotalizerBatchVolume       = 521;  // 2 regs
inline constexpr uint16_t MaxSetpointRamp            = 524;  // 2 regs (per-ms × 10^7)

//==============================================================================
// INSTANTANEOUS DATA REGISTERS
//==============================================================================

inline constexpr uint16_t SetpointCommand            = 2053; // 2 regs (signed × 1000)
inline constexpr uint16_t SelectedGas                = 2100;
inline constexpr uint16_t StatusBitfield             = 2101;
inline constexpr uint16_t TemperatureCenticC         = 2102; // signed / 100
inline constexpr uint16_t Flow                       = 2103; // signed (× 10^-decimals)
inline constexpr uint16_t TotalVolumeStart           = 2104; // 2 regs
inline constexpr uint16_t SetpointReadback           = 2106;
inline constexpr uint16_t ValveDriveCenti            = 2107; // / 100 = %
inline constexpr uint16_t BatchRemainingStart        = 2108; // 2 regs

//==============================================================================
// MEASUREMENT REGISTERS
//==============================================================================

inline constexpr uint16_t MeasurementTriggerMode     = 4200;
inline constexpr uint16_t MeasurementSampleCount     = 4201;
inline constexpr uint16_t MeasurementMinTemp         = 4202;
inline constexpr uint16_t MeasurementMaxTemp         = 4203;
inline constexpr uint16_t MeasurementMinFlow         = 4204;
inline constexpr uint16_t MeasurementMaxFlow         = 4205;
inline constexpr uint16_t MeasurementSamplesAveraged = 4206;
inline constexpr uint16_t MeasurementAvgTemp         = 4207;
inline constexpr uint16_t MeasurementAvgFlow         = 4208;
inline constexpr uint16_t MeasurementValveDriveCurr  = 4209;
inline constexpr uint16_t MeasurementPrevMinFlow     = 4210;
inline constexpr uint16_t MeasurementPrevMaxFlow     = 4211;
inline constexpr uint16_t MeasurementPrevSamplesAvg  = 4212;
inline constexpr uint16_t MeasurementPrevAvgTemp     = 4213;
inline constexpr uint16_t MeasurementPrevAvgFlowLow  = 4214; // 2 regs (32-bit)

//==============================================================================
// MODBUS FUNCTION CODES (we only use 3, 6, 16)
//==============================================================================

namespace fn {
inline constexpr uint8_t ReadHolding         = 0x03;
inline constexpr uint8_t WriteSingle         = 0x06;
inline constexpr uint8_t WriteMultiple       = 0x10;
inline constexpr uint8_t ExceptionMask       = 0x80;
}  // namespace fn

}  // namespace reg
}  // namespace alicat_basis2
