// =============================================================================
// sysinfo.h — System Information Gathering
// Unified Engineering Workbench
// =============================================================================
// Gathers system information on Windows: CPU, RAM, OS, compiler details.
// Uses Windows API for runtime queries and compiler macros for build info.
// =============================================================================

#ifndef WB_SYSINFO_H
#define WB_SYSINFO_H

#include <cstdint>
#include <string>

namespace wb {

/// System information snapshot.
struct SystemInfo {
    std::string cpu_name;            ///< CPU model name (e.g., "AMD Ryzen 9 5900X")
    int         physical_cores = 0;  ///< Physical core count
    int         logical_cores  = 0;  ///< Logical processor (thread) count
    double      clock_speed_ghz = 0; ///< Base clock speed [GHz]
    uint64_t    ram_bytes = 0;       ///< Total physical RAM [bytes]
    std::string os_name;             ///< OS name and version
    std::string compiler;            ///< Compiler identification string
    std::string build_type;          ///< "Release" or "Debug"
};

/// Gather system information.
/// @return SystemInfo populated with current system details
SystemInfo get_system_info();

/// Format system info as a human-readable multi-line string.
/// @param info  System information to format
/// @return      Formatted string
std::string format_system_info(const SystemInfo& info);

/// Format a byte count as a human-readable string (e.g., "16.0 GB").
/// @param bytes  Byte count
/// @return       Formatted string
std::string format_bytes(uint64_t bytes);

}  // namespace wb

#endif  // WB_SYSINFO_H
