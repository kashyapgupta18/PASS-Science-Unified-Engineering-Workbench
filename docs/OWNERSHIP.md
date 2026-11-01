# Statement of Ownership and Academic Integrity

**Author**: Kashyap Gupta  
**Project**: Unified Engineering Workbench  
**Assessment**: PASS Science Works R&D Internship 2026–2027  
**Date**: November 1, 2026  

## 1. Original Work
I confirm that the core architectural implementation, finite element solver algorithms, concurrency pipelines, parsing logic, embedded hardware abstraction models, mathematical derivations, documentation, and automated test suites in this repository were designed and implemented by myself with documented AI assistance.

Specifically:
- **C++17 Core Library (`wb_core`)**: All headers and source files (`fem1d`, `stl_parser`, `csv_parser`, `batch`, `backprop`, `devices`, `sysinfo`, `export`) are authored for this project.
- **Mathematical Formulations**: The Thomas tridiagonal algorithm, Dirichlet boundary condition enforcement, midpoint error interpolation, and analytical backpropagation gradients were derived and implemented from first principles.
- **Embedded Architecture**: The modular topology, interface tables, and mock communication adapters were authored specifically to reflect real-world hardware practices (STM32, Artix-7, and JTAG probes).

## 2. Reused Open-Source Components and Licences
The following permissive, standard open-source libraries are vendored to support graphical presentation and visualization:
- **Dear ImGui** (v1.90.4) by Omar Cornut: MIT Licence. Used for user interface rendering.
- **ImPlot** (v0.16) by Evan Pezent: MIT Licence. Used for real-time 2D plotting of temperature and convergence curves.

No commercial solvers, proprietary CAD software, paid AI generation pipelines, or unauthorized copyrighted materials were used.

## 3. Data Attribution
All bundled datasets (`sample_unit_cube.stl`, `sample_temperature_synthetic.csv`, and all test fixtures) were self-created and synthetic. They do not contain confidential, proprietary, or actual client ANSYS simulation exports.
