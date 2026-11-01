# Supported File Formats and Specifications

## 1. ASCII STL Geometry File (`.stl`)
- **Format**: Standard ASCII Stereolithography format.
- **Specification**:
  ```text
  solid <solid_name>
    facet normal <ni> <nj> <nk>
      outer loop
        vertex <x1> <y1> <z1>
        vertex <x2> <y2> <z2>
        vertex <x3> <y3> <z3>
      endloop
    endfacet
    ...
  endsolid <solid_name>
  ```
- **Validation Rules**:
  - File must begin with the token `solid`.
  - Normal vectors and vertex coordinates must be finite floating-point values (no `NaN`, `Inf`).
  - Degenerate triangles (area < $10^{-12}$) trigger actionable warnings.
  - Max file size: 200 MiB.
  - Max triangle count: 2,000,000 triangles.
  - Viewport rendering cap: 60,000 triangles for smooth 60 FPS interactive manipulation.
  - Cancellation: Asynchronous background parsing can be safely aborted via atomic cancellation flags without UI lockup.

## 2. Nodal Temperature CSV Export (`.csv`)
- **Format**: Comma-Separated Values text file.
- **Header Line (exact match required)**:
  `node_id,x_m,y_m,z_m,temperature_C`
- **Field Definitions**:
  - `node_id`: Positive integer node identifier.
  - `x_m`, `y_m`, `z_m`: Spatial coordinates in metres (IEEE 754 64-bit float).
  - `temperature_C`: Scalar nodal temperature in degrees Celsius (°C).
- **Validation Rules**:
  - Exact header validation.
  - Duplicate `node_id` detection.
  - Non-finite numeric rejection.
  - Max file size: 200 MiB.
  - Max node count: 5,000,000 nodes.
  - Viewport rendering cap: 120,000 point-cloud vertices.

## 3. FEM 1D Result Export Formats
- **Nodal Export**: `x_m,T_fem_C,T_exact_C,error_C`
- **Midpoint Export**: `x_mid_m,T_mid_fe_C,T_mid_exact_C,error_C`
- **Convergence Study Export**: `N,h_m,max_midpoint_error_C,max_nodal_error_C`

## 4. Benchmark & Hardware Formats
- **Benchmark CSV**: `workers,median_ms,min_ms,max_ms,speedup,efficiency,max_temp_diff_C,run1_ms,run2_ms,run3_ms,run4_ms,run5_ms`
- **Backprop Loss CSV**: `step,loss,w,b,v,c,dw,db,dv,dc`
- **Telemetry Wire Format**: JSON payload containing device metrics (temperatures, bus voltages, clock frequencies).
