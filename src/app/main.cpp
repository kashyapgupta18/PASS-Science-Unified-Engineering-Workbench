// =============================================================================
// main.cpp — Unified Engineering Workbench Desktop Application
// Windows 11 x64 Native Win32 + Direct3D 11 + Dear ImGui + ImPlot
// =============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "implot.h"

#include "wb/backprop.h"
#include "wb/batch.h"
#include "wb/csv_parser.h"
#include "wb/devices.h"
#include "wb/export.h"
#include "wb/fem1d.h"
#include "wb/stl_parser.h"
#include "wb/sysinfo.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global Direct3D state
static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*         g_pSwapChain = nullptr;
static UINT                    g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// -----------------------------------------------------------------------------
// Application State
// -----------------------------------------------------------------------------
struct AppState {
    enum ViewTab { VIEW_GEOMETRY = 0, VIEW_THERMAL = 1, VIEW_PERFORMANCE = 2, VIEW_HARDWARE = 3, VIEW_BACKPROP = 4 };
    ViewTab current_tab = VIEW_GEOMETRY;

    // --- Geometry View State ---
    char stl_path[512] = "data/sample_unit_cube.stl";
    wb::StlData stl_data;
    bool stl_loaded = false;
    float cam_pitch = 0.5f;
    float cam_yaw = 0.8f;
    float cam_zoom = 2.5f;
    float cam_pan_x = 0.0f;
    float cam_pan_y = 0.0f;

    // --- Thermal View State ---
    wb::FemParams fem_params;
    wb::FemResult fem_result;
    bool fem_solved = false;
    std::vector<wb::FemResult> convergence_results;
    double observed_convergence_order = 0.0;
    bool convergence_solved = false;

    // Real CSV temperature import
    char csv_path[512] = "data/sample_temperature_synthetic.csv";
    wb::CsvData csv_data;
    bool csv_loaded = false;
    float csv_cam_pitch = 0.4f;
    float csv_cam_yaw = 0.6f;
    float csv_cam_zoom = 2.5f;

    // --- Performance View State ---
    wb::BatchConfig batch_cfg;
    wb::BenchmarkReport bench_report;
    bool bench_running = false;
    bool bench_done = false;
    std::atomic<bool> bench_cancel{false};
    float bench_progress = 0.0f;
    std::string bench_status = "Ready";

    // --- Hardware View State ---
    wb::DeviceAdapter mcu = wb::create_mcu_device();
    wb::DeviceAdapter fpga = wb::create_fpga_device();
    wb::DeviceAdapter jtag = wb::create_jtag_device();
    std::string mcu_telemetry_str;
    std::string fpga_telemetry_str;
    std::string jtag_telemetry_str;
    std::vector<std::string> hw_log;
    bool retry_demo_active = false;

    // --- Backprop View State ---
    wb::BackpropParams bp_params;
    wb::BackpropResult bp_result;
    bool bp_computed = false;

    // --- System Info ---
    wb::SystemInfo sys_info;
    std::string status_message = "Unified Engineering Workbench initialized successfully.";
};

static AppState g_app;

// -----------------------------------------------------------------------------
// 3D Projection Helpers for ImGui DrawList
// -----------------------------------------------------------------------------
struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };

static Vec2 ProjectPoint3D(const Vec3& pt, const Vec3& center, float pitch, float yaw, float zoom, ImVec2 viewport_min, ImVec2 viewport_size) {
    // Translate relative to center
    float rx = pt.x - center.x;
    float ry = pt.y - center.y;
    float rz = pt.z - center.z;

    // Rotate yaw (around Z axis)
    float cy = cosf(yaw), sy = sinf(yaw);
    float x1 = rx * cy - ry * sy;
    float y1 = rx * sy + ry * cy;
    float z1 = rz;

    // Rotate pitch (around X axis)
    float cp = cosf(pitch), sp = sinf(pitch);
    float y2 = y1 * cp - z1 * sp;
    float z2 = y1 * sp + z1 * cp;

    // Orthographic projection scaled with zoom
    float scale = (viewport_size.x < viewport_size.y ? viewport_size.x : viewport_size.y) * 0.45f * zoom;
    float screen_x = viewport_min.x + viewport_size.x * 0.5f + x1 * scale;
    float screen_y = viewport_min.y + viewport_size.y * 0.5f - z2 * scale; // Y-up to screen Y-down

    return { screen_x, screen_y };
}

// Map temperature to RGB colormap (Jet / Turbo style)
static ImU32 TemperatureToColor(double T, double T_min, double T_max) {
    double t = (T_max > T_min) ? (T - T_min) / (T_max - T_min) : 0.5;
    t = std::max(0.0, std::min(1.0, t));

    // 4-stop Jet approximation: Blue -> Cyan -> Yellow -> Red
    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (t < 0.25f) {
        float f = static_cast<float>(t / 0.25f);
        r = 0.0f; g = f; b = 1.0f;
    } else if (t < 0.5f) {
        float f = static_cast<float>((t - 0.25f) / 0.25f);
        r = 0.0f; g = 1.0f; b = 1.0f - f;
    } else if (t < 0.75f) {
        float f = static_cast<float>((t - 0.5f) / 0.25f);
        r = f; g = 1.0f; b = 0.0f;
    } else {
        float f = static_cast<float>((t - 0.75f) / 0.25f);
        r = 1.0f; g = 1.0f - f; b = 0.0f;
    }
    return IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255), 230);
}

