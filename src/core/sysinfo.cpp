// =============================================================================
// sysinfo.cpp — System Information Gathering (Windows)
// Unified Engineering Workbench
// =============================================================================

#include "wb/sysinfo.h"

#include <iomanip>
#include <sstream>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace wb {

// ---------------------------------------------------------------------------
// format_bytes: Human-readable byte count
// ---------------------------------------------------------------------------
std::string format_bytes(uint64_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    if (bytes >= (1ULL << 30))
        oss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
    else if (bytes >= (1ULL << 20))
        oss << (bytes / (1024.0 * 1024.0)) << " MB";
    else
        oss << (bytes / 1024.0) << " KB";
    return oss.str();
}

// ---------------------------------------------------------------------------
// get_system_info
// ---------------------------------------------------------------------------
SystemInfo get_system_info() {
    SystemInfo info;

    // --- Compiler info (compile-time macros) --------------------------------
#ifdef _MSC_VER
    info.compiler = "MSVC " + std::to_string(_MSC_VER);
    #if _MSC_VER >= 1944
        info.compiler += " (VS 2022 17.14+)";
    #elif _MSC_VER >= 1940
        info.compiler += " (VS 2022 17.10+)";
    #elif _MSC_VER >= 1930
        info.compiler += " (VS 2022)";
    #endif
#elif defined(__GNUC__)
    info.compiler = "GCC " + std::to_string(__GNUC__) + "." +
                    std::to_string(__GNUC_MINOR__) + "." +
                    std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(__clang__)
    info.compiler = "Clang " + std::string(__clang_version__);
#else
    info.compiler = "Unknown compiler";
#endif

    // --- Build type ---------------------------------------------------------
#ifdef NDEBUG
    info.build_type = "Release";
#else
    info.build_type = "Debug";
#endif

#ifdef _WIN32
    // --- CPU info from registry ---------------------------------------------
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buf[256] = {};
        DWORD bufSize = sizeof(buf);
        if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(buf), &bufSize) == ERROR_SUCCESS) {
            info.cpu_name = buf;
        }
        DWORD mhz = 0;
        DWORD mhzSize = sizeof(mhz);
        if (RegQueryValueExA(hKey, "~MHz", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(&mhz), &mhzSize) == ERROR_SUCCESS) {
            info.clock_speed_ghz = mhz / 1000.0;
        }
        RegCloseKey(hKey);
    }

    // --- Core counts --------------------------------------------------------
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    info.logical_cores = static_cast<int>(si.dwNumberOfProcessors);

    // Physical cores via GetLogicalProcessorInformation
    DWORD bufferSize = 0;
    GetLogicalProcessorInformation(nullptr, &bufferSize);
    if (bufferSize > 0) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
            bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
            int physical = 0;
            for (const auto& entry : buffer) {
                if (entry.Relationship == RelationProcessorCore) {
                    physical++;
                }
            }
            info.physical_cores = physical;
        }
    }
    if (info.physical_cores == 0) {
        info.physical_cores = info.logical_cores;
    }

    // --- RAM ----------------------------------------------------------------
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        info.ram_bytes = mem.ullTotalPhys;
    }

    // --- OS version ---------------------------------------------------------
    info.os_name = "Windows";
    // Try reading from registry for detailed version
    HKEY hKeyOS;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0, KEY_READ, &hKeyOS) == ERROR_SUCCESS) {
        char product[256] = {};
        DWORD prodSize = sizeof(product);
        if (RegQueryValueExA(hKeyOS, "ProductName", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(product), &prodSize) == ERROR_SUCCESS) {
            info.os_name = product;
        }
        char build[64] = {};
        DWORD buildSize = sizeof(build);
        if (RegQueryValueExA(hKeyOS, "CurrentBuild", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(build), &buildSize) == ERROR_SUCCESS) {
            info.os_name += " Build " + std::string(build);
        }
        RegCloseKey(hKeyOS);
    }
#else
    // Minimal fallback for non-Windows builds
    info.cpu_name = "Unknown CPU";
    info.logical_cores = static_cast<int>(std::thread::hardware_concurrency());
    info.physical_cores = info.logical_cores;
    info.os_name = "Non-Windows";
#endif

    return info;
}

// ---------------------------------------------------------------------------
// format_system_info
// ---------------------------------------------------------------------------
std::string format_system_info(const SystemInfo& info) {
    std::ostringstream oss;
    oss << "CPU:       " << info.cpu_name << "\n"
        << "Cores:     " << info.physical_cores << " physical / "
                         << info.logical_cores << " logical\n"
        << "Clock:     " << std::fixed << std::setprecision(2)
                         << info.clock_speed_ghz << " GHz\n"
        << "RAM:       " << format_bytes(info.ram_bytes) << "\n"
        << "OS:        " << info.os_name << "\n"
        << "Compiler:  " << info.compiler << "\n"
        << "Build:     " << info.build_type << "\n";
    return oss.str();
}

}  // namespace wb

