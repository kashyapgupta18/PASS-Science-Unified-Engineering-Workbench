// =============================================================================
// backprop.cpp — Backpropagation Implementation
// Unified Engineering Workbench
// =============================================================================

#include "wb/backprop.h"

#include <cmath>
#include <algorithm>

namespace wb {

// ---------------------------------------------------------------------------
// Compute loss for given parameters
// ---------------------------------------------------------------------------
double backprop_loss(double w, double b, double v, double c,
                     double x, double y) {
    double h = std::tanh(w * x + b);
    double y_hat = v * h + c;
    return 0.5 * (y_hat - y) * (y_hat - y);
}

// ---------------------------------------------------------------------------
// Run backpropagation training with gradient verification
// ---------------------------------------------------------------------------
BackpropResult backprop_train(const BackpropParams& params) {
    BackpropResult result;
    result.grad_check_passed = true;
    result.max_grad_check_error = 0.0;

    double w = params.w0;
    double b = params.b0;
    double v = params.v0;
    double c = params.c0;
    const double x = params.x;
    const double y = params.y;
    const double lr = params.lr;
    const double eps = params.grad_check_eps;

    for (int step = 0; step <= params.num_steps; ++step) {
        BackpropStep s;
        s.w = w; s.b = b; s.v = v; s.c = c;

        // --- Forward pass ---------------------------------------------------
        s.h = std::tanh(w * x + b);
        s.y_hat = v * s.h + c;
        s.loss = 0.5 * (s.y_hat - y) * (s.y_hat - y);

        // --- Analytical gradients (chain rule) ------------------------------
        // dJ/dy_hat = (y_hat - y)
        double dJ_dyhat = s.y_hat - y;
        // dJ/dc = dJ/dy_hat * dy_hat/dc = (y_hat - y) * 1
        s.dc = dJ_dyhat;
        // dJ/dv = dJ/dy_hat * dy_hat/dv = (y_hat - y) * h
        s.dv = dJ_dyhat * s.h;
        // dJ/db = dJ/dy_hat * dy_hat/dh * dh/db
        //       = (y_hat - y) * v * (1 - h²)
        double sech2 = 1.0 - s.h * s.h;
        s.db = dJ_dyhat * v * sech2;
        // dJ/dw = dJ/db * x = (y_hat - y) * v * (1 - h²) * x
        s.dw = s.db * x;

        // --- Finite-difference gradient check (initial step only) -----------
        if (step == 0) {
            // Central differences: df/dp ≈ (f(p+eps) - f(p-eps)) / (2*eps)
            double num_dw = (backprop_loss(w + eps, b, v, c, x, y) -
                            backprop_loss(w - eps, b, v, c, x, y)) / (2.0 * eps);
            double num_db = (backprop_loss(w, b + eps, v, c, x, y) -
                            backprop_loss(w, b - eps, v, c, x, y)) / (2.0 * eps);
            double num_dv = (backprop_loss(w, b, v + eps, c, x, y) -
                            backprop_loss(w, b, v - eps, c, x, y)) / (2.0 * eps);
            double num_dc = (backprop_loss(w, b, v, c + eps, x, y) -
                            backprop_loss(w, b, v, c - eps, x, y)) / (2.0 * eps);

            double err_w = std::abs(s.dw - num_dw);
            double err_b = std::abs(s.db - num_db);
            double err_v = std::abs(s.dv - num_dv);
            double err_c = std::abs(s.dc - num_dc);

            result.max_grad_check_error = std::max({err_w, err_b, err_v, err_c});
            if (result.max_grad_check_error > 1e-5) {
                result.grad_check_passed = false;
            }
        }

        result.steps.push_back(s);

        // --- Simultaneous gradient descent update ---------------------------
        // Use gradients computed from current parameters (before update)
        if (step < params.num_steps) {
            w -= lr * s.dw;
            b -= lr * s.db;
            v -= lr * s.dv;
            c -= lr * s.dc;
        }
    }

    return result;
}

}  // namespace wb

