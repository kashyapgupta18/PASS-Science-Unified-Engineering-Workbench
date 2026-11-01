// =============================================================================
// batch.h — Serial & Parallel Batch Solver with Benchmarking
// Unified Engineering Workbench
// =============================================================================
// Executes independent 1D thermal FEM solves in serial and parallel using a
// std::thread pool with atomic work-stealing. Benchmarks with warm-up runs
// and timed repetitions to measure speedup and efficiency.
// =============================================================================

#ifndef WB_BATCH_H
#define WB_BATCH_H

#include <cstdint>
#include <string>
#include <vector>

namespace wb {

/// Configuration for the batch solver benchmark.
struct BatchConfig {
    int      case_count = 200;   ///< Number of independent cases to solve
    int      mesh_size  = 80;    ///< Number of FEM elements per case
    uint64_t seed       = 42;    ///< Random seed for reproducibility
    // k range: [1, 50] W/(m·K)
    // q range: [1e3, 1e5] W/m³
};

/// Result of a single benchmark configuration (specific worker count).
struct BenchmarkResult {
    int    num_workers;     ///< Number of worker threads (1 = serial)
    double median_time_ms;  ///< Median wall-clock time of timed runs [ms]
    double min_time_ms;     ///< Minimum wall-clock time [ms]
    double max_time_ms;     ///< Maximum wall-clock time [ms]
    double speedup;         ///< T1 / Tp
    double efficiency;      ///< speedup / p
    double max_temp_diff;   ///< Max |serial - parallel| temp difference
    std::vector<double> all_times_ms;  ///< All 5 timed repetition times [ms]
};

/// Complete benchmark report including system info and all results.
struct BenchmarkReport {
    std::string cpu_name;            ///< CPU model name
    int         physical_cores = 0;  ///< Physical core count
    int         logical_cores  = 0;  ///< Logical processor count
    size_t      ram_bytes      = 0;  ///< Total RAM [bytes]
    std::string os_name;             ///< Operating system name
    std::string compiler;            ///< Compiler identification
    std::string build_type;          ///< Debug or Release
    std::string opt_flags;           ///< Optimization flags used
    BatchConfig config;              ///< Batch configuration used
    std::vector<BenchmarkResult> results;  ///< Results for each worker count
    double launch_to_ready_ms = 0.0;       ///< Startup overhead [ms]
    bool   agreement_check_passed = false; ///< True if max_temp_diff <= 1e-8
};

/// Run the full benchmark suite.
/// Executes the batch solver with 1, 2, and 4 workers.
/// Each configuration: 1 warm-up + 5 timed repetitions.
/// @param config  Batch configuration parameters
/// @return        Complete benchmark report
BenchmarkReport batch_benchmark(const BatchConfig& config = {});

/// Run a batch of FEM solves with the specified number of workers.
/// @param config       Batch configuration
/// @param num_workers  Number of parallel threads (1 = serial)
/// @return             Vector of max nodal temperatures per case
std::vector<double> batch_solve(const BatchConfig& config, int num_workers);

}  // namespace wb

#endif  // WB_BATCH_H
