# Unified Engineering Workbench

**PASS Science Works — R&D Internship 2026–2027 Assessment Submission**  
**Candidate**: Kashyap Gupta  
**Evaluation Target**: Windows 11 x64  
**Primary Executable**: `Launch.exe`  

---

## 1. Quick Start: One-Click Execution Gate

### Standalone Launch (Zero Installation / No Terminal)
1. Extract `CandidateID_Workbench.zip` to any local directory on Windows 11 x64.
2. Double-click **`Launch.exe`** in the extracted root directory.
3. The native Direct3D 11 desktop workbench opens immediately.
   - **No Python, node, or CMake required.**
   - **No administrator rights required.**
   - **No internet connection or package installation required.**
   - Statically linked with the MSVC runtime (`/MT`) and utilizes standard Windows DirectX 11 runtime drivers with automatic WARP software fallback.

### VS Code / Antigravity One-Click Launch
1. Open this repository directory in VS Code or Google Antigravity.
2. Navigate to **Run and Debug** (`Ctrl+Shift+D`).
3. Select **"Launch Workbench (Standalone)"** and press **F5**.

---

## 2. System Architecture & Separation of Concerns

The workbench is strictly engineered with clear decoupling between core scientific computing, transport interfaces, and presentation layers:

```text
+--------------------------------------------------------------------------+
|                            PRESENTATION LAYER                            |
|   Win32 / Direct3D 11 Native Desktop UI (Dear ImGui 1.90.4 + ImPlot 0.16) |
|   + Responsive Companion Web Interface (HTML5 / Vanilla CSS / ES6)     |
+--------------------------------------------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
|                 UI-INDEPENDENT C++17 CORE STATIC LIBRARY                 |
|                               (wb_core)                                  |
+------------------+-------------------+-----------------+-----------------+
|  fem1d           |  stl_parser       |  csv_parser     |  batch          |
|  - 1D FEA rod    |  - ASCII STL      |  - ANSYS nodal  |  - Multicore    |
|  - Thomas solver |  - Degeneracy     |  - Statistics   |  - Thread pool  |
|  - Error norm    |  - Bounds check   |  - Validation   |  - Benchmark    |
+------------------+-------------------+-----------------+-----------------+
|  devices         |  backprop         |  sysinfo        |  export         |
|  - MCU mock      |  - Analytical dJ  |  - Windows API  |  - CSV exporter |
|  - FPGA mock     |  - Finite diff    |  - CPU / RAM    |  - Convergence  |
|  - JTAG debug    |  - 100-step loop  |  - OS / flags   |  - Benchmark    |
+------------------+-------------------+-----------------+-----------------+
```

---

## 3. Workbench Views & Capabilities

### View 1: Geometry View (CAD / ASCII STL)
- Imports and validates standard ASCII `.stl` surface geometry meshes.
- Enforces strict limits (max 200 MiB, max 2,000,000 triangles).
- Actionable diagnostics: line-specific syntax errors, non-finite values (`NaN`, `Inf`), degenerate triangles (area $< 10^{-12}$).
- Real-time 3D interactive viewport: Pitch/Yaw rotation via mouse drag, Zoom via scroll wheel, Reset Camera, and 3D coordinate axis gizmo.

### View 2: Thermal Analysis View
- **1D Steady-State Thermal Finite Element Solver**:
  - Solves $-d/dx(kA dT/dx) = qA$ on a rod of length $L = 1.0$ m, $A = 10^{-4}$ m², $k = 10.0$ W/(m·K), $q = 10,000$ W/m³, $T(0) = 100$ °C, $T(L) = 20$ °C.
  - Implements the Thomas tridiagonal algorithm with exact Dirichlet boundary condition substitution.
  - Linear element midpoint error evaluation verifies exact second-order $O(h^2)$ convergence.
  - Observed convergence order: **2.0000**.
  - Interactive ImPlot comparison between FE solution and analytical quadratic curve $T_{\text{exact}}(x)$.
- **ANSYS / CSV 3D Nodal Temperature Results**:
  - Imports solver CSV with schema `node_id,x_m,y_m,z_m,temperature_C`.
  - Rejects missing headers, malformed rows, and duplicate IDs.
  - Renders interactive 3D colormapped point cloud with calibrated temperature legend bar and node inspection statistics ($T_{\text{min}}$, $T_{\text{max}}$, $T_{\text{mean}}$).

