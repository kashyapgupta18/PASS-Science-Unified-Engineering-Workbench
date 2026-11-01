// =============================================================================
// batch.cpp — Serial & Parallel Batch Solver Implementation
// Unified Engineering Workbench
// =============================================================================

#include "wb/batch.h"
#include "wb/fem1d.h"
#include "wb/sysinfo.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>
#include <vector>

namespace wb {

// ---------------------------------------------------------------------------
// Generate batch of FEM parameter sets with fixed seed
// ---------------------------------------------------------------------------
static std::vector<FemParams> generate_batch(const BatchConfig& config) {
    std::mt19937_64 rng(config.seed);
    std::uniform_real_distribution<double> k_dist(1.0, 50.0);    // W/(m·K)
    std::uniform_real_distribution<double> q_dist(1e3, 1e5);     // W/m³

    std::vector<FemParams> cases(config.case_count);
    for (int i = 0; i < config.case_count; ++i) {
        FemParams& p = cases[i];
        p.L  = 1.0;
        p.A  = 1e-4;
        p.k  = k_dist(rng);
        p.q  = q_dist(rng);
        p.T0 = 100.0;
        p.TL = 20.0;
        p.N  = config.mesh_size;
    }
    return cases;
}

// ---------------------------------------------------------------------------
// Solve a batch: extract max temperature from each case
// ---------------------------------------------------------------------------
static std::vector<double> solve_serial(const std::vector<FemParams>& cases) {
    std::vector<double> results(cases.size());
    for (size_t i = 0; i < cases.size(); ++i) {
        FemResult r = fem1d_solve(cases[i]);
        results[i] = *std::max_element(r.T_nodes.begin(), r.T_nodes.end());
    }
    return results;
}

static std::vector<double> solve_parallel(const std::vector<FemParams>& cases,
                                           int num_workers) {
    std::vector<double> results(cases.size());
    std::atomic<size_t> next_case{0};

    auto worker = [&]() {
        while (true) {
            size_t idx = next_case.fetch_add(1, std::memory_order_relaxed);
            if (idx >= cases.size()) break;
            FemResult r = fem1d_solve(cases[idx]);
            results[idx] = *std::max_element(r.T_nodes.begin(), r.T_nodes.end());
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_workers);
    for (int t = 0; t < num_workers; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) {
        th.join();
    }
    return results;
}

// ---------------------------------------------------------------------------
// Public batch_solve
// ---------------------------------------------------------------------------
std::vector<double> batch_solve(const BatchConfig& config, int num_workers) {
    auto cases = generate_batch(config);
    if (num_workers <= 1) {
        return solve_serial(cases);
    }
    return solve_parallel(cases, num_workers);
}

// ---------------------------------------------------------------------------
// Median helper
// ---------------------------------------------------------------------------
static double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    if (n % 2 == 0)
        return (v[n / 2 - 1] + v[n / 2]) / 2.0;
    return v[n / 2];
}

// ---------------------------------------------------------------------------
// Full benchmark
// ---------------------------------------------------------------------------
BenchmarkReport batch_benchmark(const BatchConfig& config) {
    using clock = std::chrono::high_resolution_clock;

    BenchmarkReport report;

    // Gather system info
    SystemInfo sys = get_system_info();
    report.cpu_name       = sys.cpu_name;
    report.physical_cores = sys.physical_cores;
    report.logical_cores  = sys.logical_cores;
    report.ram_bytes      = sys.ram_bytes;
    report.os_name        = sys.os_name;
    report.compiler       = sys.compiler;
    report.build_type     = sys.build_type;
#ifdef _MSC_VER
    #ifdef NDEBUG
        report.opt_flags = "/O2 (Release)";
    #else
        report.opt_flags = "/Od (Debug)";
    #endif
#else
    report.opt_flags = "Unknown";
#endif
    report.config = config;

    // Generate cases once
    auto cases = generate_batch(config);

    // Worker counts: 1, 2, and up to 4 (capped at logical CPUs)
    int max_workers = std::min(4, report.logical_cores);
    std::vector<int> worker_counts = {1};
    if (max_workers >= 2) worker_counts.push_back(2);
    if (max_workers >= 4) worker_counts.push_back(4);

    // Reference serial results for agreement check
    std::vector<double> serial_results;

    double t1_median = 0.0;  // Serial median for speedup calculation

    for (int nw : worker_counts) {
        BenchmarkResult br;
        br.num_workers = nw;

        // Warm-up run
        if (nw == 1) {
            solve_serial(cases);
        } else {
            solve_parallel(cases, nw);
        }

        // 5 timed repetitions
        for (int rep = 0; rep < 5; ++rep) {
            auto t_start = clock::now();

            std::vector<double> results;
            if (nw == 1) {
                results = solve_serial(cases);
            } else {
                results = solve_parallel(cases, nw);
            }

            auto t_end = clock::now();
            double elapsed_ms = std::chrono::duration<double, std::milli>(
                t_end - t_start).count();
            br.all_times_ms.push_back(elapsed_ms);

            // Store serial results for comparison
            if (nw == 1 && rep == 0) {
                serial_results = results;
            }

            // Check agreement with serial results
            if (nw > 1 && !serial_results.empty()) {
                double max_diff = 0.0;
                for (size_t i = 0; i < results.size(); ++i) {
                    max_diff = std::max(max_diff,
                        std::abs(results[i] - serial_results[i]));
                }
                br.max_temp_diff = std::max(br.max_temp_diff, max_diff);
            }
        }

        br.median_time_ms = median(br.all_times_ms);
        br.min_time_ms = *std::min_element(br.all_times_ms.begin(),
                                            br.all_times_ms.end());
        br.max_time_ms = *std::max_element(br.all_times_ms.begin(),
                                            br.all_times_ms.end());

        if (nw == 1) {
            t1_median = br.median_time_ms;
            br.speedup = 1.0;
            br.efficiency = 1.0;
        } else {
            br.speedup = t1_median / br.median_time_ms;
            br.efficiency = br.speedup / nw;
        }

        report.results.push_back(br);
    }

    // Overall agreement check
    report.agreement_check_passed = true;
    for (const auto& br : report.results) {
        if (br.num_workers > 1 && br.max_temp_diff > 1e-8) {
            report.agreement_check_passed = false;
        }
    }

    return report;
}

}  // namespace wb

