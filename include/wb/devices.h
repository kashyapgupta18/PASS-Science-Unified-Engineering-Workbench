// =============================================================================
// devices.h — Embedded Device Mock Adapters
// Unified Engineering Workbench
// =============================================================================
// Mock adapters for MCU, FPGA, and JTAG devices. Provides connect/disconnect
// lifecycle, telemetry generation, fault injection for testing, and
// device-specific capability reporting.
// =============================================================================

#ifndef WB_DEVICES_H
#define WB_DEVICES_H

#include <random>
#include <string>
#include <vector>

namespace wb {

/// Device type enumeration.
enum class DeviceType { MCU, FPGA, JTAG };

/// A single device capability (key-value pair).
struct DeviceCapability {
    std::string name;   ///< Capability name
    std::string value;  ///< Capability value
};

/// A telemetry message from a device.
struct TelemetryMessage {
    std::string device_name;    ///< Name of the reporting device
    double      timestamp_s;    ///< Timestamp in seconds since connection
    std::string payload_json;   ///< JSON-formatted telemetry payload
};

/// Mock device adapter supporting connect/disconnect, telemetry, and fault injection.
class DeviceAdapter {
public:
    /// Construct a device adapter.
    /// @param type  Device type (MCU, FPGA, JTAG)
    /// @param name  Human-readable device name
    DeviceAdapter(DeviceType type, const std::string& name);

    /// Get the device type.
    DeviceType type() const;

    /// Get the device name.
    const std::string& name() const;

    /// Get device-specific capabilities.
    std::vector<DeviceCapability> capabilities() const;

    /// Attempt to connect to the device.
    /// @param timeout_ms  Connection timeout in milliseconds (unused in mock)
    /// @return True if connection succeeded
    bool connect(int timeout_ms = 5000);

    /// Attempt to connect with automatic retries.
    /// @param max_retries  Maximum number of attempts
    /// @param timeout_ms   Timeout per attempt in milliseconds
    /// @return True if connection eventually succeeded
    bool connect_with_retry(int max_retries = 3, int timeout_ms = 2000);

    /// Disconnect from the device.
    void disconnect();

    /// Check if device is currently connected.
    bool is_connected() const;

    /// Read a telemetry message from the device.
    /// @return TelemetryMessage with device-specific data
    /// @throws std::runtime_error if not connected
    TelemetryMessage read_telemetry();

    /// Inject connection failures for testing.
    /// The next num_failures connect() calls will fail.
    /// @param num_failures  Number of failures to inject
    void inject_connect_fault(int num_failures);

private:
    DeviceType   type_;
    std::string  name_;
    bool         connected_ = false;
    int          injected_failures_ = 0;
    int          connect_attempts_ = 0;
    std::mt19937 rng_;
    double       connect_time_ = 0.0;   ///< Simulated time at connection
    int          telemetry_seq_ = 0;    ///< Telemetry message sequence counter
};

/// Create a pre-configured MCU device adapter (STM32F407).
DeviceAdapter create_mcu_device();

/// Create a pre-configured FPGA device adapter (Xilinx Artix-7).
DeviceAdapter create_fpga_device();

/// Create a pre-configured JTAG device adapter (SEGGER J-Link).
DeviceAdapter create_jtag_device();

/// Convert DeviceType to a human-readable string.
const char* device_type_to_string(DeviceType type);

}  // namespace wb

#endif  // WB_DEVICES_H
