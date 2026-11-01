// =============================================================================
// export.cpp — CSV/Data Export Utilities Implementation
// Unified Engineering Workbench
// =============================================================================

#include "wb/export.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace wb {

// ---------------------------------------------------------------------------
// Export FEM nodal results
// ---------------------------------------------------------------------------
bool export_fem_results(const FemResult& result,
                        const FemParams& params,
                        const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "x_m,T_fem_C,T_exact_C,error_C\n";
    file << std::fixed << std::setprecision(10);

    for (size_t i = 0; i < result.x_nodes.size(); ++i) {
        double x = result.x_nodes[i];
        double T_ex = fem1d_analytical(x, params);
        double err = std::abs(result.T_nodes[i] - T_ex);
        file << x << ","
             << result.T_nodes[i] << ","
             << T_ex << ","
             << err << "\n";
    }
    return true;
}

// ---------------------------------------------------------------------------
// Export FEM midpoint results
// ---------------------------------------------------------------------------
bool export_fem_midpoint_results(const FemResult& result,
                                  const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "x_mid_m,T_mid_fe_C,T_mid_exact_C,error_C\n";
    file << std::fixed << std::setprecision(10);

    for (size_t i = 0; i < result.x_mid.size(); ++i) {
        double err = std::abs(result.T_mid_fe[i] - result.T_mid_exact[i]);
        file << result.x_mid[i] << ","
             << result.T_mid_fe[i] << ","
             << result.T_mid_exact[i] << ","
             << err << "\n";
    }
    return true;
}

// ---------------------------------------------------------------------------
// Export convergence data
// ---------------------------------------------------------------------------
bool export_convergence_data(const std::vector<FemResult>& results,
                              const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "N,h_m,max_midpoint_error_C,max_nodal_error_C\n";
    file << std::fixed;

    for (const auto& r : results) {
        file << r.N << ","
             << std::setprecision(6) << r.h << ","
             << std::setprecision(10) << r.max_midpoint_error << ","
             << std::scientific << r.max_nodal_error << std::fixed << "\n";
    }
    return true;
}

// ---------------------------------------------------------------------------
// Export benchmark results
// ---------------------------------------------------------------------------
bool export_benchmark_results(const BenchmarkReport& report,
                               const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# CPU: " << report.cpu_name << "\n"
         << "# Cores: " << report.physical_cores << " physical / "
         << report.logical_cores << " logical\n"
         << "# RAM: " << (report.ram_bytes / (1024*1024*1024)) << " GB\n"
         << "# OS: " << report.os_name << "\n"
         << "# Compiler: " << report.compiler << "\n"
         << "# Build: " << report.build_type << "\n"
         << "# Opt flags: " << report.opt_flags << "\n"
         << "# Cases: " << report.config.case_count
         << ", Mesh: " << report.config.mesh_size
         << ", Seed: " << report.config.seed << "\n"
         << "# Agreement check: " << (report.agreement_check_passed ? "PASSED" : "FAILED") << "\n"
         << "#\n";

    file << "workers,median_ms,min_ms,max_ms,speedup,efficiency,max_temp_diff_C";
    for (int i = 0; i < 5; ++i) file << ",run" << (i+1) << "_ms";
    file << "\n";

    file << std::fixed << std::setprecision(3);
    for (const auto& br : report.results) {
        file << br.num_workers << ","
             << br.median_time_ms << ","
             << br.min_time_ms << ","
             << br.max_time_ms << ","
             << std::setprecision(4) << br.speedup << ","
             << br.efficiency << ","
             << std::scientific << br.max_temp_diff << std::fixed
             << std::setprecision(3);
        for (double t : br.all_times_ms) {
            file << "," << t;
        }
        file << "\n";
    }
    return true;
}

// ---------------------------------------------------------------------------
// Export backprop loss history
// ---------------------------------------------------------------------------
bool export_backprop_loss(const BackpropResult& result,
                           const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "step,loss,w,b,v,c,dw,db,dv,dc\n";
    file << std::fixed << std::setprecision(12);

    for (size_t i = 0; i < result.steps.size(); ++i) {
        const auto& s = result.steps[i];
        file << i << ","
             << s.loss << ","
             << s.w << "," << s.b << ","
             << s.v << "," << s.c << ","
             << s.dw << "," << s.db << ","
             << s.dv << "," << s.dc << "\n";
    }
    return true;
}

}  // namespace wb