### View 3: Performance View (Multicore Batch Solver)
- Benchmarks batch of independent thermal solves with configurable case count, mesh size, and random seed.
- Executes across 1, 2, and 4 worker threads using a bounded thread pool and atomic work stealing.
- Captures genuine system provenance: CPU model, cores, clock, RAM, OS, compiler, optimization flags (`/O2`).
- Timing protocol: 1 warm-up run + 5 timed repetitions per configuration; reports median runtime, speedup $T_1/T_p$, and efficiency.
- Verified strict serial/parallel numerical agreement: max temperature difference $\le 10^{-8}$ °C.

### View 4: Hardware View (Embedded Subsystems)
- Modular host-connected topology for three discrete entities:
  - **STM32F407 MCU**: ARM Cortex-M4, 168 MHz, 3.3V IO, USB-UART bridge at 115200 8N1.
  - **Xilinx Artix-7 FPGA**: 1.0V core, 3.3V Bank 34 IO, synthetic telemetry stream.
  - **SEGGER J-Link JTAG Probe**: Specialized debug adapter tracking target `VTref` (1.65–3.6V).
- **Fault Recovery Demonstration**: Injects 2 consecutive connection timeouts; demonstrates automatic recovery on attempt 3 via bounded retry loop.
- All simulated hardware panels clearly display `[SIMULATED]` indicators.

### View 5: Backpropagation Inspector
- Neural network regression neuron: $h = \tanh(wx + b)$, $\hat{y} = vh + c$, $J = \frac{1}{2}(\hat{y} - y)^2$.
- Evaluates analytical gradients vs central finite differences ($\varepsilon = 10^{-6}$): absolute error $1.13 \times 10^{-10} \le 10^{-5}$ [PASSED].
- 100-step gradient descent updates parameters simultaneously; loss decreases monotonically from $1.182542$ to $1.70 \times 10^{-20}$.

---

## 4. Verification and Automated Test Suite

An automated test suite covering all numerical solvers, parsers, concurrency paths, hardware adapters, and backpropagation models is included:

```powershell
# Run the complete test suite
.\build\wb_tests.exe
```

### Verified Test Summary (21 / 21 Tests Passing):
1. **FEM Analytical Verification & Convergence**:
   - $N=10$ ($h=0.1000$ m): Midpoint Error = $1.25000000$ °C (Ref: $1.25000000$ °C) $\to$ **PASS**
   - $N=20$ ($h=0.0500$ m): Midpoint Error = $0.31250000$ °C (Ref: $0.31250000$ °C) $\to$ **PASS**
   - $N=40$ ($h=0.0250$ m): Midpoint Error = $0.07812500$ °C (Ref: $0.07812500$ °C) $\to$ **PASS**
   - $N=80$ ($h=0.0125$ m): Midpoint Error = $0.01953125$ °C (Ref: $0.01953125$ °C) $\to$ **PASS**
   - Observed Convergence Order: **2.0000** $\to$ **PASS**
   - Parameter Validation ($N=0$, $k < 0$) $\to$ **PASS**
2. **ASCII STL Parser Tests**:
   - Valid geometry parsing (`sample_unit_cube.stl`, 12 triangles) $\to$ **PASS**
   - Malformed line recovery (`test_malformed.stl`) $\to$ **PASS**
   - Empty file rejection (`test_empty.stl`) $\to$ **PASS**
3. **Temperature CSV Parser Tests**:
   - Valid 216-node synthetic grid dataset $\to$ **PASS**
   - Missing column rejection (`test_missing_column.csv`) $\to$ **PASS**
   - Non-finite value rejection (`NaN`, `Inf`) $\to$ **PASS**
   - Duplicate ID detection $\to$ **PASS**
4. **Multicore Concurrency & Numerical Consistency**:
   - Serial vs Parallel numerical agreement ($\le 10^{-8}$ °C diff) $\to$ **PASS**
5. **Embedded Mocks & Fault Recovery**:
   - Device discovery and capability reporting $\to$ **PASS**
   - 2 injected connection timeouts recovered on attempt 3 $\to$ **PASS**
6. **Backpropagation Verification**:
   - Analytical gradient vs central finite differences ($1.13 \times 10^{-10} \le 10^{-5}$) $\to$ **PASS**
   - Monotonic 100-step training loss reduction $\to$ **PASS**

---

## 5. Source Build Instructions

To build from source on Windows 11 x64:

### Prerequisites
- CMake 3.20 or newer
- Microsoft Visual Studio 2022 (MSVC v143 or BuildTools) with C++17 support
- Windows 10/11 SDK

