/**
 * @file alicat_basis2_modbus.hpp
 * @brief Modbus-RTU framing and CRC-16 helpers used by the BASIS-2 driver.
 *
 * @details Implements only the three function codes BASIS-2 supports:
 *            03 — Read Holding Registers
 *            06 — Write Single Register
 *            16 — Write Multiple Registers
 *          plus the canonical Modbus CRC-16 (polynomial 0xA001, init 0xFFFF,
 *          LSB first) defined by the Modbus-RTU specification.
 *
 *          All routines are pure / constexpr-friendly, allocation-free, and
 *          operate on plain `uint8_t*` byte buffers so the implementation
 *          can be reused on any MCU without the STL.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

#include "alicat_basis2_registers.hpp"
#include "alicat_basis2_types.hpp"

namespace alicat_basis2 {
namespace modbus {

/**
 * @brief Compute the Modbus-RTU CRC-16 over a byte buffer.
 * @param data     Pointer to the bytes to checksum.
 * @param length   Number of bytes.
 * @return Little-endian CRC-16 (low byte first when transmitted).
 */
inline uint16_t Crc16(const uint8_t* data, std::size_t length) noexcept {
    uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x0001U) {
                crc = static_cast<uint16_t>((crc >> 1) ^ 0xA001U);
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/// Append the CRC-16 to `frame[length..length+1]` and return new length.
inline std::size_t AppendCrc(uint8_t* frame, std::size_t length) noexcept {
    const uint16_t crc = Crc16(frame, length);
    frame[length]     = static_cast<uint8_t>(crc & 0x00FFU);
    frame[length + 1] = static_cast<uint8_t>((crc >> 8) & 0x00FFU);
    return length + 2;
}

/// Verify the CRC at `frame[length-2..length-1]` matches the body.
inline bool CheckCrc(const uint8_t* frame, std::size_t length) noexcept {
    if (length < 4) return false;
    const uint16_t want = Crc16(frame, length - 2);
    const uint16_t got  = static_cast<uint16_t>(frame[length - 2]) |
                          (static_cast<uint16_t>(frame[length - 1]) << 8);
    return want == got;
}

//==============================================================================
// REQUEST BUILDERS — format the bytes a master sends on the wire.
//==============================================================================

/// Build a Function-03 (Read Holding Registers) request.
/// Returns the total frame length (header + payload + CRC).
inline std::size_t BuildReadHolding(uint8_t  unit_addr,
                                    uint16_t reg_addr,
                                    uint16_t reg_count,
                                    uint8_t* out, std::size_t out_capacity) noexcept {
    if (out_capacity < 8) return 0;
    out[0] = unit_addr;
    out[1] = reg::fn::ReadHolding;
    out[2] = static_cast<uint8_t>((reg_addr >> 8) & 0xFFU);
    out[3] = static_cast<uint8_t>(reg_addr        & 0xFFU);
    out[4] = static_cast<uint8_t>((reg_count >> 8) & 0xFFU);
    out[5] = static_cast<uint8_t>(reg_count        & 0xFFU);
    return AppendCrc(out, 6);
}

/// Build a Function-06 (Write Single Register) request.
inline std::size_t BuildWriteSingle(uint8_t  unit_addr,
                                    uint16_t reg_addr,
                                    uint16_t value,
                                    uint8_t* out, std::size_t out_capacity) noexcept {
    if (out_capacity < 8) return 0;
    out[0] = unit_addr;
    out[1] = reg::fn::WriteSingle;
    out[2] = static_cast<uint8_t>((reg_addr >> 8) & 0xFFU);
    out[3] = static_cast<uint8_t>(reg_addr        & 0xFFU);
    out[4] = static_cast<uint8_t>((value >> 8) & 0xFFU);
    out[5] = static_cast<uint8_t>(value        & 0xFFU);
    return AppendCrc(out, 6);
}

/// Build a Function-16 (Write Multiple Registers) request.
/// `regs` carries `count` 16-bit values in big-endian-on-the-wire order.
inline std::size_t BuildWriteMultiple(uint8_t  unit_addr,
                                      uint16_t reg_addr,
                                      const uint16_t* regs, uint8_t count,
                                      uint8_t* out, std::size_t out_capacity) noexcept {
    const std::size_t needed = 9 + count * 2;
    if (count == 0 || out_capacity < needed) return 0;
    out[0] = unit_addr;
    out[1] = reg::fn::WriteMultiple;
    out[2] = static_cast<uint8_t>((reg_addr >> 8) & 0xFFU);
    out[3] = static_cast<uint8_t>(reg_addr        & 0xFFU);
    out[4] = static_cast<uint8_t>((count >> 8) & 0xFFU);
    out[5] = static_cast<uint8_t>(count        & 0xFFU);
    out[6] = static_cast<uint8_t>(count * 2);
    for (uint8_t i = 0; i < count; ++i) {
        out[7 + i * 2]     = static_cast<uint8_t>((regs[i] >> 8) & 0xFFU);
        out[7 + i * 2 + 1] = static_cast<uint8_t>(regs[i]        & 0xFFU);
    }
    return AppendCrc(out, 7 + count * 2);
}

//==============================================================================
// RESPONSE PARSERS — return the number of registers extracted, or 0 on error.
//==============================================================================

/// Parse a Function-03 response into `regs[]`. `expected_count` is the number
/// of 16-bit registers the master asked for.
inline DriverError ParseReadHolding(const uint8_t* frame, std::size_t length,
                                    uint8_t expected_addr, uint8_t expected_count,
                                    uint16_t* regs) noexcept {
    if (length < 5)                             return DriverError::InvalidFrame;
    if (!CheckCrc(frame, length))               return DriverError::BadCrc;
    if (frame[0] != expected_addr)              return DriverError::ResponseMismatch;
    if (frame[1] & reg::fn::ExceptionMask)      return DriverError::ModbusException;
    if (frame[1] != reg::fn::ReadHolding)       return DriverError::ResponseMismatch;
    const std::size_t byte_count = frame[2];
    if (byte_count != expected_count * 2U)      return DriverError::InvalidFrame;
    if (length < 3 + byte_count + 2)            return DriverError::InvalidFrame;
    for (uint8_t i = 0; i < expected_count; ++i) {
        regs[i] = (static_cast<uint16_t>(frame[3 + i * 2]) << 8) |
                  static_cast<uint16_t>(frame[3 + i * 2 + 1]);
    }
    return DriverError::None;
}

/// Validate the echo of a Function-06 (Write Single) request against the
/// just-sent request. The slave must echo the full request.
inline DriverError ParseWriteSingleEcho(const uint8_t* frame, std::size_t length,
                                        const uint8_t* request, std::size_t req_len) noexcept {
    if (length < req_len)                       return DriverError::InvalidFrame;
    if (!CheckCrc(frame, length))               return DriverError::BadCrc;
    if (frame[0] != request[0])                 return DriverError::ResponseMismatch;
    if (frame[1] & reg::fn::ExceptionMask)      return DriverError::ModbusException;
    for (std::size_t i = 0; i < req_len; ++i) {
        if (frame[i] != request[i])             return DriverError::ResponseMismatch;
    }
    return DriverError::None;
}

/// Validate the response of a Function-16 (Write Multiple) request.
inline DriverError ParseWriteMultipleEcho(const uint8_t* frame, std::size_t length,
                                          uint8_t expected_addr,
                                          uint16_t expected_reg_addr,
                                          uint16_t expected_count) noexcept {
    if (length < 8)                             return DriverError::InvalidFrame;
    if (!CheckCrc(frame, length))               return DriverError::BadCrc;
    if (frame[0] != expected_addr)              return DriverError::ResponseMismatch;
    if (frame[1] & reg::fn::ExceptionMask)      return DriverError::ModbusException;
    if (frame[1] != reg::fn::WriteMultiple)     return DriverError::ResponseMismatch;
    const uint16_t got_addr = (static_cast<uint16_t>(frame[2]) << 8) | frame[3];
    const uint16_t got_cnt  = (static_cast<uint16_t>(frame[4]) << 8) | frame[5];
    if (got_addr != expected_reg_addr)          return DriverError::ResponseMismatch;
    if (got_cnt  != expected_count)             return DriverError::ResponseMismatch;
    return DriverError::None;
}

}  // namespace modbus
}  // namespace alicat_basis2
