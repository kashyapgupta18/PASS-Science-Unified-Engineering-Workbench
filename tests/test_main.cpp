// =============================================================================
// test_main.cpp — Comprehensive Automated Test Suite
// Unified Engineering Workbench
// =============================================================================

#include "wb/backprop.h"
#include "wb/batch.h"
#include "wb/csv_parser.h"
#include "wb/devices.h"
#include "wb/export.h"
#include "wb/fem1d.h"
#include "wb/stl_parser.h"
#include "wb/sysinfo.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            g_tests_failed++; \
            return; \
        } \
    } while (0)

#define TEST_PASS(msg) \
    do { \
        std::cout << "  [PASS] " << msg << "\n"; \
        g_tests_passed++; \
    } while (0)

// -----------------------------------------------------------------------------
// Suite 1: 1D Thermal FEM Tests
// -----------------------------------------------------------------------------
void test_fem_analytical_and_convergence() {
    std::cout << "\n=== Running Suite 1: Thermal FEM Verification ===\n";

    wb::FemParams p;
    p.L = 1.0;
    p.A = 1e-4;
    p.k = 10.0;
    p.q = 1e4;
    p.T0 = 100.0;
    p.TL = 20.0;

    // Check N = 10, 20, 40, 80
    struct Expected {
        int N;
        double h;
        double expected_mid_err;
    } cases[] = {
        {10, 0.1, 1.25},
        {20, 0.05, 0.3125},
        {40, 0.025, 0.078125},
        {80, 0.0125, 0.01953125}
    };

    std::vector<wb::FemResult> results;

    for (const auto& c : cases) {
        p.N = c.N;
        wb::FemResult r = wb::fem1d_solve(p);
        TEST_ASSERT(r.success, "FEM solve failed for N=" + std::to_string(c.N));

        // Midpoint error verification against analytical reference: q*h^2/(8*k)
        double diff = std::abs(r.max_midpoint_error - c.expected_mid_err);
        TEST_ASSERT(diff < 1e-6, "Midpoint error mismatch for N=" + std::to_string(c.N));

        // Nodal error verification: should match exact quadratic up to floating-point round-off
        TEST_ASSERT(r.max_nodal_error < 1e-11, "Nodal error exceeded round-off tolerance for N=" + std::to_string(c.N));

        results.push_back(r);
        TEST_PASS("Mesh N=" + std::to_string(c.N) + " (h=" + std::to_string(c.h) +
                  "m): Midpoint Error = " + std::to_string(r.max_midpoint_error) +
                  " C (Ref: " + std::to_string(c.expected_mid_err) +
                  " C), Nodal Error = " + std::to_string(r.max_nodal_error) + " C");
    }

    // Verify convergence order: log(e1/e2) / log(h1/h2) between N=10 and N=80
    double order = wb::fem1d_convergence_order(results[0].h, results[0].max_midpoint_error,
                                              results[3].h, results[3].max_midpoint_error);
    TEST_ASSERT(std::abs(order - 2.0) < 0.001, "Convergence order is not 2.0 (got " + std::to_string(order) + ")");
    TEST_PASS("Observed convergence order: " + std::to_string(order) + " (theoretical: 2.0000)");

    // Test parameter validation (negative parameters)
    wb::FemParams bad = p;
    bad.N = 0;
    wb::FemResult bad_res = wb::fem1d_solve(bad);
    TEST_ASSERT(!bad_res.success, "Solver should fail for N=0");
    TEST_PASS("Rejected invalid mesh size N=0: " + bad_res.error_message);

    bad = p;
    bad.k = -5.0;
    bad_res = wb::fem1d_solve(bad);
    TEST_ASSERT(!bad_res.success, "Solver should fail for negative k");
    TEST_PASS("Rejected negative conductivity k=-5.0");
}

// -----------------------------------------------------------------------------
// Suite 2: ASCII STL Parser Tests
// -----------------------------------------------------------------------------
void test_stl_parser() {
    std::cout << "\n=== Running Suite 2: ASCII STL Parser Tests ===\n";

    // 1. Valid file
    wb::StlData stl = wb::stl_parse_file("data/sample_unit_cube.stl");
    TEST_ASSERT(stl.success, "Failed to parse data/sample_unit_cube.stl: " + stl.error_message);
    TEST_ASSERT(stl.triangles.size() == 12, "Expected 12 triangles for unit cube, got " + std::to_string(stl.triangles.size()));
    TEST_ASSERT(std::abs(stl.bounds_min[0] - 0.0f) < 1e-4f, "Bounds X min mismatch");
    TEST_ASSERT(std::abs(stl.bounds_max[0] - 1.0f) < 1e-4f, "Bounds X max mismatch");
    TEST_PASS("Parsed sample_unit_cube.stl successfully (12 triangles, bounds [0, 1])");

    // 2. Malformed file
    wb::StlData mal = wb::stl_parse_file("data/test_malformed.stl");
    TEST_ASSERT(mal.error_count > 0, "Malformed STL should report errors");
    TEST_PASS("Correctly flagged malformed STL file with " + std::to_string(mal.error_count) + " errors");

    // 3. Empty geometry
    wb::StlData emp = wb::stl_parse_file("data/test_empty.stl");
    TEST_ASSERT(!emp.success, "Empty STL should fail");
    TEST_PASS("Correctly rejected empty STL file: " + emp.error_message);
}

