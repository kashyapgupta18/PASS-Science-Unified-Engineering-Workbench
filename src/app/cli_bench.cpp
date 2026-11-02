// =============================================================================
// cli_bench.cpp — Standalone Multicore Performance Benchmark Runner
// Unified Engineering Workbench
// =============================================================================

#include "wb/batch.h"
#include "wb/export.h"
#include "wb/sysinfo.h"

#include <filesystem>
#include <iomanip>
#include <iostream>

int main() {
    std::cout << "===================================================================\n";
    std::cout << "  MULTICORE BATCH THERMAL SOLVER BENCHMARK                         \n";
    std::cout << "===================================================================\n";

    wb::SystemInfo sys = wb::get_system_info();
    std::cout << wb::format_system_info(sys) << "\n";

    wb::BatchConfig cfg;
    cfg.case_count = 200;
    cfg.mesh_size = 80;
    cfg.seed = 42;

    std::cout << "Workload: " << cfg.case_count << " cases, "
              << cfg.mesh_size << " elements/case, Seed: " << cfg.seed << "\n";
    std::cout << "Protocol: 1 warm-up run + 5 timed repetitions per configuration\n\n";

    std::cout << "Running benchmark on this target system...\n";
    wb::BenchmarkReport report = wb::batch_benchmark(cfg);

    std::cout << "\nResults Table:\n";
    std::cout << std::setw(10) << "Workers"
              << std::setw(16) << "Median [ms]"
              << std::setw(14) << "Min [ms]"
              << std::setw(14) << "Max [ms]"
              << std::setw(12) << "Speedup"
              << std::setw(14) << "Efficiency"
              << std::setw(20) << "Max Temp Diff [C]\n";
    std::cout << std::string(100, '-') << "\n";

    for (const auto& br : report.results) {
        std::cout << std::setw(10) << br.num_workers
                  << std::setw(16) << std::fixed << std::setprecision(2) << br.median_time_ms
                  << std::setw(14) << std::fixed << std::setprecision(2) << br.min_time_ms
                  << std::setw(14) << std::fixed << std::setprecision(2) << br.max_time_ms
                  << std::setw(12) << std::fixed << std::setprecision(3) << br.speedup
                  << std::setw(14) << std::fixed << std::setprecision(3) << br.efficiency
                  << std::setw(20) << std::scientific << std::setprecision(3) << br.max_temp_diff
                  << "\n";
    }
    std::cout << std::string(100, '-') << "\n";

    std::cout << "Numerical Agreement Check (diff <= 1e-8 C): "
              << (report.agreement_check_passed ? "[PASSED]" : "[FAILED]") << "\n";

    std::filesystem::create_directories("results");
    wb::export_benchmark_results(report, "results/benchmark_results.csv");
    std::cout << "Exported results to results/benchmark_results.csv\n";

    return 0;
}