// -----------------------------------------------------------------------------
// VIEW 1: Geometry View
// -----------------------------------------------------------------------------
void RenderGeometryView() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CAD / STL SURFACE MESH VISUALIZATION");
    ImGui::Separator();

    ImGui::BeginChild("GeometryLeft", ImVec2(360, 0), true);
    {
        ImGui::Text("File Input (ASCII STL):");
        ImGui::InputText("##StlPath", g_app.stl_path, sizeof(g_app.stl_path));

        if (ImGui::Button("Load & Parse STL", ImVec2(160, 28))) {
            g_app.stl_data = wb::stl_parse_file(g_app.stl_path);
            g_app.stl_loaded = g_app.stl_data.success;
            if (g_app.stl_loaded) {
                g_app.status_message = "Parsed STL: " + std::to_string(g_app.stl_data.triangles.size()) + " triangles.";
            } else {
                g_app.status_message = "STL Parse Error: " + g_app.stl_data.error_message;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Unit Cube Sample", ImVec2(160, 28))) {
            strcpy_s(g_app.stl_path, "data/sample_unit_cube.stl");
            g_app.stl_data = wb::stl_parse_file(g_app.stl_path);
            g_app.stl_loaded = g_app.stl_data.success;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Mesh Diagnostics:");

        if (g_app.stl_loaded) {
            ImGui::BulletText("Status: VALID");
            ImGui::BulletText("Solid Name: %s", g_app.stl_data.name.c_str());
            ImGui::BulletText("Triangles: %zu", g_app.stl_data.triangles.size());
            ImGui::BulletText("X Bounds: [%.3f, %.3f]", g_app.stl_data.bounds_min[0], g_app.stl_data.bounds_max[0]);
            ImGui::BulletText("Y Bounds: [%.3f, %.3f]", g_app.stl_data.bounds_min[1], g_app.stl_data.bounds_max[1]);
            ImGui::BulletText("Z Bounds: [%.3f, %.3f]", g_app.stl_data.bounds_min[2], g_app.stl_data.bounds_max[2]);
            ImGui::BulletText("Errors / Warnings: %d", g_app.stl_data.error_count);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No valid mesh currently loaded.");
            if (!g_app.stl_data.error_message.empty()) {
                ImGui::TextWrapped("Error: %s", g_app.stl_data.error_message.c_str());
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Camera Controls:");
        ImGui::SliderFloat("Pitch", &g_app.cam_pitch, -1.5f, 1.5f);
        ImGui::SliderFloat("Yaw", &g_app.cam_yaw, -3.14f, 3.14f);
        ImGui::SliderFloat("Zoom", &g_app.cam_zoom, 0.2f, 5.0f);
        if (ImGui::Button("Reset Camera", ImVec2(140, 24))) {
            g_app.cam_pitch = 0.5f;
            g_app.cam_yaw = 0.8f;
            g_app.cam_zoom = 2.5f;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Data Classification: [SYNTHETIC DEMO]");
        ImGui::TextWrapped("ASCII STL represents triangulated boundary geometry. Native CAD solids are outside scope.");
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("GeometryViewport", ImVec2(0, 0), true);
    {
        ImVec2 v_min = ImGui::GetCursorScreenPos();
        ImVec2 v_size = ImGui::GetContentRegionAvail();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Viewport background
        draw->AddRectFilled(v_min, ImVec2(v_min.x + v_size.x, v_min.y + v_size.y), IM_COL32(20, 24, 30, 255));
        draw->AddRect(v_min, ImVec2(v_min.x + v_size.x, v_min.y + v_size.y), IM_COL32(50, 60, 75, 255));

        // Interactive mouse drag for camera
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            g_app.cam_yaw += delta.x * 0.01f;
            g_app.cam_pitch += delta.y * 0.01f;
            g_app.cam_pitch = std::max(-1.5f, std::min(1.5f, g_app.cam_pitch));
        }
        float wheel = ImGui::GetIO().MouseWheel;
        if (ImGui::IsWindowHovered() && wheel != 0.0f) {
            g_app.cam_zoom = std::max(0.1f, g_app.cam_zoom + wheel * 0.2f);
        }

        if (g_app.stl_loaded && !g_app.stl_data.triangles.empty()) {
            Vec3 center = {
                (g_app.stl_data.bounds_min[0] + g_app.stl_data.bounds_max[0]) * 0.5f,
                (g_app.stl_data.bounds_min[1] + g_app.stl_data.bounds_max[1]) * 0.5f,
                (g_app.stl_data.bounds_min[2] + g_app.stl_data.bounds_max[2]) * 0.5f
            };

            size_t max_draw = std::min(g_app.stl_data.triangles.size(), size_t(60000));
            for (size_t i = 0; i < max_draw; ++i) {
                const auto& tri = g_app.stl_data.triangles[i];
                Vec2 p1 = ProjectPoint3D({tri.v1[0], tri.v1[1], tri.v1[2]}, center, g_app.cam_pitch, g_app.cam_yaw, g_app.cam_zoom, v_min, v_size);
                Vec2 p2 = ProjectPoint3D({tri.v2[0], tri.v2[1], tri.v2[2]}, center, g_app.cam_pitch, g_app.cam_yaw, g_app.cam_zoom, v_min, v_size);
                Vec2 p3 = ProjectPoint3D({tri.v3[0], tri.v3[1], tri.v3[2]}, center, g_app.cam_pitch, g_app.cam_yaw, g_app.cam_zoom, v_min, v_size);

                draw->AddTriangleFilled(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImVec2(p3.x, p3.y), IM_COL32(50, 110, 180, 160));
                draw->AddTriangle(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImVec2(p3.x, p3.y), IM_COL32(140, 200, 255, 255), 1.0f);
            }

            // Draw coordinate axis gizmo in corner
            Vec3 origin = {0, 0, 0};
            Vec2 scr_o = ProjectPoint3D(origin, origin, g_app.cam_pitch, g_app.cam_yaw, 1.0f, ImVec2(v_min.x + 60, v_min.y + v_size.y - 60), ImVec2(80, 80));
            Vec2 scr_x = ProjectPoint3D({0.3f, 0, 0}, origin, g_app.cam_pitch, g_app.cam_yaw, 1.0f, ImVec2(v_min.x + 60, v_min.y + v_size.y - 60), ImVec2(80, 80));
            Vec2 scr_y = ProjectPoint3D({0, 0.3f, 0}, origin, g_app.cam_pitch, g_app.cam_yaw, 1.0f, ImVec2(v_min.x + 60, v_min.y + v_size.y - 60), ImVec2(80, 80));
            Vec2 scr_z = ProjectPoint3D({0, 0, 0.3f}, origin, g_app.cam_pitch, g_app.cam_yaw, 1.0f, ImVec2(v_min.x + 60, v_min.y + v_size.y - 60), ImVec2(80, 80));

            draw->AddLine(ImVec2(scr_o.x, scr_o.y), ImVec2(scr_x.x, scr_x.y), IM_COL32(255, 60, 60, 255), 2.0f);
            draw->AddLine(ImVec2(scr_o.x, scr_o.y), ImVec2(scr_y.x, scr_y.y), IM_COL32(60, 255, 60, 255), 2.0f);
            draw->AddLine(ImVec2(scr_o.x, scr_o.y), ImVec2(scr_z.x, scr_z.y), IM_COL32(60, 120, 255, 255), 2.0f);
            draw->AddText(ImVec2(scr_x.x + 4, scr_x.y), IM_COL32(255, 80, 80, 255), "X");
            draw->AddText(ImVec2(scr_y.x + 4, scr_y.y), IM_COL32(80, 255, 80, 255), "Y");
            draw->AddText(ImVec2(scr_z.x + 4, scr_z.y), IM_COL32(80, 140, 255, 255), "Z");
        } else {
            draw->AddText(ImVec2(v_min.x + 20, v_min.y + 20), IM_COL32(180, 180, 180, 255), "Load an STL file to inspect interactive 3D geometry.");
        }
    }
    ImGui::EndChild();
}

// -----------------------------------------------------------------------------
// VIEW 2: Thermal Analysis View
// -----------------------------------------------------------------------------
void RenderThermalView() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "THERMAL FINITE ELEMENT ANALYSIS & VERIFICATION");
    ImGui::Separator();

    if (ImGui::BeginTabBar("ThermalSubTabs")) {
        // --- Sub-Tab A: 1D Steady-State FEM Solver ---
        if (ImGui::BeginTabItem("1D Rod Solver & Verification")) {
            ImGui::BeginChild("FemParamsPanel", ImVec2(340, 0), true);
            {
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Problem Formulation:");
                ImGui::TextWrapped("-d/dx(kA dT/dx) = qA on rod [0, L]");
                ImGui::Separator();

                ImGui::Text("Parameters:");
                ImGui::InputDouble("Length L [m]", &g_app.fem_params.L, 0.1, 1.0, "%.3f");
                ImGui::InputDouble("Area A [m2]", &g_app.fem_params.A, 1e-5, 1e-4, "%.2e");
                ImGui::InputDouble("Conductivity k [W/m-K]", &g_app.fem_params.k, 1.0, 5.0, "%.2f");
                ImGui::InputDouble("Heat Gen q [W/m3]", &g_app.fem_params.q, 1e3, 5e3, "%.1f");
                ImGui::InputDouble("T(0) Boundary [C]", &g_app.fem_params.T0, 5.0, 10.0, "%.1f");
                ImGui::InputDouble("T(L) Boundary [C]", &g_app.fem_params.TL, 5.0, 10.0, "%.1f");

                const char* mesh_items[] = { "10 Elements", "20 Elements", "40 Elements", "80 Elements" };
                static int current_mesh_idx = 0;
                if (ImGui::Combo("Mesh Resolution", &current_mesh_idx, mesh_items, IM_ARRAYSIZE(mesh_items))) {
                    int n_table[] = {10, 20, 40, 80};
                    g_app.fem_params.N = n_table[current_mesh_idx];
                }

                ImGui::Spacing();
                if (ImGui::Button("Solve 1D FEM", ImVec2(150, 30))) {
                    g_app.fem_result = wb::fem1d_solve(g_app.fem_params);
                    g_app.fem_solved = g_app.fem_result.success;
                    if (g_app.fem_solved) {
                        g_app.status_message = "Solved 1D FEM for N=" + std::to_string(g_app.fem_params.N) + " elements.";
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Run Convergence", ImVec2(150, 30))) {
                    g_app.convergence_results.clear();
                    int sizes[] = {10, 20, 40, 80};
                    wb::FemParams cp = g_app.fem_params;
                    for (int n : sizes) {
                        cp.N = n;
                        g_app.convergence_results.push_back(wb::fem1d_solve(cp));
                    }
                    if (g_app.convergence_results.size() >= 4) {
                        g_app.observed_convergence_order = wb::fem1d_convergence_order(
                            g_app.convergence_results[0].h, g_app.convergence_results[0].max_midpoint_error,
                            g_app.convergence_results[3].h, g_app.convergence_results[3].max_midpoint_error);
                        g_app.convergence_solved = true;
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Results Summary:");
                if (g_app.fem_solved) {
                    ImGui::BulletText("Elements: %d (h = %.4f m)", g_app.fem_result.N, g_app.fem_result.h);
                    ImGui::BulletText("Max Midpoint Error: %.8f C", g_app.fem_result.max_midpoint_error);
                    double theor_err = (g_app.fem_params.q * g_app.fem_result.h * g_app.fem_result.h) / (8.0 * g_app.fem_params.k);
                    ImGui::BulletText("Theoretical q*h2/(8k): %.8f C", theor_err);
                    ImGui::BulletText("Max Nodal Error: %.2e C", g_app.fem_result.max_nodal_error);

                    ImGui::Spacing();
                    if (ImGui::Button("Export Results CSV", ImVec2(200, 24))) {
                        std::filesystem::create_directories("results");
                        wb::export_fem_results(g_app.fem_result, g_app.fem_params, "results/fem_active_results.csv");
                        g_app.status_message = "Exported results/fem_active_results.csv";
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Click Solve to calculate temperature profile.");
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("FemPlotArea", ImVec2(0, 0), true);
            {
                if (g_app.fem_solved && !g_app.fem_result.x_nodes.empty()) {
                    if (ImPlot::BeginPlot("Rod Temperature Profile: T(x) [C]", ImVec2(-1, 380))) {
                        ImPlot::SetupAxes("Position x [m]", "Temperature [C]");
                        // Analytical smooth curve (100 points)
                        static std::vector<double> x_exact(101), t_exact(101);
                        for (int i = 0; i <= 100; ++i) {
                            x_exact[i] = g_app.fem_params.L * (i / 100.0);
                            t_exact[i] = wb::fem1d_analytical(x_exact[i], g_app.fem_params);
                        }
                        ImPlot::PlotLine("Analytical Exact Solution", x_exact.data(), t_exact.data(), 101);
                        ImPlot::PlotScatter("FE Nodal Solution", g_app.fem_result.x_nodes.data(), g_app.fem_result.T_nodes.data(), static_cast<int>(g_app.fem_result.x_nodes.size()));
                        ImPlot::PlotScatter("FE Midpoints", g_app.fem_result.x_mid.data(), g_app.fem_result.T_mid_fe.data(), static_cast<int>(g_app.fem_result.x_mid.size()));
                        ImPlot::EndPlot();
                    }

                    if (g_app.convergence_solved) {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Verification & Convergence Evidence:");
                        ImGui::Text("Observed Convergence Order: %.4f (Theoretical: 2.0000)", g_app.observed_convergence_order);

                        if (ImGui::BeginTable("ConvTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("N Elements");
                            ImGui::TableSetupColumn("h [m]");
                            ImGui::TableSetupColumn("Midpoint Error [C]");
                            ImGui::TableSetupColumn("Theoretical Error [C]");
                            ImGui::TableSetupColumn("Nodal Roundoff [C]");
                            ImGui::TableHeadersRow();

                            for (const auto& cr : g_app.convergence_results) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", cr.N);
                                ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", cr.h);
                                ImGui::TableSetColumnIndex(2); ImGui::Text("%.8f", cr.max_midpoint_error);
                                double th = (g_app.fem_params.q * cr.h * cr.h) / (8.0 * g_app.fem_params.k);
                                ImGui::TableSetColumnIndex(3); ImGui::Text("%.8f", th);
                                ImGui::TableSetColumnIndex(4); ImGui::Text("%.2e", cr.max_nodal_error);
                            }
                            ImGui::EndTable();
                        }
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Run the 1D FEM solver to view interactive temperature plots and verification evidence.");
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // --- Sub-Tab B: ANSYS / CSV Temperature Results Import ---
        if (ImGui::BeginTabItem("ANSYS / CSV 3D Nodal Temperature Results")) {
            ImGui::BeginChild("CsvControls", ImVec2(340, 0), true);
            {
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Import Temperature CSV:");
                ImGui::InputText("##CsvPath", g_app.csv_path, sizeof(g_app.csv_path));

                if (ImGui::Button("Parse Temperature CSV", ImVec2(180, 28))) {
                    g_app.csv_data = wb::csv_parse_file(g_app.csv_path);
                    g_app.csv_loaded = g_app.csv_data.success;
                    if (g_app.csv_loaded) {
                        g_app.status_message = "Parsed CSV: " + std::to_string(g_app.csv_data.nodes.size()) + " nodes.";
                    } else {
                        g_app.status_message = "CSV Parse Error: " + g_app.csv_data.error_message;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Synthetic Sample", ImVec2(130, 28))) {
                    strcpy_s(g_app.csv_path, "data/sample_temperature_synthetic.csv");
                    g_app.csv_data = wb::csv_parse_file(g_app.csv_path);
                    g_app.csv_loaded = g_app.csv_data.success;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Dataset Statistics:");
                if (g_app.csv_loaded) {
                    ImGui::BulletText("Node Count: %zu", g_app.csv_data.nodes.size());
                    ImGui::BulletText("Min Temp: %.2f C", g_app.csv_data.temp_min);
                    ImGui::BulletText("Max Temp: %.2f C", g_app.csv_data.temp_max);
                    ImGui::BulletText("Mean Temp: %.2f C", g_app.csv_data.temp_mean);
                    ImGui::BulletText("X Range: [%.2f, %.2f] m", g_app.csv_data.bounds_min[0], g_app.csv_data.bounds_max[0]);
                    ImGui::BulletText("Y Range: [%.2f, %.2f] m", g_app.csv_data.bounds_min[1], g_app.csv_data.bounds_max[1]);
                    ImGui::BulletText("Z Range: [%.2f, %.2f] m", g_app.csv_data.bounds_min[2], g_app.csv_data.bounds_max[2]);
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No valid CSV currently loaded.");
                    if (!g_app.csv_data.error_message.empty()) {
                        ImGui::TextWrapped("Error: %s", g_app.csv_data.error_message.c_str());
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Classification: [SYNTHETIC DEMO]");
                ImGui::TextWrapped("Required schema: node_id,x_m,y_m,z_m,temperature_C");
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("CsvPointcloudViewport", ImVec2(0, 0), true);
            {
                ImVec2 v_min = ImGui::GetCursorScreenPos();
                ImVec2 v_size = ImGui::GetContentRegionAvail();
                ImDrawList* draw = ImGui::GetWindowDrawList();

                draw->AddRectFilled(v_min, ImVec2(v_min.x + v_size.x, v_min.y + v_size.y), IM_COL32(18, 22, 28, 255));
                draw->AddRect(v_min, ImVec2(v_min.x + v_size.x, v_min.y + v_size.y), IM_COL32(45, 55, 70, 255));

                if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    ImVec2 delta = ImGui::GetIO().MouseDelta;
                    g_app.csv_cam_yaw += delta.x * 0.01f;
                    g_app.csv_cam_pitch += delta.y * 0.01f;
                }
                float wheel = ImGui::GetIO().MouseWheel;
                if (ImGui::IsWindowHovered() && wheel != 0.0f) {
                    g_app.csv_cam_zoom = std::max(0.2f, g_app.csv_cam_zoom + wheel * 0.2f);
                }

                if (g_app.csv_loaded && !g_app.csv_data.nodes.empty()) {
                    Vec3 center = {
                        static_cast<float>((g_app.csv_data.bounds_min[0] + g_app.csv_data.bounds_max[0]) * 0.5),
                        static_cast<float>((g_app.csv_data.bounds_min[1] + g_app.csv_data.bounds_max[1]) * 0.5),
                        static_cast<float>((g_app.csv_data.bounds_min[2] + g_app.csv_data.bounds_max[2]) * 0.5)
                    };

                    for (const auto& node : g_app.csv_data.nodes) {
                        Vec2 pt = ProjectPoint3D({static_cast<float>(node.x), static_cast<float>(node.y), static_cast<float>(node.z)},
                                                 center, g_app.csv_cam_pitch, g_app.csv_cam_yaw, g_app.csv_cam_zoom, v_min, v_size);
                        ImU32 col = TemperatureToColor(node.temperature, g_app.csv_data.temp_min, g_app.csv_data.temp_max);
                        draw->AddCircleFilled(ImVec2(pt.x, pt.y), 4.5f, col);
                    }

                    // Temperature Colormap Legend
                    float legend_w = 20.0f;
                    float legend_h = 160.0f;
                    float leg_x = v_min.x + v_size.x - 70.0f;
                    float leg_y = v_min.y + 30.0f;

                    draw->AddText(ImVec2(leg_x - 10, leg_y - 20), IM_COL32(255, 255, 255, 255), "Temp [C]");
                    for (int i = 0; i < 100; ++i) {
                        float y_curr = leg_y + (1.0f - (i / 100.0f)) * legend_h;
                        float y_next = leg_y + (1.0f - ((i + 1) / 100.0f)) * legend_h;
                        double T = g_app.csv_data.temp_min + (i / 100.0) * (g_app.csv_data.temp_max - g_app.csv_data.temp_min);
                        ImU32 c = TemperatureToColor(T, g_app.csv_data.temp_min, g_app.csv_data.temp_max);
                        draw->AddRectFilled(ImVec2(leg_x, y_next), ImVec2(leg_x + legend_w, y_curr), c);
                    }
                    draw->AddRect(ImVec2(leg_x, leg_y), ImVec2(leg_x + legend_w, leg_y + legend_h), IM_COL32(200, 200, 200, 255));

                    std::string max_lbl = std::to_string(static_cast<int>(g_app.csv_data.temp_max)) + " C";
                    std::string min_lbl = std::to_string(static_cast<int>(g_app.csv_data.temp_min)) + " C";
                    draw->AddText(ImVec2(leg_x + legend_w + 4, leg_y), IM_COL32(220, 220, 220, 255), max_lbl.c_str());
                    draw->AddText(ImVec2(leg_x + legend_w + 4, leg_y + legend_h - 14), IM_COL32(220, 220, 220, 255), min_lbl.c_str());
                } else {
                    draw->AddText(ImVec2(v_min.x + 20, v_min.y + 20), IM_COL32(180, 180, 180, 255), "Load temperature CSV to inspect 3D colored point cloud.");
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// -----------------------------------------------------------------------------
// VIEW 3: Multicore Performance View
// -----------------------------------------------------------------------------
void RenderPerformanceView() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "MULTICORE BATCH SOLVER BENCHMARK & SYSTEM PROVENANCE");
    ImGui::Separator();

    ImGui::BeginChild("PerfLeft", ImVec2(360, 0), true);
    {
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Target Machine Provenance:");
        ImGui::BulletText("CPU: %s", g_app.sys_info.cpu_name.c_str());
        ImGui::BulletText("Cores: %d physical / %d logical", g_app.sys_info.physical_cores, g_app.sys_info.logical_cores);
        ImGui::BulletText("RAM: %s", wb::format_bytes(g_app.sys_info.ram_bytes).c_str());
        ImGui::BulletText("OS: %s", g_app.sys_info.os_name.c_str());
        ImGui::BulletText("Compiler: %s", g_app.sys_info.compiler.c_str());
        ImGui::BulletText("Build Type: %s", g_app.sys_info.build_type.c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Benchmark Configuration:");
        ImGui::SliderInt("Batch Cases", &g_app.batch_cfg.case_count, 50, 1000);
        ImGui::SliderInt("Mesh Size / Case", &g_app.batch_cfg.mesh_size, 20, 160);

        ImGui::Spacing();
        if (g_app.bench_running) {
            ImGui::ProgressBar(g_app.bench_progress, ImVec2(-1, 24));
            if (ImGui::Button("Cancel Benchmark", ImVec2(180, 28))) {
                g_app.bench_cancel.store(true);
            }
        } else {
            if (ImGui::Button("Run Benchmark (1, 2, 4 Workers)", ImVec2(240, 32))) {
                g_app.bench_running = true;
                g_app.bench_done = false;
                g_app.bench_cancel.store(false);
                g_app.bench_status = "Benchmarking on logical cores...";

                std::thread([&]() {
                    g_app.bench_report = wb::batch_benchmark(g_app.batch_cfg);
                    g_app.bench_running = false;
                    g_app.bench_done = true;
                    g_app.bench_status = "Completed 5 timed repetitions across 1, 2, 4 workers.";
                }).detach();
            }
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Status: %s", g_app.bench_status.c_str());

        if (g_app.bench_done) {
            ImGui::Spacing();
            if (ImGui::Button("Export Benchmark CSV", ImVec2(200, 24))) {
                std::filesystem::create_directories("results");
                wb::export_benchmark_results(g_app.bench_report, "results/benchmark_results.csv");
                g_app.status_message = "Exported results/benchmark_results.csv";
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("PerfRight", ImVec2(0, 0), true);
    {
        if (g_app.bench_done && !g_app.bench_report.results.empty()) {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Benchmark Execution Results:");

            if (ImGui::BeginTable("BenchTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Workers");
                ImGui::TableSetupColumn("Median [ms]");
                ImGui::TableSetupColumn("Min [ms]");
                ImGui::TableSetupColumn("Max [ms]");
                ImGui::TableSetupColumn("Speedup");
                ImGui::TableSetupColumn("Efficiency");
                ImGui::TableSetupColumn("Max Temp Diff [C]");
                ImGui::TableHeadersRow();

                for (const auto& br : g_app.bench_report.results) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", br.num_workers);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f", br.median_time_ms);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f", br.min_time_ms);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f", br.max_time_ms);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f x", br.speedup);
                    ImGui::TableSetColumnIndex(5); ImGui::Text("%.1f %%", br.efficiency * 100.0);
                    ImGui::TableSetColumnIndex(6); ImGui::Text("%.2e", br.max_temp_diff);
                }
                ImGui::EndTable();
            }

            ImGui::Spacing();
            bool passed = g_app.bench_report.agreement_check_passed;
            ImGui::Text("Numerical Agreement Check (Max Diff <= 1e-8 C): ");
            ImGui::SameLine();
            if (passed) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[PASSED] — Strict serial/parallel numerical consistency verified.");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[FAILED] — Inconsistency detected.");
            }

            ImGui::Spacing();
            if (ImPlot::BeginPlot("Multicore Speedup Scaling", ImVec2(-1, 320))) {
                ImPlot::SetupAxes("Worker Threads", "Speedup T1/Tp");
                std::vector<double> workers, speedups, ideal;
                for (const auto& br : g_app.bench_report.results) {
                    workers.push_back(br.num_workers);
                    speedups.push_back(br.speedup);
                    ideal.push_back(br.num_workers);
                }
                ImPlot::PlotLine("Measured Speedup", workers.data(), speedups.data(), static_cast<int>(workers.size()));
                ImPlot::PlotLine("Ideal Linear Scaling", workers.data(), ideal.data(), static_cast<int>(workers.size()));
                ImPlot::EndPlot();
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Click 'Run Benchmark' to execute genuine serial and parallel batch thermal solves on this CPU.");
        }
    }
    ImGui::EndChild();
}

// -----------------------------------------------------------------------------
// VIEW 4: Hardware View
// -----------------------------------------------------------------------------
void RenderHardwareView() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "EMBEDDED SUBSYSTEM ARCHITECTURE & SIMULATED HARDWARE");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[SIMULATED HARDWARE]");
    ImGui::Separator();

    ImGui::BeginChild("HwTopology", ImVec2(340, 0), true);
    {
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Host & Bus Architecture:");
        ImGui::TextWrapped("Windows Host PC connected via Powered USB 2.0 Hub (5.0V VBUS, 500mA budget) to 3 discrete subsystems.");
        ImGui::Separator();

        // Fault injection test demo
        ImGui::Text("Fault Injection & Recovery Demo:");
        ImGui::TextWrapped("Demonstrates 2 injected connection timeouts followed by successful connection on retry attempt 3.");

        if (ImGui::Button("Run Timeout & Retry Demo", ImVec2(240, 30))) {
            g_app.hw_log.clear();
            g_app.hw_log.push_back("[INFO] Initializing fault injection demonstration on FPGA target...");
            g_app.fpga.inject_connect_fault(2);

            g_app.hw_log.push_back("[ATTEMPT 1] Connecting to Artix-7 FPGA (timeout=1000ms)...");
            bool a1 = g_app.fpga.connect(1000);
            if (!a1) g_app.hw_log.push_back("[ERROR] Attempt 1 Failed: Connection timed out (Injected Fault).");

            g_app.hw_log.push_back("[ATTEMPT 2] Retrying connection (attempt 2/3)...");
            bool a2 = g_app.fpga.connect(1000);
            if (!a2) g_app.hw_log.push_back("[ERROR] Attempt 2 Failed: Connection timed out (Injected Fault).");

            g_app.hw_log.push_back("[ATTEMPT 3] Retrying connection (attempt 3/3)...");
            bool a3 = g_app.fpga.connect(1000);
            if (a3) {
                g_app.hw_log.push_back("[SUCCESS] Attempt 3 Succeeded: FPGA connected and operational!");
                g_app.hw_log.push_back("[TELEMETRY] " + g_app.fpga.read_telemetry().payload_json);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Fault Recovery Log:");
        for (const auto& line : g_app.hw_log) {
            if (line.find("[SUCCESS]") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", line.c_str());
            } else if (line.find("[ERROR]") != std::string::npos) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", line.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", line.c_str());
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("HwDevices", ImVec2(0, 0), true);
    {
        // Device 1: MCU
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Device 1: STM32F407 Microcontroller Board");
        ImGui::SameLine();
        if (g_app.mcu.is_connected()) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[CONNECTED]");
            ImGui::SameLine();
            if (ImGui::Button("Disconnect##MCU", ImVec2(100, 20))) g_app.mcu.disconnect();
            g_app.mcu_telemetry_str = g_app.mcu.read_telemetry().payload_json;
            ImGui::Text("Telemetry: %s", g_app.mcu_telemetry_str.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[DISCONNECTED]");
            ImGui::SameLine();
            if (ImGui::Button("Connect##MCU", ImVec2(100, 20))) g_app.mcu.connect();
        }
        for (const auto& cap : g_app.mcu.capabilities()) {
            ImGui::BulletText("%s: %s", cap.name.c_str(), cap.value.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Device 2: FPGA
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Device 2: Xilinx Artix-7 (XC7A35T) FPGA Board");
        ImGui::SameLine();
        if (g_app.fpga.is_connected()) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[CONNECTED]");
            ImGui::SameLine();
            if (ImGui::Button("Disconnect##FPGA", ImVec2(100, 20))) g_app.fpga.disconnect();
            g_app.fpga_telemetry_str = g_app.fpga.read_telemetry().payload_json;
            ImGui::Text("Telemetry: %s", g_app.fpga_telemetry_str.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[DISCONNECTED]");
            ImGui::SameLine();
            if (ImGui::Button("Connect##FPGA", ImVec2(100, 20))) g_app.fpga.connect();
        }
        for (const auto& cap : g_app.fpga.capabilities()) {
            ImGui::BulletText("%s: %s", cap.name.c_str(), cap.value.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Device 3: JTAG Probe
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Device 3: SEGGER J-Link JTAG Debug Adapter");
        ImGui::SameLine();
        if (g_app.jtag.is_connected()) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[CONNECTED]");
            ImGui::SameLine();
            if (ImGui::Button("Disconnect##JTAG", ImVec2(100, 20))) g_app.jtag.disconnect();
            g_app.jtag_telemetry_str = g_app.jtag.read_telemetry().payload_json;
            ImGui::Text("Telemetry: %s", g_app.jtag_telemetry_str.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[DISCONNECTED]");
            ImGui::SameLine();
            if (ImGui::Button("Connect##JTAG", ImVec2(100, 20))) g_app.jtag.connect();
        }
        for (const auto& cap : g_app.jtag.capabilities()) {
            ImGui::BulletText("%s: %s", cap.name.c_str(), cap.value.c_str());
        }
    }
    ImGui::EndChild();
}

// -----------------------------------------------------------------------------
// VIEW 5: Backpropagation View
// -----------------------------------------------------------------------------
void RenderBackpropView() {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "NEURAL NETWORK BACKPROPAGATION & GRADIENT VERIFICATION");
    ImGui::Separator();

    ImGui::BeginChild("BpLeft", ImVec2(360, 0), true);
    {
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Model Specification:");
        ImGui::Text("h = tanh(w*x + b)");
        ImGui::Text("y_hat = v*h + c");
        ImGui::Text("Loss J = 0.5 * (y_hat - y)^2");
        ImGui::Separator();

        ImGui::Text("Initial Conditions:");
        ImGui::BulletText("Input x = %.1f, Target y = %.1f", g_app.bp_params.x, g_app.bp_params.y);
        ImGui::BulletText("w0 = %.2f, b0 = %.2f", g_app.bp_params.w0, g_app.bp_params.b0);
        ImGui::BulletText("v0 = %.2f, c0 = %.2f", g_app.bp_params.v0, g_app.bp_params.c0);
        ImGui::BulletText("Learning Rate = %.2f", g_app.bp_params.lr);

        ImGui::Spacing();
        if (ImGui::Button("Compute 100 Training Steps", ImVec2(240, 30))) {
            g_app.bp_result = wb::backprop_train(g_app.bp_params);
            g_app.bp_computed = true;
            g_app.status_message = "Computed 100 backprop steps. Max grad check error: " +
                                   std::to_string(g_app.bp_result.max_grad_check_error);
        }

        if (g_app.bp_computed) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Finite-Difference Gradient Check:");
            ImGui::BulletText("Epsilon: 1e-6");
            ImGui::BulletText("Max Error: %.2e", g_app.bp_result.max_grad_check_error);
            ImGui::BulletText("Status: %s", g_app.bp_result.grad_check_passed ? "PASSED (<= 1e-5)" : "FAILED");

            ImGui::Spacing();
            if (ImGui::Button("Export Loss CSV", ImVec2(200, 24))) {
                std::filesystem::create_directories("results");
                wb::export_backprop_loss(g_app.bp_result, "results/backprop_loss.csv");
                g_app.status_message = "Exported results/backprop_loss.csv";
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("BpRight", ImVec2(0, 0), true);
    {
        if (g_app.bp_computed && !g_app.bp_result.steps.empty()) {
            if (ImPlot::BeginPlot("Training Loss Trajectory J(step)", ImVec2(-1, 380))) {
                ImPlot::SetupAxes("Training Step", "Loss J = 0.5*(y_hat - y)^2");
                std::vector<double> steps, losses;
                for (size_t i = 0; i < g_app.bp_result.steps.size(); ++i) {
                    steps.push_back(static_cast<double>(i));
                    losses.push_back(g_app.bp_result.steps[i].loss);
                }
                ImPlot::PlotLine("Loss J", steps.data(), losses.data(), static_cast<int>(steps.size()));
                ImPlot::EndPlot();
            }

            const auto& s0 = g_app.bp_result.steps[0];
            const auto& s100 = g_app.bp_result.steps[100];
            ImGui::Text("Step 0:  Loss = %.6f, y_hat = %.6f, w = %.4f, v = %.4f", s0.loss, s0.y_hat, s0.w, s0.v);
            ImGui::Text("Step 100: Loss = %.6f, y_hat = %.6f, w = %.4f, v = %.4f", s100.loss, s100.y_hat, s100.w, s100.v);
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Click 'Compute 100 Training Steps' to evaluate analytical gradients and plot loss trajectory.");
        }
    }
    ImGui::EndChild();
}

// -----------------------------------------------------------------------------
// WinMain Entry Point
// -----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Populate system info
    g_app.sys_info = wb::get_system_info();

    // Auto-load sample STL and solve initial FEM
    g_app.stl_data = wb::stl_parse_file(g_app.stl_path);
    g_app.stl_loaded = g_app.stl_data.success;
    g_app.fem_result = wb::fem1d_solve(g_app.fem_params);
    g_app.fem_solved = g_app.fem_result.success;

    // Register window class
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"UnifiedWorkbenchClass", nullptr };
    ::RegisterClassExW(&wc);

    // Create application window
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Unified Engineering Workbench — PASS Science Works R&D",
                               WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Engineering dark theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.12f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_Header]   = ImVec4(0.20f, 0.35f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_Button]   = ImVec4(0.18f, 0.30f, 0.48f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.42f, 0.65f, 1.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main event loop
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Top-level full-window docking container
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("MainDockHost", nullptr, host_flags);
        {
            // Header Bar
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "UNIFIED ENGINEERING WORKBENCH");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "| Candidate: Kashyap Gupta | PASS Science Works R&D Internship");
            ImGui::Separator();

            // View Selector Tabs
            if (ImGui::Button("1. Geometry View", ImVec2(170, 32))) g_app.current_tab = AppState::VIEW_GEOMETRY;
            ImGui::SameLine();
            if (ImGui::Button("2. Thermal Analysis", ImVec2(170, 32))) g_app.current_tab = AppState::VIEW_THERMAL;
            ImGui::SameLine();
            if (ImGui::Button("3. Performance View", ImVec2(170, 32))) g_app.current_tab = AppState::VIEW_PERFORMANCE;
            ImGui::SameLine();
            if (ImGui::Button("4. Hardware View", ImVec2(170, 32))) g_app.current_tab = AppState::VIEW_HARDWARE;
            ImGui::SameLine();
            if (ImGui::Button("5. Backpropagation", ImVec2(170, 32))) g_app.current_tab = AppState::VIEW_BACKPROP;

            ImGui::Separator();

            // Render selected view
            switch (g_app.current_tab) {
            case AppState::VIEW_GEOMETRY:    RenderGeometryView(); break;
            case AppState::VIEW_THERMAL:     RenderThermalView(); break;
            case AppState::VIEW_PERFORMANCE: RenderPerformanceView(); break;
            case AppState::VIEW_HARDWARE:    RenderHardwareView(); break;
            case AppState::VIEW_BACKPROP:    RenderBackpropView(); break;
            }

            // Bottom Status Bar
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 28);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Status: %s", g_app.status_message.c_str());
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color[4] = { 0.08f, 0.09f, 0.12f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // VSync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// -----------------------------------------------------------------------------
// Direct3D 11 Initialization with WARP Fallback
// -----------------------------------------------------------------------------
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

    // Try Hardware driver first
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                               featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                               &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) {
        // Fallback to WARP software renderer for maximum compatibility
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
                                           featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                           &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0; // Disable ALT application menu
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
