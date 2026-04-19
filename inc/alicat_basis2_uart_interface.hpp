/**
 * @file alicat_basis2_uart_interface.hpp
 * @brief CRTP serial transport for the Alicat BASIS-2 driver.
 *
 * @details The driver itself only deals in byte buffers. Each platform
 *          (ESP-IDF, mock test harness, embedded MCU project, …) provides
 *          a concrete adapter inheriting from this CRTP base, implementing
 *          three small methods:
 *
 *            void write(const uint8_t* data, std::size_t length);
 *            std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms);
 *            void flush_rx();
 *
 *          Optionally:
 *
 *            void delay_ms(uint32_t ms);   // defaults to busy-wait via while().
 *            void log(const char* tag, const char* fmt, ...);   // optional
 *
 *          The Modbus-RTU spec mandates a ≥ 3.5-character idle gap between
 *          frames. Concrete adapters should map RS-485 DE/RE handling
 *          (when needed) onto the underlying UART driver's RS-485 mode
 *          before passing the adapter to the BASIS-2 driver.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace alicat_basis2 {

/**
 * @brief CRTP base for the BASIS-2 serial transport.
 *
 * @tparam Derived The concrete adapter (e.g. `AlicatUartEspAdapter`).
 *
 * The driver dispatches every byte through `static_cast<Derived*>(this)`,
 * so there is *zero* virtual-call overhead. The compiler can fully inline
 * the adapter calls into the driver's read/write loops.
 */
template <typename Derived>
class UartInterface {
public:
    /**
     * @brief Send `length` bytes synchronously. Must block until all bytes
     *        are accepted by the underlying serial driver.
     */
    void write(const uint8_t* data, std::size_t length) noexcept {
        static_cast<Derived*>(this)->write(data, length);
    }

    /**
     * @brief Read up to `max` bytes into `out` with a per-call timeout.
     * @return Number of bytes actually written into `out`.
     */
    std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept {
        return static_cast<Derived*>(this)->read(out, max, timeout_ms);
    }

    /// Drop any unread bytes from the RX path.
    void flush_rx() noexcept {
        static_cast<Derived*>(this)->flush_rx();
    }

    /**
     * @brief Sleep `ms` milliseconds. Default implementation is a no-op;
     *        adapters that have a sleep primitive should override.
     */
    void delay_ms(uint32_t ms) noexcept {
        if constexpr (HasDelay<Derived>::value) {
            static_cast<Derived*>(this)->delay_ms_impl(ms);
        } else {
            (void)ms;
        }
    }

    /**
     * @brief Optional structured logging hook. Default is no-op.
     */
    void log(const char* tag, const char* fmt, ...) noexcept {
        (void)tag; (void)fmt;
    }

private:
    template <typename, typename = void>
    struct HasDelay : std::false_type {};
    template <typename T>
    struct HasDelay<T, std::void_t<decltype(std::declval<T>().delay_ms_impl(0U))>>
        : std::true_type {};
};

}  // namespace alicat_basis2
