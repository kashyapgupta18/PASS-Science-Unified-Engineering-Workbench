// =============================================================================
// stl_parser.h — ASCII STL File Parser
// Unified Engineering Workbench
// =============================================================================
// Parses ASCII STL files containing triangulated surface meshes. Validates
// geometry, detects degenerate triangles, and computes bounding boxes.
// =============================================================================

#ifndef WB_STL_PARSER_H
#define WB_STL_PARSER_H

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace wb {

/// A single triangle facet from an STL file.
struct StlTriangle {
    float normal[3];  ///< Facet normal vector (ni, nj, nk)
    float v1[3];      ///< First vertex (x, y, z)
    float v2[3];      ///< Second vertex (x, y, z)
    float v3[3];      ///< Third vertex (x, y, z)
};

/// Parsed STL data with validation results.
struct StlData {
    std::string name;                    ///< Solid name from header
    std::vector<StlTriangle> triangles;  ///< Parsed triangles
    float bounds_min[3];                 ///< Bounding box minimum (x, y, z)
    float bounds_max[3];                 ///< Bounding box maximum (x, y, z)
    bool success = false;                ///< True if parsing completed
    std::string error_message;           ///< Fatal error description
    int error_count = 0;                 ///< Number of non-fatal errors
    std::vector<std::string> warnings;   ///< Warning/info messages
};

/// Maximum allowed triangle count (2 million).
constexpr size_t STL_MAX_TRIANGLES = 2'000'000;

/// Maximum allowed file size (200 MiB).
constexpr size_t STL_MAX_FILE_SIZE = 200ULL * 1024 * 1024;

/// Parse an ASCII STL file.
/// @param filepath   Path to the STL file
/// @param cancel     Optional cancellation flag (set to true to abort)
/// @return           StlData with parsed triangles and validation info
StlData stl_parse_file(const std::string& filepath,
                       std::atomic<bool>* cancel = nullptr);

/// Parse an ASCII STL from a string buffer.
/// @param content    STL file content as string
/// @param cancel     Optional cancellation flag
/// @return           StlData with parsed triangles and validation info
StlData stl_parse_string(const std::string& content,
                         std::atomic<bool>* cancel = nullptr);

}  // namespace wb

#endif  // WB_STL_PARSER_H
