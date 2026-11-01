// =============================================================================
// fem1d.h — 1D Finite Element Thermal Solver
// Unified Engineering Workbench
// =============================================================================
// Steady-state 1D thermal FEM solver for a uniform rod using two-node linear
// elements. Solves: -d/dx(kA dT/dx) = qA with Dirichlet boundary conditions.
// =============================================================================

#ifndef WB_FEM1D_H
#define WB_FEM1D_H

#include <string>
#include <vector>

namespace wb {

/// Parameters for the 1D thermal FEM problem.
struct FemParams {
    double L  = 1.0;     ///< Rod length [m]
    double A  = 1e-4;    ///< Cross-section area [m²]
    double k  = 10.0;    ///< Thermal conductivity [W/(m·K)]
    double q  = 1e4;     ///< Volumetric heat generation [W/m³]
    double T0 = 100.0;   ///< Temperature at x=0 boundary [°C]
    double TL = 20.0;    ///< Temperature at x=L boundary [°C]
    int    N  = 10;      ///< Number of elements
};

/// Results from a single FEM solve.
struct FemResult {
    std::vector<double> x_nodes;      ///< Node positions [N+1]
    std::vector<double> T_nodes;      ///< Nodal temperatures [N+1]
    std::vector<double> x_mid;        ///< Element midpoint positions [N]
    std::vector<double> T_mid_fe;     ///< FE midpoint temperatures [N]
    std::vector<double> T_mid_exact;  ///< Analytical midpoint temperatures [N]
    double max_midpoint_error = 0.0;  ///< Max |T_mid_fe - T_mid_exact|
    double max_nodal_error    = 0.0;  ///< Max |T_node - T_exact(x_node)|
    int    N = 0;                     ///< Number of elements used
    double h = 0.0;                   ///< Element size [m]
    bool   success = false;           ///< True if solve completed without error
    std::string error_message;        ///< Description of any error
};

/// Solve the 1D thermal FEM problem with the given parameters.
/// @param params  Problem parameters (geometry, material, BCs, mesh)
/// @return        FemResult containing nodal/midpoint temperatures and errors
FemResult fem1d_solve(const FemParams& params);

/// Compute the analytical solution at position x.
/// T_exact(x) = T0 + (TL - T0)*x/L + q/(2*k) * x*(L - x)
/// @param x       Position [m]
/// @param params  Problem parameters
/// @return        Analytical temperature [°C]
double fem1d_analytical(double x, const FemParams& params);

/// Compute convergence order from two error/mesh-size pairs.
/// order = log(e1/e2) / log(h1/h2)
/// @param h1  Coarser mesh size
/// @param e1  Error on coarser mesh
/// @param h2  Finer mesh size
/// @param e2  Error on finer mesh
/// @return    Estimated convergence order
double fem1d_convergence_order(double h1, double e1, double h2, double e2);

}  // namespace wb

#endif  // WB_FEM1D_H
