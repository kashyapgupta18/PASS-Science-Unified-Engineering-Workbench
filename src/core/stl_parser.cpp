// =============================================================================
// stl_parser.cpp — ASCII STL File Parser Implementation
// Unified Engineering Workbench
// =============================================================================

#include "wb/stl_parser.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

namespace wb {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static bool is_finite_float(float v) {
    return std::isfinite(v);
}

static float triangle_area(const float v1[3], const float v2[3], const float v3[3]) {
    // Cross product of (v2-v1) x (v3-v1), magnitude / 2
    float ax = v2[0] - v1[0], ay = v2[1] - v1[1], az = v2[2] - v1[2];
    float bx = v3[0] - v1[0], by = v3[1] - v1[1], bz = v3[2] - v1[2];
    float cx = ay * bz - az * by;
    float cy = az * bx - ax * bz;
    float cz = ax * by - ay * bx;
    return 0.5f * std::sqrt(cx * cx + cy * cy + cz * cz);
}

// ---------------------------------------------------------------------------
// Parse ASCII STL from string
// ---------------------------------------------------------------------------
StlData stl_parse_string(const std::string& content, std::atomic<bool>* cancel) {
    StlData result;
    result.bounds_min[0] = result.bounds_min[1] = result.bounds_min[2] =  std::numeric_limits<float>::max();
    result.bounds_max[0] = result.bounds_max[1] = result.bounds_max[2] = -std::numeric_limits<float>::max();

    if (content.empty()) {
        result.error_message = "File is empty";
        return result;
    }

    if (content.size() > STL_MAX_FILE_SIZE) {
        result.error_message = "File exceeds maximum size of 200 MiB";
        return result;
    }

    std::istringstream stream(content);
    std::string line;
    int line_num = 0;

    // Read "solid" header
    if (!std::getline(stream, line)) {
        result.error_message = "Cannot read first line";
        return result;
    }
    ++line_num;
    line = trim(line);

    if (!starts_with(line, "solid")) {
        result.error_message = "Line 1: Expected 'solid' header, got: " + line.substr(0, 40);
        return result;
    }
    result.name = trim(line.substr(5));

    // Parse facets
    while (std::getline(stream, line)) {
        ++line_num;
        if (cancel && cancel->load()) {
            result.error_message = "Parsing cancelled by user";
            return result;
        }

        line = trim(line);
        if (line.empty()) continue;

        if (starts_with(line, "endsolid")) {
            break;
        }

        if (!starts_with(line, "facet normal")) {
            if (starts_with(line, "facet")) {
                result.warnings.push_back("Line " + std::to_string(line_num) +
                    ": Malformed facet line (missing 'normal' keyword)");
                result.error_count++;
            }
            continue;
        }

        if (result.triangles.size() >= STL_MAX_TRIANGLES) {
            result.error_message = "Triangle count exceeds maximum of " +
                std::to_string(STL_MAX_TRIANGLES);
            return result;
        }

        StlTriangle tri;

        // Parse normal
        std::istringstream nss(line.substr(12));
        bool normal_ok = true;
        for (int i = 0; i < 3; ++i) {
            if (!(nss >> tri.normal[i])) { normal_ok = false; break; }
        }
        if (!normal_ok) {
            result.warnings.push_back("Line " + std::to_string(line_num) +
                ": Could not parse facet normal values");
            tri.normal[0] = tri.normal[1] = tri.normal[2] = 0.0f;
            result.error_count++;
        }

        // Expect "outer loop"
        if (!std::getline(stream, line)) break;
        ++line_num;

        // Parse 3 vertices
        float* verts[3] = {tri.v1, tri.v2, tri.v3};
        bool vert_ok = true;
        for (int v = 0; v < 3; ++v) {
            if (!std::getline(stream, line)) { vert_ok = false; break; }
            ++line_num;
            line = trim(line);

            if (!starts_with(line, "vertex")) {
                result.warnings.push_back("Line " + std::to_string(line_num) +
                    ": Expected 'vertex', got: " + line.substr(0, 30));
                result.error_count++;
                vert_ok = false;
                break;
            }

            std::istringstream vss(line.substr(6));
            for (int i = 0; i < 3; ++i) {
                if (!(vss >> verts[v][i])) {
                    result.warnings.push_back("Line " + std::to_string(line_num) +
                        ": Could not parse vertex coordinate " + std::to_string(i) +
                        ". Suggestion: ensure three numeric values after 'vertex'.");
                    result.error_count++;
                    vert_ok = false;
                    break;
                }
                if (!is_finite_float(verts[v][i])) {
                    result.warnings.push_back("Line " + std::to_string(line_num) +
                        ": Non-finite coordinate detected (NaN or Inf)");
                    result.error_count++;
                    vert_ok = false;
                    break;
                }
            }
            if (!vert_ok) break;
        }

        // Read "endloop" and "endfacet"
        std::getline(stream, line); ++line_num;  // endloop
        std::getline(stream, line); ++line_num;  // endfacet

        if (!vert_ok) continue;  // Skip malformed facet

        // Check for degenerate triangle
        float area = triangle_area(tri.v1, tri.v2, tri.v3);
        if (area < 1e-12f) {
            result.warnings.push_back("Triangle " + std::to_string(result.triangles.size()) +
                " near line " + std::to_string(line_num) +
                ": Degenerate triangle (near-zero area)");
            result.error_count++;
        }

        // Update bounding box
        for (int v = 0; v < 3; ++v) {
            const float* p = verts[v];
            for (int i = 0; i < 3; ++i) {
                result.bounds_min[i] = std::min(result.bounds_min[i], p[i]);
                result.bounds_max[i] = std::max(result.bounds_max[i], p[i]);
            }
        }

        result.triangles.push_back(tri);

        if (result.warnings.size() >= 20) {
            result.warnings.push_back("(Further warnings suppressed after 20)");
            break;
        }
    }

    if (result.triangles.empty() && result.error_message.empty()) {
        result.error_message = "No valid triangles found in STL file";
        return result;
    }

    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// Parse ASCII STL from file
// ---------------------------------------------------------------------------
StlData stl_parse_file(const std::string& filepath, std::atomic<bool>* cancel) {
    // Check file size first
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        StlData err;
        err.error_message = "Cannot open file: " + filepath;
        return err;
    }

    auto size = static_cast<size_t>(file.tellg());
    if (size > STL_MAX_FILE_SIZE) {
        StlData err;
        err.error_message = "File size (" + std::to_string(size / (1024 * 1024)) +
                           " MiB) exceeds limit of 200 MiB";
        return err;
    }
    file.seekg(0);

    std::string content(size, '\0');
    file.read(&content[0], static_cast<std::streamsize>(size));
    file.close();

    return stl_parse_string(content, cancel);
}

}  // namespace wb

