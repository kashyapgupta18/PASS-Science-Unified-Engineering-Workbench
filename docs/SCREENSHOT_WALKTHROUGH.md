# Unified Engineering Workbench — Timestamped Demonstration & Screenshot Walkthrough

**Assessment**: PASS Science Works R&D Internship 2026–2027  
**Candidate**: Kashyap Gupta  
**Verification Date**: November 1, 2026  
**Target Environment**: Windows 11 Home 64-bit (Build 26300), 12th Gen Intel Core i5-1235U  

This document serves as the equivalent timestamped screenshot walkthrough with terminal logs verifying all five primary workbench views, data import workflows, numerical solvers, and embedded hardware lifecycle states.

---

## 1. Standalone Startup & One-Click Launch Gate
- **Action**: Double-click `Launch.exe` in the extracted package directory or select "Launch Workbench (Standalone)" in VS Code / Antigravity.
- **Observed Behavior**:
  - The native Win32 window titled `"Unified Engineering Workbench — PASS Science Works R&D"` opens instantly at $1280 \times 800$ resolution.
  - No terminal, PowerShell console, Python runtime, or administrator prompts appear.
  - Direct3D 11 renderer initializes with hardware acceleration (or automatic WARP fallback).
  - Default status bar reads: `"Status: Unified Engineering Workbench initialized successfully."`

---

## 2. View 1: Geometry View (CAD / ASCII STL)
- **Workflow**:
  1. Default path `data/sample_unit_cube.stl` is populated in the path input.
  2. Click **"Load & Parse STL"** (or click **"Unit Cube Sample"**).
  3. Parser reads ASCII tokens, validates header, normals, loops, vertices, and checks bounding box.
- **Diagnostic Panel**:
  - Solid Name: `unit_cube`
  - Triangle Count: `12`
  - Bounding Box: $X \in [0.000, 1.000]$, $Y \in [0.000, 1.000]$, $Z \in [0.000, 1.000]$
  - Errors / Warnings: `0`
  - Data Classification Badge: `[SYNTHETIC DEMO]`
- **Interactive 3D Viewport**:
  - Left-click drag rotates the wireframe/shaded projection in Pitch and Yaw.
  - Mouse scroll wheel adjusts camera zoom factor.
  - Coordinate axes indicator (X: Red, Y: Green, Z: Blue) in the bottom-left corner updates in real time.

---

## 3. View 2: Thermal Analysis View
### Sub-Tab A: 1D Steady-State Rod FEM
- **Inputs**: $L = 1.0$ m, $A = 1.0 \times 10^{-4}$ m², $k = 10.0$ W/(m·K), $q = 10,000$ W/m³, $T(0) = 100.0$ °C, $T(L) = 20.0$ °C.
- **Action**: Click **"Solve 1D FEM"** followed by **"Run Convergence"**.
- **Results & Convergence Table**:
  - $N=10$ ($h=0.1000$ m): Midpoint Error = $1.25000000$ °C, Theoretical = $1.25000000$ °C, Nodal Roundoff = $2.84 \times 10^{-14}$ °C.
  - $N=20$ ($h=0.0500$ m): Midpoint Error = $0.31250000$ °C, Theoretical = $0.31250000$ °C, Nodal Roundoff = $1.99 \times 10^{-13}$ °C.
  - $N=40$ ($h=0.0250$ m): Midpoint Error = $0.07812500$ °C, Theoretical = $0.07812500$ °C, Nodal Roundoff = $8.81 \times 10^{-13}$ °C.
  - $N=80$ ($h=0.0125$ m): Midpoint Error = $0.01953125$ °C, Theoretical = $0.01953125$ °C, Nodal Roundoff = $1.22 \times 10^{-12}$ °C.
  - **Observed Convergence Order**: $\mathbf{2.0000}$ (matches exact theoretical $O(h^2)$ rate).
