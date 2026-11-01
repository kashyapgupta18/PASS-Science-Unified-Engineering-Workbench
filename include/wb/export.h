// =============================================================================
// export.h — CSV/Data Export Utilities
// Unified Engineering Workbench
// =============================================================================
// Functions to export FEM results, convergence data, benchmark results,
// and backpropagation loss history to CSV files in the results/ directory.
// =============================================================================

#ifndef WB_EXPORT_H
#define WB_EXPORT_H

#include "backprop.h"
#include "batch.h"
#include "fem1d.h"

#include <string>
#include <vector>

namespace wb {

/// Export FEM nodal results to CSV.
/// Columns: x, T_fem, T_exact, error
/// @param result    FEM result to export
/// @param params    FEM parameters (for analytical solution)
/// @param filepath  Output file path
/// @return True if export succeeded
bool export_fem_results(const FemResult& result,
                        const FemParams& params,
                        const std::string& filepath);

/// Export FEM midpoint results to CSV.
/// Columns: x_mid, T_mid_fe, T_mid_exact, error
/// @param result    FEM result to export
/// @param filepath  Output file path
/// @return True if export succeeded
bool export_fem_midpoint_results(const FemResult& result,
                                 const std::string& filepath);

/// Export convergence data from multiple FEM runs to CSV.
/// Columns: N, h, max_midpoint_error, max_nodal_error
/// @param results   Vector of FEM results at different mesh sizes
/// @param filepath  Output file path
/// @return True if export succeeded
bool export_convergence_data(const std::vector<FemResult>& results,
                             const std::string& filepath);

/// Export benchmark results to CSV.
/// Columns: num_workers, median_ms, min_ms, max_ms, speedup, efficiency
/// @param report    Benchmark report to export
/// @param filepath  Output file path
/// @return True if export succeeded
bool export_benchmark_results(const BenchmarkReport& report,
                              const std::string& filepath);

/// Export backpropagation loss history to CSV.
/// Columns: step, loss, w, b, v, c
/// @param result    Backprop result to export
/// @param filepath  Output file path
/// @return True if export succeeded
bool export_backprop_loss(const BackpropResult& result,
                          const std::string& filepath);

}  // namespace wb

#endif  // WB_EXPORT_H
