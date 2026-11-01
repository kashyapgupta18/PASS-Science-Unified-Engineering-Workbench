// =============================================================================
// backprop.h — Backpropagation Neural Network Demonstration
// Unified Engineering Workbench
// =============================================================================
// Implements a minimal single-neuron network with tanh activation:
//   h = tanh(w*x + b), y_hat = v*h + c, J = 0.5*(y_hat - y)²
// Demonstrates analytical gradient computation and finite-difference checking.
// =============================================================================

#ifndef WB_BACKPROP_H
#define WB_BACKPROP_H

#include <string>
#include <vector>

namespace wb {

/// One step of the backpropagation training process.
struct BackpropStep {
    double w;      ///< Weight w
    double b;      ///< Bias b
    double v;      ///< Weight v
    double c;      ///< Bias c
    double h;      ///< Hidden unit activation: tanh(w*x + b)
    double y_hat;  ///< Network output: v*h + c
    double loss;   ///< Loss: 0.5*(y_hat - y)²
    double dw;     ///< Gradient dJ/dw
    double db;     ///< Gradient dJ/db
    double dv;     ///< Gradient dJ/dv
    double dc;     ///< Gradient dJ/dc
};

/// Complete result of the backpropagation training run.
struct BackpropResult {
    std::vector<BackpropStep> steps;  ///< 101 steps: initial state + 100 updates
    double max_grad_check_error;      ///< Maximum |analytical - numerical| gradient error
    bool   grad_check_passed;         ///< True if max error <= 1e-5
};

/// Parameters for the backpropagation demonstration.
struct BackpropParams {
    double x  = 1.0;    ///< Input value
    double y  = 2.0;    ///< Target value
    double w0 = 0.5;    ///< Initial weight w
    double b0 = 0.0;    ///< Initial bias b
    double v0 = 1.0;    ///< Initial weight v
    double c0 = 0.0;    ///< Initial bias c
    double lr = 0.1;    ///< Learning rate
    int    num_steps = 100;  ///< Number of training steps
    double grad_check_eps = 1e-7;  ///< Epsilon for finite-difference gradient check
};

/// Run the backpropagation training loop and gradient verification.
/// @param params  Network and training parameters
/// @return        BackpropResult with training history and gradient check
BackpropResult backprop_train(const BackpropParams& params = {});

/// Compute loss for given parameters (used in gradient checking).
/// @param w  Weight w
/// @param b  Bias b
/// @param v  Weight v
/// @param c  Bias c
/// @param x  Input value
/// @param y  Target value
/// @return   Loss J = 0.5*(y_hat - y)²
double backprop_loss(double w, double b, double v, double c,
                     double x, double y);

}  // namespace wb

#endif  // WB_BACKPROP_H
