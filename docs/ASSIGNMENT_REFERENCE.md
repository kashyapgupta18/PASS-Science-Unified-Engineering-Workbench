# PASS SCIENCE WORKS | R&D INTERNSHIP 2026-2027
## Common Technical Assignment | 15 Calendar Days | 100 Points

Build a small, reliable engineering workbench that imports geometry, solves a thermal problem, visualizes results, runs parallel workloads, and represents embedded hardware.

### 1. Scope, time and permitted tools
- Submit individually within 15 calendar days. Target effort: 40-50 hours.
- A web interface, desktop application or packaged executable is acceptable.
- All backend logic, numerical computation, file processing and concurrency must be C++17 or later.
- You may use any GPT/AI assistant and openly available libraries. Disclose AI assistance.
- Use synthetic or openly licensed data only.

### 2. Task A: Interface, backend and one-click execution (8 marks)
- Provide one coherent interface with Geometry, Thermal Analysis, Performance and Hardware views.
- Include file selection, parameter validation, progress/status, actionable error messages, and result export.
- Keep UI responsive during computation; handle cancellation or safe completion.
- Separate C++ core from presentation and transport code.
- Target Windows 11 x64. After ZIP extraction, double-clicking Launch.exe must open the working application.
- Bundle redistributable runtime dependencies lawfully.
- Include VS Code or Antigravity run configuration.
- Provide CMake source-build instructions separately.

### 3. Task B: Existing CAD / ANSYS data visualization (10 marks)
- Support (i) ASCII STL geometry file and (ii) CSV export with columns: node_id,x_m,y_m,z_m,temperature_C
- Supply one small, self-created example of each with provenance, units and schema notes.
- Display STL triangles with rotate, pan, zoom and fit-to-view.
- Display CSV nodes as coloured 3D point cloud with temperature legend, units and node inspection.
- Parse and validate both formats in C++; frontend rendering libraries are allowed.
- Reject malformed/non-finite data, explain empty files, document tested file-size limits.

### 4. Task C: Mechanical thermal FEA (14 marks)
- Steady-state, 1D thermal finite-element solver for a uniform rod using two-node linear elements.
- L = 1 m, A = 1e-4 m², k = 10 W/(m·K), q = 10,000 W/m³, T(0) = 100°C, T(L) = 20°C.
- Constant properties and insulated lateral surfaces.
- Solve -d/dx(kA dT/dx) = qA; assemble element stiffness and load, impose prescribed temperatures.
- For N = 10, 20, 40, 80: plot temperature vs x, compare with T_exact(x) = 100 - 80x/L + q/(2k)·x(L-x).
- Report max absolute error at element midpoints using FE interpolation.
- Show mesh convergence, export underlying CSV.
- Explain assumptions, units, boundary conditions and one debugging insight.
- Subcriteria: Correct formulation/assembly/BCs (6), analytical verification and convergence (5), assumptions and learning explanation (3).

### 5. Task D: Multicore/multithreaded execution (10 marks)
- Serial and CPU-parallel versions of batch independent thermal solves in C++.
- Bounded worker pool, standard threads or OpenMP.
- Reproducible batch generator with fixed seed, varied positive k and q, configurable case count and mesh size.
- Identical cases and numerical methods in both versions.
- Measure wall-clock compute time for 1, 2 and up to 4 workers (capped at logical CPUs).
- Report CPU, core counts, RAM, compiler, optimisation flags, workload, warm-up, five repetitions, median time, speedup T1/Tp, efficiency.
- Exclude file loading, rendering and compilation from compute timings.
- Verify serial/parallel agreement with max absolute temperature difference <= 1e-8°C.
- Subcriteria: Real parallel work (4), reproducible timing and agreement checks (4), honest analysis (2).

### 6. Task E: Embedded-system integration design (8 marks)
- Design modular host-connected workbench: MCU board, FPGA board, JTAG debug adapter.
- Labelled block diagram and interface/pin-voltage table.
- Explain JTAG adapter as debug interface, not generic compute board.
- C++ mock adapters: device discovery, capability reporting, connect/disconnect, telemetry message.
- Demonstrate all three device types in UI and one recoverable failure (timeout).
- Clearly label simulated behaviour.
- Subcriteria: Coherent design and electrical assumptions (4), working mocks and recovery (4).

### 7. Task F: Simple handwritten backpropagation (7 marks)
- x = 1, y = 2, h = tanh(wx + b), y_hat = vh + c, J = 0.5(y_hat - y)²
- w = 0.5, b = 0, v = 1, c = 0, learning rate = 0.1.
- Handwrite forward pass, chain-rule derivatives, one simultaneous gradient-descent update.
- C++ implementation without automatic differentiation.
- Check each gradient by central finite differences: epsilon = 1e-6, absolute error <= 1e-5.
- Plot loss over 100 training steps, explain the result.
- Subcriteria: Correct handwritten chain rule and update (4), C++ gradient check and loss plot (3).

### 8. Required ZIP contents
- CandidateID_Workbench.zip
- Launch.exe and packaged application
- src/ with frontend and C++ backend
- CMakeLists.txt and dependency info
- Editor run configuration
- data/ with provenance
- tests/
- results/ with raw CSV and labelled plots
- docs/ and README
- Version-control evidence
- Report (max 6 pages, excluding 2-page handwritten appendix)
- 3-5 minute demo video or timestamped screenshot walkthrough
- Git bundle/repository with meaningful development history
- AI-use log with at least one incorrect/unsuitable suggestion identified
- Ownership statement

### Rubric (100 points)
| Criterion | Max | Evidence |
|-----------|-----|----------|
| Interface/webpage design | 8 | Clear workflow, readable layout, responsive status, validation, usable plots |
| C++ backend and testing | 12 | Working integration, separation of concerns, robust error handling; positive, negative and numerical tests |
| Folder architecture | 6 | Clean module boundaries, sensible folders, relative paths, controlled dependencies |
| Code readability | 6 | Clear naming, focused functions, consistent style, useful comments, maintainable interfaces |
| Version control | 5 | Traceable meaningful changes; sensible exclusions, dependency/version records |
| Thermal FEA and adoption | 14 | Correct formulation/assembly/BCs (6), analytical verification and convergence (5), assumptions and learning explanation (3) |
| CAD/ANSYS visualization | 10 | Both required imports, correct units/legend, interaction, provenance, malformed-file handling |
| Multicore performance | 10 | Real parallel work (4), reproducible timing and agreement checks (4), honest analysis (2) |
| Embedded integration | 8 | Coherent modular design and electrical/interface assumptions (4); working mocks and recovery (4) |
| Backpropagation | 7 | Correct handwritten chain rule and update (4); C++ gradient check and loss plot (3) |
| Prompt engineering/AI judgement | 8 | Clear prompts, iterative refinement, independent verification, disclosure, critical judgement |
| One-click delivery/report | 6 | Clean-machine launch and editor config (3); reproducible evidence, report and limitations (3) |

### Shortlisting Gates
- 80-100: strong interview shortlist
- 50-80: interview shortlist
- Below 50: not shortlisted
- **Minimum gates**: 6/12 C++ backend, 7/14 thermal FEA, 3/6 delivery/report, working C++ backend, successful standalone launch

