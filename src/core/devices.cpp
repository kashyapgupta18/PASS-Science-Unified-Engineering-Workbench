// =============================================================================
// devices.cpp — Embedded Device Mock Adapters Implementation
// Unified Engineering Workbench
// =============================================================================

#include "wb/devices.h"

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <iomanip>

namespace wb {

// ---------------------------------------------------------------------------
// DeviceAdapter implementation
// ---------------------------------------------------------------------------
DeviceAdapter::DeviceAdapter(DeviceType type, const std::string& name)
    : type_(type), name_(name),
      rng_(static_cast<unsigned>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {}

DeviceType DeviceAdapter::type() const { return type_; }

const std::string& DeviceAdapter::name() const { return name_; }

std::vector<DeviceCapability> DeviceAdapter::capabilities() const {
    std::vector<DeviceCapability> caps;
    switch (type_) {
    case DeviceType::MCU:
        caps.push_back({"Architecture", "ARM Cortex-M4"});
        caps.push_back({"Flash", "1024 KB"});
        caps.push_back({"RAM", "192 KB"});
        caps.push_back({"Clock", "168 MHz"});
        caps.push_back({"Supply Voltage", "3.3 V"});
        caps.push_back({"ADC Channels", "16 (12-bit)"});
        caps.push_back({"UART Ports", "6"});
        caps.push_back({"Communication", "USB-UART bridge, 115200 8N1"});
        caps.push_back({"Firmware", "v1.2.3 (simulated)"});
        break;
    case DeviceType::FPGA:
        caps.push_back({"Family", "Artix-7 (XC7A35T)"});
        caps.push_back({"Logic Cells", "33,280"});
        caps.push_back({"Block RAM", "225 KB (50 x 36 Kb)"});
        caps.push_back({"DSP Slices", "90"});
        caps.push_back({"I/O Pins", "250"});
        caps.push_back({"Core Voltage", "1.0 V"});
        caps.push_back({"I/O Voltage", "3.3 V (Bank 34)"});
        caps.push_back({"Max Clock", "450 MHz"});
        caps.push_back({"Communication", "USB-UART bridge, 115200 8N1"});
        break;
    case DeviceType::JTAG:
        caps.push_back({"Type", "Debug adapter (not a compute board)"});
        caps.push_back({"Model", "SEGGER J-Link EDU"});
        caps.push_back({"Protocols", "JTAG, SWD"});
        caps.push_back({"Target Voltage", "1.65 - 3.6 V (VTref)"});
        caps.push_back({"Speed", "Up to 15 MHz"});
        caps.push_back({"Drives", "TCK, TMS, TDI (adapter output)"});
        caps.push_back({"Reads", "TDO (target output)"});
        caps.push_back({"VTref", "Input from target for level shifting"});
        caps.push_back({"Capabilities", "Reset, halt, step, flash, read memory"});
        break;
    }
    return caps;
}

bool DeviceAdapter::connect(int /*timeout_ms*/) {
    connect_attempts_++;
    if (injected_failures_ > 0) {
        injected_failures_--;
        return false;  // Simulate timeout
    }
    connected_ = true;
    telemetry_seq_ = 0;
    connect_time_ = static_cast<double>(
        std::chrono::steady_clock::now().time_since_epoch().count()) / 1e9;
    return true;
}

bool DeviceAdapter::connect_with_retry(int max_retries, int timeout_ms) {
    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        if (connect(timeout_ms)) {
            return true;
        }
        // In a real system, we'd sleep here; mock just continues
    }
    return false;
}

void DeviceAdapter::disconnect() {
    connected_ = false;
    telemetry_seq_ = 0;
}

bool DeviceAdapter::is_connected() const {
    return connected_;
}

TelemetryMessage DeviceAdapter::read_telemetry() {
    if (!connected_) {
        throw std::runtime_error("Device '" + name_ + "' is not connected");
    }

    TelemetryMessage msg;
    msg.device_name = name_;
    msg.timestamp_s = telemetry_seq_ * 1.0;  // 1-second intervals
    telemetry_seq_++;

    std::uniform_real_distribution<double> noise(-0.5, 0.5);
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);

    switch (type_) {
    case DeviceType::MCU:
        json << "{\"temperature_C\": " << (42.0 + noise(rng_))
             << ", \"voltage_V\": " << (3.3 + noise(rng_) * 0.02)
             << ", \"uptime_s\": " << (telemetry_seq_ * 1.0)
             << ", \"firmware\": \"v1.2.3\"}";
        break;
    case DeviceType::FPGA:
        json << "{\"fabric_utilization_pct\": " << (35.0 + noise(rng_) * 2.0)
             << ", \"clock_mhz\": " << (100.0 + noise(rng_))
             << ", \"bram_used_kb\": " << (45.0 + noise(rng_))
             << ", \"temperature_C\": " << (55.0 + noise(rng_)* 2.0) << "}";
        break;
    case DeviceType::JTAG:
        json << "{\"target_voltage_V\": " << (3.3 + noise(rng_) * 0.01)
             << ", \"protocol\": \"JTAG\""
             << ", \"scan_chain_length\": 1"
             << ", \"target_state\": \"halted\"}";
        break;
    }

    msg.payload_json = json.str();
    return msg;
}

void DeviceAdapter::inject_connect_fault(int num_failures) {
    injected_failures_ = num_failures;
    connect_attempts_ = 0;
}

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------
DeviceAdapter create_mcu_device() {
    return DeviceAdapter(DeviceType::MCU, "STM32F407 Discovery");
}

DeviceAdapter create_fpga_device() {
    return DeviceAdapter(DeviceType::FPGA, "Xilinx Artix-7 (XC7A35T)");
}

DeviceAdapter create_jtag_device() {
    return DeviceAdapter(DeviceType::JTAG, "SEGGER J-Link EDU");
}

const char* device_type_to_string(DeviceType type) {
    switch (type) {
    case DeviceType::MCU:  return "MCU";
    case DeviceType::FPGA: return "FPGA";
    case DeviceType::JTAG: return "JTAG";
    }
    return "Unknown";
}

}  // namespace wb