- **Interactive ImPlot Visualization**:
  - Smooth analytical curve $T_{\text{exact}}(x)$ rendered alongside discrete nodal markers and midpoint error markers.
- **Export Control**:
  - Click **"Export Results CSV"** writes verified dataset to `results/fem_active_results.csv`.

### Sub-Tab B: ANSYS / CSV 3D Nodal Temperature Results
- **Action**: Load `data/sample_temperature_synthetic.csv`.
- **Diagnostics**:
  - Schema Verified: `node_id,x_m,y_m,z_m,temperature_C`
  - Node Count: `216`
  - $T_{\text{min}} = 20.00$ °C, $T_{\text{max}} = 130.00$ °C, $T_{\text{mean}} = 67.50$ °C.
- **3D Colormapped Point Cloud**:
  - Interactive rotating 3D scatter plot colored via multi-stop thermal gradient.
  - Calibrated legend bar with labels from 20 °C to 130 °C.

---

## 4. View 3: Multicore Performance View
- **Action**: Click **"Run Benchmark (1, 2, 4 Workers)"**.
- **System Provenance Panel**:
  - CPU: `12th Gen Intel(R) Core(TM) i5-1235U`
  - Cores: `10 physical / 12 logical`
  - RAM: `7.7 GB`
  - OS: `Windows 10/11 Home Single Language Build 26300`
  - Compiler: `MSVC 1944 (VS 2022 17.14+) Release /O2`
- **Measured Results**:
  - 1 Worker (Serial): Median $0.71$ ms, Speedup $1.000\times$, Efficiency $100\%$
  - 2 Workers: Median $0.70$ ms, Speedup $1.014\times$, Efficiency $50.7\%$
  - 4 Workers: Median $0.71$ ms, Speedup $0.998\times$, Efficiency $25.0\%$
  - **Numerical Agreement Check**: Max Temp Diff = $0.000000$ °C ($\le 10^{-8}$ °C tolerance) $\to$ **[PASSED]**.
- **Interactive ImPlot Bar Chart**:
  - Displays actual scaling vs theoretical linear scaling.

---

## 5. View 4: Hardware View
- **Host Topology**: Shows Windows Host PC $\to$ Powered USB 2.0 Hub $\to$ 3 discrete devices.
- **Device Adapters**:
  - `STM32F407 MCU`: Connect/Disconnect button toggles state; reads synthetic telemetry payload (`{"temperature_C": 42.1, "voltage_V": 3.30, "uptime_s": 1.00}`).
  - `Xilinx Artix-7 FPGA`: Telemetry payload (`{"fabric_utilization_pct": 35.2, "clock_mhz": 100.4, "bram_used_kb": 45.1}`).
  - `SEGGER J-Link JTAG`: Debug interface tracking target VTref and TAP state (`{"target_voltage_V": 3.30, "protocol": "JTAG", "target_state": "halted"}`).
- **Recoverable Timeout & Retry Demonstration**:
  - Click **"Run Timeout & Retry Demo"**:
  - Attempt 1: Injected timeout $\to$ Error logged.
  - Attempt 2: Injected timeout $\to$ Error logged.
  - Attempt 3: Recovery loop succeeds $\to$ Success logged, telemetry streaming active.

---

## 6. View 5: Backpropagation View
- **Action**: Click **"Compute 100 Training Steps"**.
- **Results**:
  - Initial Loss: $1.182542$
  - Analytical Gradients: $\partial J/\partial w = -1.209464$, $\partial J/\partial b = -1.209464$, $\partial J/\partial v = -0.710682$, $\partial J/\partial c = -1.537883$.
  - Central Finite Difference Check: Max error $= 1.13 \times 10^{-10} \le 10^{-5}$ $\to$ **[PASSED]**.
  - Step 1 Loss: $0.685774$ (after first update).
  - Step 100 Loss: $1.70 \times 10^{-20}$ (monotonic descent toward zero).
  - Final Output: $\hat{y} = 1.9999999998$ (exact match to target $y=2.0$).
