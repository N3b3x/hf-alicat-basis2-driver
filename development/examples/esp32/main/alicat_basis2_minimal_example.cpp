// =============================================================================
// Alicat BASIS-2 — minimal ESP-IDF example
// =============================================================================
// Demonstrates the smallest amount of code needed to talk to one BASIS-2
// instrument from an ESP32-S3:
//   - Configure UART1 in RS-485 half-duplex.
//   - Bridge it to the Driver via a tiny CRTP adapter.
//   - Read identity once, then loop reading instantaneous data.
// =============================================================================

#include "alicat_basis2.hpp"
#include "alicat_basis2_uart_interface.hpp"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace al = alicat_basis2;

namespace {

constexpr const char* TAG          = "Basis2Min";
constexpr uart_port_t kUartPort    = UART_NUM_1;
constexpr int         kTxGpio      = 17;
constexpr int         kRxGpio      = 18;
constexpr int         kRtsDeRe     = 8;       // -1 if your transceiver doesn't need DE/RE
constexpr int         kRxBufBytes  = 1024;
constexpr int         kTxBufBytes  = 0;       // synchronous writes, no TX buffer needed
constexpr uint32_t    kBaud        = 38400;   // BASIS-2 factory default
constexpr std::uint8_t kModbusAddr = 1;

class EspUartAdapter : public al::UartInterface<EspUartAdapter> {
public:
    void write(const std::uint8_t* data, std::size_t len) noexcept {
        if (len > 0) uart_write_bytes(kUartPort, data, len);
    }
    std::size_t read(std::uint8_t* out, std::size_t max, std::uint32_t timeout_ms) noexcept {
        const int n = uart_read_bytes(kUartPort, out, max, pdMS_TO_TICKS(timeout_ms));
        return (n < 0) ? 0 : static_cast<std::size_t>(n);
    }
    void flush_rx() noexcept       { uart_flush_input(kUartPort); }
    void delay_ms_impl(std::uint32_t ms) noexcept { vTaskDelay(pdMS_TO_TICKS(ms)); }
};

}  // namespace

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Alicat BASIS-2 minimal example (ESP32-S3 UART%d, RS-485)", kUartPort);

    // 1) Bring up the UART.
    const uart_config_t uart_cfg = {
        .baud_rate = static_cast<int>(kBaud),
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {0},
    };
    ESP_ERROR_CHECK(uart_driver_install(kUartPort, kRxBufBytes, kTxBufBytes, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(kUartPort, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(kUartPort, kTxGpio, kRxGpio, kRtsDeRe, UART_PIN_NO_CHANGE));
    if (kRtsDeRe >= 0) {
        ESP_ERROR_CHECK(uart_set_mode(kUartPort, UART_MODE_RS485_HALF_DUPLEX));
        ESP_LOGI(TAG, "UART%d in RS-485 half-duplex (DE/RE auto-toggle on GPIO%d)",
                 kUartPort, kRtsDeRe);
    }

    // 2) Build the driver.
    EspUartAdapter adapter;
    al::Driver<EspUartAdapter> meter(adapter, kModbusAddr);

    // 3) Identity.
    auto id = meter.ReadIdentity();
    if (!id.ok()) {
        ESP_LOGE(TAG, "ReadIdentity failed: %s — is the BASIS-2 powered & wired?",
                 al::ToString(id.error).data());
    } else {
        ESP_LOGI(TAG, "FW %u.%u.%u  SN %s  units=%s  full-scale=%lu (units/1000)",
                 id.value.fw_major, id.value.fw_minor, id.value.fw_patch,
                 id.value.serial_number,
                 al::ToString(id.value.flow_units).data(),
                 static_cast<unsigned long>(id.value.full_scale_flow_raw));
    }

    // 4) Periodic live-data loop.
    while (true) {
        const auto live = meter.ReadInstantaneous();
        if (live.ok()) {
            const auto& d = live.value;
            ESP_LOGI(TAG, "%s | T=%.2f C | flow=%.4f | total=%.4f | drive=%.1f%%",
                     al::GasShortName(d.gas).data(),
                     static_cast<double>(d.temperature_c),
                     static_cast<double>(d.flow),
                     static_cast<double>(d.total_volume),
                     static_cast<double>(d.valve_drive_pct));
        } else {
            ESP_LOGW(TAG, "read failed: %s", al::ToString(live.error).data());
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