// -----------------------------------------------------------------------------
// Suite 3: Temperature CSV Parser Tests
// -----------------------------------------------------------------------------
void test_csv_parser() {
    std::cout << "\n=== Running Suite 3: Temperature CSV Parser Tests ===\n";

    // 1. Valid file
    wb::CsvData csv = wb::csv_parse_file("data/sample_temperature_synthetic.csv");
    TEST_ASSERT(csv.success, "Failed to parse sample_temperature_synthetic.csv: " + csv.error_message);
    TEST_ASSERT(csv.nodes.size() == 216, "Expected 216 nodes, got " + std::to_string(csv.nodes.size()));
    TEST_ASSERT(std::abs(csv.temp_min - 20.0) < 1e-4, "Min temp mismatch (expected 20.0)");
    TEST_ASSERT(std::abs(csv.temp_max - 130.0) < 1e-4, "Max temp mismatch (expected 130.0)");
    TEST_ASSERT(std::abs(csv.temp_mean - 67.5) < 1e-4, "Mean temp mismatch (expected 67.5)");
    TEST_PASS("Parsed sample_temperature_synthetic.csv (216 nodes, min=20.0 C, max=130.0 C, mean=67.5 C)");

    // 2. Missing column
    wb::CsvData bad_header = wb::csv_parse_file("data/test_missing_column.csv");
    TEST_ASSERT(!bad_header.success, "CSV with missing columns should fail header validation");
    TEST_PASS("Correctly rejected missing column: " + bad_header.error_message);

    // 3. NaN / Inf values
    wb::CsvData nan_csv = wb::csv_parse_file("data/test_nan_values.csv");
    TEST_ASSERT(nan_csv.error_count > 0, "CSV with NaN/Inf should report errors");
    TEST_PASS("Correctly detected non-finite values (NaN/Inf) in CSV");

    // 4. Duplicate IDs
    wb::CsvData dup_csv = wb::csv_parse_file("data/test_duplicate_ids.csv");
    TEST_ASSERT(dup_csv.error_count > 0, "CSV with duplicate IDs should report warnings");
    TEST_PASS("Correctly detected duplicate node_id in CSV");
}

// -----------------------------------------------------------------------------
// Suite 4: Parallel Batch Performance and Agreement Check
// -----------------------------------------------------------------------------
void test_batch_performance() {
    std::cout << "\n=== Running Suite 4: Multicore Batch Performance & Agreement ===\n";

    wb::BatchConfig cfg;
    cfg.case_count = 100;
    cfg.mesh_size = 40;
    cfg.seed = 12345;

    // Run serial
    std::vector<double> serial_res = wb::batch_solve(cfg, 1);
    TEST_ASSERT(serial_res.size() == 100, "Serial solve failed");

    // Run parallel with 2 and 4 workers
    std::vector<double> par2_res = wb::batch_solve(cfg, 2);
    TEST_ASSERT(par2_res.size() == 100, "Parallel (2 workers) solve failed");

    std::vector<double> par4_res = wb::batch_solve(cfg, 4);
    TEST_ASSERT(par4_res.size() == 100, "Parallel (4 workers) solve failed");

    // Check numerical agreement <= 1e-8 C
    double max_diff2 = 0.0;
    double max_diff4 = 0.0;
    for (size_t i = 0; i < 100; ++i) {
        max_diff2 = std::max(max_diff2, std::abs(serial_res[i] - par2_res[i]));
        max_diff4 = std::max(max_diff4, std::abs(serial_res[i] - par4_res[i]));
    }

    TEST_ASSERT(max_diff2 <= 1e-8, "Agreement check failed for 2 workers (diff=" + std::to_string(max_diff2) + ")");
    TEST_ASSERT(max_diff4 <= 1e-8, "Agreement check failed for 4 workers (diff=" + std::to_string(max_diff4) + ")");
    TEST_PASS("Serial/Parallel numerical agreement verified: max diff (2 workers) = " +
              std::to_string(max_diff2) + " C, max diff (4 workers) = " +
              std::to_string(max_diff4) + " C (<= 1e-8 C tolerance)");
}

