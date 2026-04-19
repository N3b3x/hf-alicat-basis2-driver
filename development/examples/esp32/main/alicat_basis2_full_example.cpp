// =============================================================================
// Alicat BASIS-2 — comprehensive ESP-IDF example
// =============================================================================
// Walks through the entire datasheet feature surface that the driver exposes:
//
//   1. Configure UART1 for RS-485 half-duplex on TX/RX/DE-RE.
//   2. Discover present addresses on the bus (1..247) — useful at first
//      bring-up when an operator may not know how each instrument was
//      previously programmed.
//   3. Read identity (firmware, serial, address, ASCII unit id, units,
//      full-scale).
//   4. Configuration walkthrough — SetGas, SetReferenceTemperatureC,
//      SetFlowAveragingMs, SetTotalizerLimitMode, SetCommWatchdogMs.
//   5. Setpoint demo (controllers only) — SetSetpointSource +
//      SetSetpoint; gracefully handles "ModbusException" returned by
//      meter-only instruments.
//   6. Tare + ResetTotalizer.
//   7. Steady-state 1 Hz instantaneous read loop printing flow,
//      temperature, totalizer, valve drive, status bits.
//   8. Optional measurement-window demo — ConfigureMeasurementTrigger,
//      StartMeasurementSamples, ReadMeasurement.
// =============================================================================

#include "alicat_basis2.hpp"
#include "alicat_basis2_uart_interface.hpp"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <array>
#include <cstdio>

namespace al = alicat_basis2;

