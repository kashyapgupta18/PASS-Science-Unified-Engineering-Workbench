// =============================================================================
// cli_fem.cpp — Standalone 1D Thermal FEM Solver & Exporter
// Unified Engineering Workbench
// =============================================================================

#include "wb/export.h"
#include "wb/fem1d.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    std::cout << "===================================================================\n";
    std::cout << "  1D THERMAL FINITE ELEMENT SOLVER & CONVERGENCE STUDY             \n";
    std::cout << "===================================================================\n";

    wb::FemParams p;
    p.L = 1.0;
    p.A = 1e-4;
    p.k = 10.0;
    p.q = 1e4;
    p.T0 = 100.0;
    p.TL = 20.0;

    int mesh_sizes[] = {10, 20, 40, 80};
    std::vector<wb::FemResult> results;

    std::filesystem::create_directories("results");

    std::cout << std::setw(6)  << "N"
              << std::setw(12) << "h [m]"
              << std::setw(22) << "Max Mid Error [C]"
              << std::setw(22) << "Theoretical [C]"
              << std::setw(20) << "Max Node Error [C]\n";
    std::cout << std::string(82, '-') << "\n";

    for (int N : mesh_sizes) {
        p.N = N;
        wb::FemResult r = wb::fem1d_solve(p);
        if (!r.success) {
            std::cerr << "Error solving for N=" << N << ": " << r.error_message << "\n";
            return 1;
        }

        double theoretical = (p.q * r.h * r.h) / (8.0 * p.k);
        std::cout << std::setw(6)  << r.N
                  << std::setw(12) << std::fixed << std::setprecision(4) << r.h
                  << std::setw(22) << std::fixed << std::setprecision(8) << r.max_midpoint_error
                  << std::setw(22) << std::fixed << std::setprecision(8) << theoretical
                  << std::setw(20) << std::scientific << std::setprecision(3) << r.max_nodal_error
                  << "\n";

        std::string path_nodal = "results/fem_N" + std::to_string(N) + ".csv";
        wb::export_fem_results(r, p, path_nodal);

        results.push_back(r);
    }

    std::cout << std::string(82, '-') << "\n";

    // Observed convergence order
    double order = wb::fem1d_convergence_order(results[0].h, results[0].max_midpoint_error,
                                              results[3].h, results[3].max_midpoint_error);
    std::cout << "Observed convergence order: " << std::fixed << std::setprecision(4)
              << order << " (theoretical: 2.0000)\n";

    wb::export_convergence_data(results, "results/fem_convergence.csv");
    std::cout << "Exported convergence data to results/fem_convergence.csv\n";

    return 0;
}