// -----------------------------------------------------------------------------
// Suite 5: Embedded Device Mocks & Fault Recovery
// -----------------------------------------------------------------------------
void test_devices_and_recovery() {
    std::cout << "\n=== Running Suite 5: Embedded Device Mocks & Fault Recovery ===\n";

    wb::DeviceAdapter mcu = wb::create_mcu_device();
    wb::DeviceAdapter fpga = wb::create_fpga_device();
    wb::DeviceAdapter jtag = wb::create_jtag_device();

    // Verify types and capabilities
    TEST_ASSERT(mcu.type() == wb::DeviceType::MCU, "MCU type mismatch");
    TEST_ASSERT(fpga.type() == wb::DeviceType::FPGA, "FPGA type mismatch");
    TEST_ASSERT(jtag.type() == wb::DeviceType::JTAG, "JTAG type mismatch");
    TEST_ASSERT(!mcu.capabilities().empty(), "MCU capabilities empty");
    TEST_PASS("Device discovery verified for MCU, FPGA, and JTAG probe");

    // Normal connection and telemetry
    TEST_ASSERT(mcu.connect(), "Failed to connect to MCU");
    TEST_ASSERT(mcu.is_connected(), "MCU should be connected");
    wb::TelemetryMessage msg = mcu.read_telemetry();
    TEST_ASSERT(!msg.payload_json.empty(), "MCU telemetry payload empty");
    TEST_PASS("MCU connection and telemetry verified: " + msg.payload_json);
    mcu.disconnect();
    TEST_ASSERT(!mcu.is_connected(), "MCU should be disconnected");

    // Recoverable failure test: inject 2 connection timeouts, then connect_with_retry succeeds on attempt 3
    wb::DeviceAdapter recovering_device = wb::create_fpga_device();
    recovering_device.inject_connect_fault(2);  // First 2 calls to connect() will fail

    // Direct single connect should fail
    bool direct_conn = recovering_device.connect();
    TEST_ASSERT(!direct_conn, "Injected fault should cause connection failure");
    TEST_PASS("Injected timeout failure reproduced on attempt 1");

    // Reset and test connect_with_retry(3)
    recovering_device.inject_connect_fault(2);
    bool retry_conn = recovering_device.connect_with_retry(3, 100);
    TEST_ASSERT(retry_conn, "connect_with_retry should succeed on attempt 3");
    TEST_ASSERT(recovering_device.is_connected(), "Device should now be connected");
    TEST_PASS("Fault recovery verified: two injected timeouts recovered by bounded retry loop");
}

// -----------------------------------------------------------------------------
// Suite 6: Backpropagation Verification
// -----------------------------------------------------------------------------
void test_backpropagation() {
    std::cout << "\n=== Running Suite 6: Backpropagation & Finite Differences ===\n";

    wb::BackpropParams bp;
    bp.x = 1.0;
    bp.y = 2.0;
    bp.w0 = 0.5;
    bp.b0 = 0.0;
    bp.v0 = 1.0;
    bp.c0 = 0.0;
    bp.lr = 0.1;
    bp.num_steps = 100;
    bp.grad_check_eps = 1e-6;

    wb::BackpropResult res = wb::backprop_train(bp);

    // Initial values
    const auto& s0 = res.steps[0];
    double h0_expected = std::tanh(0.5);  // ~0.462117
    TEST_ASSERT(std::abs(s0.h - h0_expected) < 1e-5, "Initial h mismatch");
    TEST_ASSERT(std::abs(s0.y_hat - h0_expected) < 1e-5, "Initial y_hat mismatch");

    // Gradient check
    TEST_ASSERT(res.grad_check_passed, "Backprop gradient check failed");
    TEST_ASSERT(res.max_grad_check_error <= 1e-5, "Gradient check error exceeded 1e-5");
    TEST_PASS("Finite-difference gradient check passed: max error = " +
              std::to_string(res.max_grad_check_error) + " (<= 1e-5 tolerance)");

    // Monotonic loss reduction
    TEST_ASSERT(res.steps.size() == 101, "Expected 101 steps");
    double initial_loss = res.steps[0].loss;
    double final_loss = res.steps[100].loss;
    TEST_ASSERT(final_loss < initial_loss, "Training should reduce loss");
    TEST_PASS("100-step gradient descent verified: initial loss = " +
              std::to_string(initial_loss) + " -> final loss = " +
              std::to_string(final_loss));
}

// -----------------------------------------------------------------------------
// Main Test Runner
// -----------------------------------------------------------------------------
int main() {
    std::cout << "===================================================================\n";
    std::cout << "  UNIFIED ENGINEERING WORKBENCH — AUTOMATED VERIFICATION SUITE    \n";
    std::cout << "===================================================================\n";

    test_fem_analytical_and_convergence();
    test_stl_parser();
    test_csv_parser();
    test_batch_performance();
    test_devices_and_recovery();
    test_backpropagation();

    std::cout << "\n===================================================================\n";
    std::cout << "  SUMMARY: " << g_tests_passed << " Passed, "
              << g_tests_failed << " Failed\n";
    std::cout << "===================================================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