### Build Commands
```powershell
# Open Developer Command Prompt for VS 2022 (or run vcvars64.bat)
cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run verification tests
.\build\wb_tests.exe

# Run standalone CLI tools
.\build\wb_fem.exe
.\build\wb_bench.exe
.\build\wb_backprop.exe

# Start desktop application
.\build\Launch.exe
```

---

## 6. Project Directory Structure

```text
Pass Science/
├── Launch.exe                       # Standalone Windows 11 application
├── CMakeLists.txt                   # Master CMake build definition
├── README.md                        # Primary user & technical guide
├── .gitignore                       # Clean repository exclusions
├── .vscode/                         # Editor run & task configurations
│   ├── launch.json
│   └── tasks.json
├── include/wb/                      # Public C++17 library API headers
│   ├── backprop.h
│   ├── batch.h
│   ├── csv_parser.h
│   ├── devices.h
│   ├── export.h
│   ├── fem1d.h
│   ├── stl_parser.h
│   └── sysinfo.h
├── src/
│   ├── core/                        # UI-independent C++17 static library (wb_core)
│   │   ├── backprop.cpp
│   │   ├── batch.cpp
│   │   ├── csv_parser.cpp
│   │   ├── devices.cpp
│   │   ├── export.cpp
│   │   ├── fem1d.cpp
│   │   ├── stl_parser.cpp
│   │   └── sysinfo.cpp
│   └── app/                         # Application frontends & CLI tools
│       ├── main.cpp                 # Win32 + Direct3D 11 + Dear ImGui desktop workbench
│       ├── cli_fem.cpp              # Standalone FEM solver CLI
│       ├── cli_bench.cpp            # Standalone multicore benchmark CLI
│       └── cli_backprop.cpp         # Standalone backprop verification CLI
├── tests/
│   └── test_main.cpp                # 21-test automated verification suite
├── data/                            # Sample & test fixtures with provenance
│   ├── PROVENANCE.md                # Strict data provenance documentation
│   ├── sample_unit_cube.stl         # Synthetic unit cube ASCII STL
│   ├── sample_temperature_synthetic.csv # Synthetic 216-node CSV
│   ├── test_malformed.stl
│   ├── test_empty.stl
│   ├── test_missing_column.csv
│   ├── test_nan_values.csv
│   └── test_duplicate_ids.csv
├── results/                         # Verified numerical outputs & exported CSVs
│   ├── fem_N10.csv
│   ├── fem_N20.csv
│   ├── fem_N40.csv
│   ├── fem_N80.csv
│   ├── fem_convergence.csv
│   ├── benchmark_results.csv
│   └── backprop_loss.csv
├── docs/                            # Formal engineering documentation
│   ├── ASSIGNMENT_REFERENCE.md      # Official prompt & rubric reference
│   ├── FILE_FORMATS.md              # Supported file format specifications
│   ├── EMBEDDED_DESIGN.md           # Embedded topology & pinout table
│   ├── BACKPROP_DERIVATION.md       # First-principles handwritten appendix derivation
│   ├── AI_USE_LOG.md                # AI use log with challenge tests & corrections
│   ├── OWNERSHIP.md                 # Original work statement & integrity pledge
│   ├── LICENSES.md                  # Third-party dependency notices
│   └── SCREENSHOT_WALKTHROUGH.md    # Timestamped walkthrough & logs
├── third_party/                     # Permissive vendored UI dependencies
│   ├── imgui/                       # Dear ImGui (MIT)
│   └── implot/                      # ImPlot (MIT)
└── website/                         # Complementary engineering product web interface
    └── index.html
```

---

## 7. Known Limitations and Scope Boundaries

1. **CAD Boundary Representation**: The geometry viewer supports standard 3D ASCII triangulated surface meshes (`.stl`). Native parametric B-Rep solids (STEP, IGES, SLDPRT) require heavyweight commercial geometric kernels and are explicitly out of scope.
2. **Thermal Physical Scope**: The built-in FEM solver implements the 1D steady-state rod heat conduction equation. Visualizing an imported 3D STL mesh or 3D point cloud represents geometric and imported field inspection, not a 3D volumetric thermal solve of the imported CAD solid.
3. **Hardware Simulation**: Microcontroller, FPGA, and JTAG communication layers are implemented as deterministic software mock adapters. Electrical voltage levels, current budgets, and level-shifting requirements are documented as engineering design assumptions to be validated against physical hardware documentation.

