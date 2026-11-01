// =============================================================================
// cli_backprop.cpp — Standalone Backpropagation Verification & Exporter
// Unified Engineering Workbench
// =============================================================================

#include "wb/backprop.h"
#include "wb/export.h"

#include <filesystem>
#include <iomanip>
#include <iostream>

int main() {
    std::cout << "===================================================================\n";
    std::cout << "  BACKPROPAGATION GRADIENT VERIFICATION & 100-STEP TRAINING        \n";
    std::cout << "===================================================================\n";

    wb::BackpropParams p;
    p.x = 1.0;
    p.y = 2.0;
    p.w0 = 0.5;
    p.b0 = 0.0;
    p.v0 = 1.0;
    p.c0 = 0.0;
    p.lr = 0.1;
    p.num_steps = 100;
    p.grad_check_eps = 1e-6;

    std::cout << "Model: h = tanh(w*x + b), y_hat = v*h + c, J = 0.5*(y_hat - y)^2\n";
    std::cout << "Inputs: x = " << p.x << ", target y = " << p.y << "\n";
    std::cout << "Initial: w = " << p.w0 << ", b = " << p.b0
              << ", v = " << p.v0 << ", c = " << p.c0
              << ", lr = " << p.lr << "\n\n";

    wb::BackpropResult res = wb::backprop_train(p);

    const auto& s0 = res.steps[0];
    std::cout << "Step 0 (Initial State):\n";
    std::cout << "  h      = " << std::fixed << std::setprecision(10) << s0.h << "\n";
    std::cout << "  y_hat  = " << s0.y_hat << "\n";
    std::cout << "  loss   = " << s0.loss << "\n";
    std::cout << "  dJ/dw  = " << s0.dw << "\n";
    std::cout << "  dJ/db  = " << s0.db << "\n";
    std::cout << "  dJ/dv  = " << s0.dv << "\n";
    std::cout << "  dJ/dc  = " << s0.dc << "\n";
    std::cout << "Central Finite Difference Check (eps = 1e-6):\n";
    std::cout << "  Max Error: " << std::scientific << res.max_grad_check_error
              << " (" << (res.grad_check_passed ? "PASSED <= 1e-5" : "FAILED") << ")\n\n";

    const auto& s1 = res.steps[1];
    std::cout << "Step 1 (After First Update):\n";
    std::cout << "  w      = " << std::fixed << std::setprecision(10) << s1.w << "\n";
    std::cout << "  b      = " << s1.b << "\n";
    std::cout << "  v      = " << s1.v << "\n";
    std::cout << "  c      = " << s1.c << "\n";
    std::cout << "  loss   = " << s1.loss << "\n\n";

    const auto& s100 = res.steps[100];
    std::cout << "Step 100 (Final State):\n";
    std::cout << "  loss   = " << std::scientific << s100.loss << "\n";
    std::cout << "  y_hat  = " << std::fixed << std::setprecision(10) << s100.y_hat
              << " (target y = 2.0000000000)\n\n";

    std::filesystem::create_directories("results");
    wb::export_backprop_loss(res, "results/backprop_loss.csv");
    std::cout << "Exported training trajectory to results/backprop_loss.csv\n";

    return 0;
}
