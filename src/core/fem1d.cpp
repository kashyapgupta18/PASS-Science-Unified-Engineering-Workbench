// =============================================================================
// fem1d.cpp — 1D Finite Element Thermal Solver Implementation
// Unified Engineering Workbench
// =============================================================================

#include "wb/fem1d.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace wb {

// ---------------------------------------------------------------------------
// Analytical solution: T(x) = T0 + (TL-T0)*x/L + q/(2k)*x*(L-x)
// ---------------------------------------------------------------------------
double fem1d_analytical(double x, const FemParams& params) {
    double slope = (params.TL - params.T0) * x / params.L;
    double heat  = (params.q / (2.0 * params.k)) * x * (params.L - x);
    return params.T0 + slope + heat;
}

// ---------------------------------------------------------------------------
// Convergence order: log(e1/e2) / log(h1/h2)
// ---------------------------------------------------------------------------
double fem1d_convergence_order(double h1, double e1, double h2, double e2) {
    if (e1 <= 0.0 || e2 <= 0.0 || h1 <= 0.0 || h2 <= 0.0) return 0.0;
    return std::log(e1 / e2) / std::log(h1 / h2);
}

// ---------------------------------------------------------------------------
// FEM Solver
// ---------------------------------------------------------------------------
FemResult fem1d_solve(const FemParams& p) {
    FemResult result;
    result.N = p.N;

    // --- Validate parameters ------------------------------------------------
    if (p.N < 1) {
        result.error_message = "Number of elements N must be >= 1, got " + std::to_string(p.N);
        return result;
    }
    if (p.L <= 0.0) {
        result.error_message = "Rod length L must be > 0, got " + std::to_string(p.L);
        return result;
    }
    if (p.A <= 0.0) {
        result.error_message = "Cross-section area A must be > 0";
        return result;
    }
    if (p.k <= 0.0) {
        result.error_message = "Thermal conductivity k must be > 0";
        return result;
    }

    const int N = p.N;
    const int n_nodes = N + 1;
    const double h = p.L / N;
    result.h = h;

    // --- Build and assemble the global system K*T = F -----------------------
    // For a tridiagonal system we only store three diagonals:
    //   lower[i], diag[i], upper[i]
    std::vector<double> diag(n_nodes, 0.0);
    std::vector<double> upper(n_nodes, 0.0);  // upper[i] = K[i][i+1]
    std::vector<double> lower(n_nodes, 0.0);  // lower[i] = K[i][i-1]
    std::vector<double> F(n_nodes, 0.0);

    // Element stiffness: k_e = (k*A/h) * [[1, -1], [-1, 1]]
    // Element load:      f_e = (q*A*h/2) * [1, 1]
    double ke = p.k * p.A / h;
    double fe = p.q * p.A * h / 2.0;

    for (int e = 0; e < N; ++e) {
        int i = e;      // Left node of element
        int j = e + 1;  // Right node of element

        // Assemble stiffness
        diag[i] += ke;
        diag[j] += ke;
        upper[i] -= ke;
        lower[j] -= ke;

        // Assemble load
        F[i] += fe;
        F[j] += fe;
    }

    // --- Apply Dirichlet boundary conditions --------------------------------
    // BC at x=0: T[0] = T0
    diag[0] = 1.0;
    upper[0] = 0.0;
    F[0] = p.T0;

    // BC at x=L: T[N] = TL
    diag[N] = 1.0;
    lower[N] = 0.0;
    F[N] = p.TL;

    // --- Solve tridiagonal system (Thomas algorithm) ------------------------
    // Forward elimination
    std::vector<double> c_star(n_nodes, 0.0);
    std::vector<double> d_star(n_nodes, 0.0);

    c_star[0] = upper[0] / diag[0];
    d_star[0] = F[0] / diag[0];

    for (int i = 1; i < n_nodes; ++i) {
        double denom = diag[i] - lower[i] * c_star[i - 1];
        if (std::abs(denom) < 1e-30) {
            result.error_message = "Thomas algorithm: near-zero pivot at row " + std::to_string(i);
            return result;
        }
        c_star[i] = upper[i] / denom;
        d_star[i] = (F[i] - lower[i] * d_star[i - 1]) / denom;
    }

    // Back substitution
    std::vector<double> T(n_nodes);
    T[n_nodes - 1] = d_star[n_nodes - 1];
    for (int i = n_nodes - 2; i >= 0; --i) {
        T[i] = d_star[i] - c_star[i] * T[i + 1];
    }

    // --- Compute node positions ---------------------------------------------
    result.x_nodes.resize(n_nodes);
    for (int i = 0; i < n_nodes; ++i) {
        result.x_nodes[i] = i * h;
    }
    result.T_nodes = T;

    // --- Compute midpoint temperatures and errors ---------------------------
    result.x_mid.resize(N);
    result.T_mid_fe.resize(N);
    result.T_mid_exact.resize(N);

    double max_mid_err = 0.0;
    double max_nod_err = 0.0;

    for (int e = 0; e < N; ++e) {
        double x_m = (result.x_nodes[e] + result.x_nodes[e + 1]) / 2.0;
        double T_fe = (T[e] + T[e + 1]) / 2.0;  // FE interpolation at midpoint
        double T_ex = fem1d_analytical(x_m, p);

        result.x_mid[e] = x_m;
        result.T_mid_fe[e] = T_fe;
        result.T_mid_exact[e] = T_ex;

        max_mid_err = std::max(max_mid_err, std::abs(T_fe - T_ex));
    }

    for (int i = 0; i < n_nodes; ++i) {
        double T_ex = fem1d_analytical(result.x_nodes[i], p);
        max_nod_err = std::max(max_nod_err, std::abs(T[i] - T_ex));
    }

    result.max_midpoint_error = max_mid_err;
    result.max_nodal_error = max_nod_err;
    result.success = true;
    return result;
}

}  // namespace wb

