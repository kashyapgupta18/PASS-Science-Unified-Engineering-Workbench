# Unified Engineering Workbench: Technical Report

**PASS Science Works R&D Internship 2026–2027 Assessment**  
**Candidate**: Kashyap Gupta  
**Date**: November 1, 2026  
**Target Architecture**: Windows 11 x64  

---

## 1. Overview and System Architecture
The Unified Engineering Workbench is a high-reliability, multi-domain desktop engineering application built for Windows 11 x64 in modern C++17. The system strictly separates core computation, numerical algorithms, and data ingestion into an independent static library (`wb_core`). The graphical interface is implemented with Win32 and Direct3D 11 utilizing Dear ImGui and ImPlot, providing high-framerate interactive 3D rendering and real-time plotting without external web runtimes, interpreters, or heavyweight framework dependencies.

### Core Architecture Modules:
1. **`fem1d`**: Steady-state 1D thermal finite element solver with Thomas algorithm tridiagonal elimination.
2. **`io`**: Robust ASCII STL surface mesh and ANSYS-schema CSV parsers with line-specific validation and fault recovery.
3. **`batch`**: Bounded worker thread pool for serial and CPU-parallel execution with deterministic result ordering.
4. **`backprop`**: First-principles neural network training and central finite-difference gradient verification.
5. **`devices`**: Deterministic software mock adapters for MCU, FPGA, and JTAG probe subsystems.
6. **`sysinfo`**: Native Windows API hardware provenance query.

---

## 2. Data Provenance and Real-Data Support
All bundled demonstration data is self-created, openly documented, and synthetic:
- **`sample_unit_cube.stl`**: Contains 12 triangles hand-authored to define a unit cube $[0,1]^3$ in space.
- **`sample_temperature_synthetic.csv`**: Comprises 216 nodes on a $6 \times 6 \times 6$ Cartesian grid with coordinates in metres and temperatures defined by $T(x,y,z) = 20 + 80x + 30yz$ °C (labelled `[SYNTHETIC]` throughout UI and documentation).

The application strictly differentiates imported CAD geometry visualization, imported temperature fields, and the 1D physical FEM solver. File ingestion enforces 200 MiB size limits, malformed line reporting with line numbers, non-finite value rejection (`NaN`, `Inf`), and non-blocking asynchronous cancellation.

---

## 3. Thermal Finite Element Analysis & Verification
The thermal solver models steady-state conduction in a uniform rod: $-d/dx(kA dT/dx) = qA$ with $L = 1.0$ m, $A = 10^{-4}$ m², $k = 10.0$ W/(m·K), volumetric heat source $q = 10,000$ W/m³, and Dirichlet boundary conditions $T(0) = 100.0$ °C, $T(L) = 20.0$ °C.

The exact analytical solution is quadratic:
$$T_{\text{exact}}(x) = 100 - 80\frac{x}{L} + \frac{q}{2k}x(L-x)$$

Using two-node linear elements, the element stiffness matrix is $k_e = \frac{kA}{h}\begin{bmatrix} 1 & -1 \\ -1 & 1 \end{bmatrix}$ and element load vector is $f_e = \frac{qAh}{2}\begin{bmatrix} 1 \\ 1 \end{bmatrix}$. Tridiagonal global assembly is solved via the Thomas algorithm.

### Mesh Convergence Verification Table

| Mesh Elements $N$ | Size $h$ [m] | Measured Midpoint Error [°C] | Theoretical $qh^2/(8k)$ [°C] | Nodal Roundoff Error [°C] | Status |
|---|---|---|---|---|---|
| 10 | 0.1000 | 1.25000000 | 1.25000000 | $2.84 \times 10^{-14}$ | **PASSED** |
| 20 | 0.0500 | 0.31250000 | 0.31250000 | $1.99 \times 10^{-13}$ | **PASSED** |
| 40 | 0.0250 | 0.07812500 | 0.07812500 | $8.81 \times 10^{-13}$ | **PASSED** |
| 80 | 0.0125 | 0.01953125 | 0.01953125 | $1.22 \times 10^{-12}$ | **PASSED** |

**Observed convergence order is exactly 2.0000**. Because the exact solution is quadratic, linear elements reproduce it identically at the mesh nodes up to machine round-off ($< 1.3 \times 10^{-12}$ °C). Discretization error is properly evaluated at element midpoints where the FE interpolant deviates from the parabolic curve by exactly $\frac{qh^2}{8k}$.

---

## 4. Multicore Performance & Concurrency Benchmarking
Multicore scaling was benchmarked on the target machine:
- **Processor**: 12th Gen Intel Core i5-1235U (10 physical cores / 12 logical processors)
- **RAM**: 7.7 GB
- **Operating System**: Windows 11 Home Single Language (Build 26300)
- **Compiler**: MSVC 19.44 (VS 2022 17.14+) Release `/O2`

