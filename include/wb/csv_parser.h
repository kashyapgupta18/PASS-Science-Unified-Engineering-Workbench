// =============================================================================
// csv_parser.h — Temperature CSV Parser
// Unified Engineering Workbench
// =============================================================================
// Parses CSV files containing nodal temperature data with schema:
//   node_id, x_m, y_m, z_m, temperature_C
// Validates data integrity and computes statistics.
// =============================================================================

#ifndef WB_CSV_PARSER_H
#define WB_CSV_PARSER_H

#include <cstdint>
#include <string>
#include <vector>

namespace wb {

/// A single temperature node from a CSV file.
struct TempNode {
    int    node_id;      ///< Unique node identifier
    double x;            ///< X coordinate [m]
    double y;            ///< Y coordinate [m]
    double z;            ///< Z coordinate [m]
    double temperature;  ///< Temperature [°C]
};

/// Parsed CSV data with validation results and statistics.
struct CsvData {
    std::vector<TempNode> nodes;         ///< Parsed temperature nodes
    double temp_min  = 0.0;              ///< Minimum temperature [°C]
    double temp_max  = 0.0;              ///< Maximum temperature [°C]
    double temp_mean = 0.0;              ///< Mean temperature [°C]
    double bounds_min[3] = {0, 0, 0};    ///< Spatial bounding box minimum
    double bounds_max[3] = {0, 0, 0};    ///< Spatial bounding box maximum
    bool   success = false;              ///< True if parsing completed
    std::string error_message;           ///< Fatal error description
    int    error_count = 0;              ///< Number of non-fatal errors
    std::vector<std::string> warnings;   ///< Warning/info messages
};

/// Maximum allowed row count (5 million).
constexpr size_t CSV_MAX_ROWS = 5'000'000;

/// Maximum allowed file size (200 MiB).
constexpr size_t CSV_MAX_FILE_SIZE = 200ULL * 1024 * 1024;

/// Expected CSV header string.
constexpr const char* CSV_EXPECTED_HEADER = "node_id,x_m,y_m,z_m,temperature_C";

/// Parse a temperature CSV file.
/// @param filepath  Path to the CSV file
/// @return          CsvData with parsed nodes and validation info
CsvData csv_parse_file(const std::string& filepath);

/// Parse temperature CSV from a string buffer.
/// @param content   CSV content as string
/// @return          CsvData with parsed nodes and validation info
CsvData csv_parse_string(const std::string& content);

}  // namespace wb

#endif  // WB_CSV_PARSER_H