namespace {

constexpr const char* TAG          = "Basis2Full";
constexpr uart_port_t kUartPort    = UART_NUM_1;
constexpr int         kTxGpio      = 17;
constexpr int         kRxGpio      = 18;
constexpr int         kRtsDeRe     = 8;
constexpr int         kRxBufBytes  = 1024;
constexpr uint32_t    kBaud        = 38400;
constexpr uint32_t    kReadIntervalMs = 1000;
constexpr std::uint8_t kPrimaryAddr = 1;     // factory default; change after first SetModbusAddress

// CRTP adapter: ESP-IDF UART → driver byte-level transport.
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

void log_rc(const char* what, al::DriverError e) {
    if (e == al::DriverError::None) {
        ESP_LOGI(TAG, "    %s: ok", what);
    } else {
        ESP_LOGW(TAG, "    %s: %s", what, al::ToString(e).data());
    }
}

}  // namespace

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Alicat BASIS-2 comprehensive example (ESP32-S3 UART%d, RS-485)", kUartPort);

    // 1) UART bring-up.
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
    ESP_ERROR_CHECK(uart_driver_install(kUartPort, kRxBufBytes, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(kUartPort, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(kUartPort, kTxGpio, kRxGpio, kRtsDeRe, UART_PIN_NO_CHANGE));
    if (kRtsDeRe >= 0) {
        ESP_ERROR_CHECK(uart_set_mode(kUartPort, UART_MODE_RS485_HALF_DUPLEX));
        ESP_LOGI(TAG, "UART%d in RS-485 half-duplex (DE/RE auto-toggle on GPIO%d)",
                 kUartPort, kRtsDeRe);
    }

    EspUartAdapter adapter;
    al::Driver<EspUartAdapter> meter(adapter, kPrimaryAddr);

    // 2) Bus discovery.
    std::array<std::uint8_t, 32> present{};
    auto disc = meter.DiscoverPresentAddresses(present.data(), present.size(), /*timeout_ms=*/40);
    if (!disc.ok()) {
        ESP_LOGW(TAG, "Bus discovery failed (%s) — continuing with addr=%u",
                 al::ToString(disc.error).data(), kPrimaryAddr);
    } else {
        ESP_LOGI(TAG, "Bus discovery: %u BASIS-2 instrument(s) responded", disc.value);
        for (uint16_t a = 1; a <= 247; ++a) {
            if (present[a / 8] & (1U << (a % 8))) {
                ESP_LOGI(TAG, "    addr=%u present", a);
            }
        }
    }

    // 3) Identity.
    auto id = meter.ReadIdentity();
    if (!id.ok()) {
        ESP_LOGE(TAG, "ReadIdentity(addr=%u) failed: %s",
                 kPrimaryAddr, al::ToString(id.error).data());
    } else {
        const auto& v = id.value;
        ESP_LOGI(TAG, "BASIS-2 @ addr=%u identity:", v.modbus_address);
        ESP_LOGI(TAG, "    firmware     = %u.%u.%u (raw 0x%04X)",
                 v.fw_major, v.fw_minor, v.fw_patch, v.firmware_version_raw);
        ESP_LOGI(TAG, "    serial       = %s",  v.serial_number);
        ESP_LOGI(TAG, "    ascii unit   = '%c'", v.ascii_unit_id);
        ESP_LOGI(TAG, "    flow units   = %s",  al::ToString(v.flow_units).data());
        ESP_LOGI(TAG, "    full-scale   = %lu (in user units / 1000)",
                 static_cast<unsigned long>(v.full_scale_flow_raw));
    }

    // 4-6) Configuration walkthrough (best-effort; meters may NACK setpoint regs).
    ESP_LOGI(TAG, "Configuration walkthrough:");
    log_rc("SetGas(N2)",                  meter.SetGas(al::Gas::Nitrogen).error);
    log_rc("SetReferenceTemperatureC(25)", meter.SetReferenceTemperatureC(25.0f).error);
    log_rc("SetFlowAveragingMs(50)",      meter.SetFlowAveragingMs(50).error);
    log_rc("SetTotalizerLimitMode(SaturateNoError)",
           meter.SetTotalizerLimitMode(al::TotalizerLimitMode::SaturateNoError).error);
    log_rc("SetCommWatchdogMs(2000)",     meter.SetCommWatchdogMs(2000).error);

    ESP_LOGI(TAG, "Setpoint demo (controllers only):");
    log_rc("SetSetpointSource(DigitalUnsaved)",
           meter.SetSetpointSource(al::SetpointSource::DigitalUnsaved).error);
    log_rc("SetSetpoint(2.5)",            meter.SetSetpoint(2.5f).error);
    log_rc("Tare()",                      meter.Tare().error);
    log_rc("ResetTotalizer()",            meter.ResetTotalizer().error);

    // 8) Optional measurement-window demo: trigger 200 samples, then read.
    ESP_LOGI(TAG, "Measurement window demo:");
    log_rc("ConfigureMeasurementTrigger(none)",
           meter.ConfigureMeasurementTrigger(0).error);
    log_rc("StartMeasurementSamples(200)",
           meter.StartMeasurementSamples(200).error);
    vTaskDelay(pdMS_TO_TICKS(700));  // 200 samples × ~2.5 ms ≈ 500 ms + slack
    if (auto m = meter.ReadMeasurement(); m.ok()) {
        ESP_LOGI(TAG, "    avg T=%.2f C  avg flow_raw=%d  prev_avg_flow_raw=%ld",
                 static_cast<double>(m.value.avg_temperature_c),
                 m.value.avg_flow_raw,
                 static_cast<long>(m.value.prev_avg_flow_raw));
    } else {
        ESP_LOGW(TAG, "    ReadMeasurement: %s", al::ToString(m.error).data());
    }

    // 7) Steady-state read loop.
    ESP_LOGI(TAG, "Entering periodic read loop @ %lu ms", static_cast<unsigned long>(kReadIntervalMs));
    while (true) {
        const auto live = meter.ReadInstantaneous();
        if (live.ok()) {
            const auto& d = live.value;
            ESP_LOGI(TAG,
                     "BASIS-2 %s | T=%.2f C | flow=%.4f | total=%.4f | sp=%.4f | drive=%.1f%% | "
                     "[MOV=%d TOV=%d OVR=%d HLD=%d VTM=%d]",
                     al::GasShortName(d.gas).data(),
                     static_cast<double>(d.temperature_c),
                     static_cast<double>(d.flow),
                     static_cast<double>(d.total_volume),
                     static_cast<double>(d.setpoint),
                     static_cast<double>(d.valve_drive_pct),
                     d.mass_flow_overrange, d.temperature_overrange,
                     d.totalizer_overrange, d.valve_held,
                     d.valve_thermal_management);
        } else {
            ESP_LOGW(TAG, "ReadInstantaneous failed: %s", al::ToString(live.error).data());
        }
        vTaskDelay(pdMS_TO_TICKS(kReadIntervalMs));
    }
}