The workload consists of a deterministic batch of 200 independent 1D thermal cases (`std::mt19937_64`, seed 42, $k \in [1,50]$ W/(m·K), $q \in [10^3, 10^5]$ W/m³, 80 elements/case). Both serial and parallel implementations execute identical numerical solvers. The parallel path employs a bounded `std::thread` worker pool with an atomic work-stealing counter.

### Measured Benchmark Results (1 Warm-up + 5 Timed Repetitions)

| Workers ($p$) | Median Time [ms] | Min Time [ms] | Max Time [ms] | Speedup ($T_1/T_p$) | Efficiency | Max Temp Diff [°C] |
|---|---|---|---|---|---|---|
| 1 (Serial) | 0.71 | 0.70 | 0.72 | 1.000× | 100.0% | 0.00 |
| 2 Workers | 0.70 | 0.60 | 0.89 | 1.014× | 50.7% | $4.61 \times 10^{-312}$ |
| 4 Workers | 0.71 | 0.69 | 0.85 | 0.998× | 25.0% | $4.61 \times 10^{-312}$ |

- **Agreement Check**: Largest temperature difference between serial and parallel runs across all 200 cases was $4.61 \times 10^{-312}$ °C, strictly passing the $\le 10^{-8}$ °C requirement.
- **Honest Systems Analysis**: The 1D Thomas solver is extremely lightweight (~3.5 µs per case). For total runtimes under 1 ms, OS thread synchronization, context-switching, and cache coherence overhead match compute duration, accurately illustrating the overhead boundary of parallel computing.

---

## 5. Embedded System Integration Design (Simulated)
The embedded integration architecture models a host PC connected via an external powered USB 2.0 hub (5.0V VBUS, 500mA budget) to three discrete subsystems:
1. **STM32F407 MCU**: ARM Cortex-M4 at 168 MHz, 3.3V IO, USB-UART bridge at 115200 8N1.
2. **Xilinx Artix-7 FPGA**: 1.0V VCCINT core, 3.3V Bank 34 IO, synthetic telemetry stream.
3. **SEGGER J-Link JTAG Probe**: Debug interface dynamically monitoring target `VTref` (1.65–3.6V).

### Why JTAG is Distinct:
A JTAG adapter is strictly an invasive debugging probe with no user logic; it dynamically senses target voltage `VTref`, drives `TCK`, `TMS`, and `TDI`, and reads `TDO` to halt, step, and flash the target processor.

### Demonstrated Fault Recovery:
The C++ mock subsystem implements deliberate timeout injection. In the Hardware View, a 2-timeout injection test triggers two failed connection attempts (logged as timeouts) followed by an automatic recovery on attempt 3 via bounded retry loop, successfully establishing telemetry streaming. All simulated panels carry visible `[SIMULATED]` badges.

---

## 6. Handwritten Backpropagation Verification
- **Model**: $h = \tanh(wx + b)$, $\hat{y} = vh + c$, loss $J = \frac{1}{2}(\hat{y} - y)^2$.
- **Sample**: $x = 1.0$, target $y = 2.0$.
- **Initial Conditions**: $w_0 = 0.5$, $b_0 = 0.0$, $v_0 = 1.0$, $c_0 = 0.0$, learning rate $\eta = 0.1$.
- **Forward Pass at $t=0$**: $h = \tanh(0.5) = 0.4621171573$, $\hat{y} = 0.4621171573$, initial loss $J_0 = 1.1825418190$.

### Analytical Gradients Derived via Multivariate Chain Rule:
- $\frac{\partial J}{\partial w} = \frac{\partial J}{\partial b} = (\hat{y} - y) \cdot v \cdot (1 - h^2) \cdot x = \mathbf{-1.2094644752}$
- $\frac{\partial J}{\partial v} = (\hat{y} - y) \cdot h = \mathbf{-0.7106820475}$
- $\frac{\partial J}{\partial c} = (\hat{y} - y) = \mathbf{-1.5378828427}$

Simultaneous parameter updates yield $w_1 = 0.6209464475$, $b_1 = 0.1209464475$, $v_1 = 1.0710682047$, $c_1 = 0.1537882843$, reducing loss to $J_1 = \mathbf{0.6857737650}$ in a single update. Central finite-difference check ($\varepsilon = 10^{-6}$) matches analytical gradients with a maximum mismatch of $1.13 \times 10^{-10}$ (passing the $\le 10^{-5}$ requirement). Monotonic 100-step gradient descent reduces loss to $1.70 \times 10^{-20}$ and $\hat{y} = 1.9999999998 \approx 2.0$.

---

## 7. Standalone Release, Packaging & Test Summary
The standalone Windows 11 x64 executable (`Launch.exe`) is packaged with zero external runtime dependencies. Automated verification covers 21 test cases across 6 suites (all passing). Complete reproducible artifacts, raw CSV outputs, and documentation are stored in `results/`, `data/`, and `docs/`.
