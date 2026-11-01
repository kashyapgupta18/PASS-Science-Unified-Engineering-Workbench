// =============================================================================
// csv_parser.cpp — Temperature CSV Parser Implementation
// Unified Engineering Workbench
// =============================================================================

#include "wb/csv_parser.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
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

// ---------------------------------------------------------------------------
// Parse CSV from string
// ---------------------------------------------------------------------------
CsvData csv_parse_string(const std::string& content) {
    CsvData result;

    if (content.empty()) {
        result.error_message = "File is empty";
        return result;
    }

    if (content.size() > CSV_MAX_FILE_SIZE) {
        result.error_message = "File exceeds maximum size of 200 MiB";
        return result;
    }

    std::istringstream stream(content);
    std::string line;
    int line_num = 0;

    // --- Read and validate header -------------------------------------------
    if (!std::getline(stream, line)) {
        result.error_message = "Cannot read header line";
        return result;
    }
    ++line_num;
    std::string header = trim(line);

    if (header != CSV_EXPECTED_HEADER) {
        result.error_message = "Header mismatch. Expected: '" +
            std::string(CSV_EXPECTED_HEADER) + "', got: '" + header + "'";
        return result;
    }

    // --- Parse data rows ----------------------------------------------------
    std::set<int> seen_ids;
    double temp_sum = 0.0;
    result.bounds_min[0] = result.bounds_min[1] = result.bounds_min[2] =  std::numeric_limits<double>::max();
    result.bounds_max[0] = result.bounds_max[1] = result.bounds_max[2] = -std::numeric_limits<double>::max();
    result.temp_min =  std::numeric_limits<double>::max();
    result.temp_max = -std::numeric_limits<double>::max();

    while (std::getline(stream, line)) {
        ++line_num;
        line = trim(line);
        if (line.empty()) continue;

        if (result.nodes.size() >= CSV_MAX_ROWS) {
            result.error_message = "Row count exceeds maximum of " +
                std::to_string(CSV_MAX_ROWS);
            return result;
        }

        // Split by comma
        std::istringstream row(line);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(row, token, ',')) {
            fields.push_back(trim(token));
        }

        if (fields.size() < 5) {
            result.warnings.push_back("Line " + std::to_string(line_num) +
                ": Expected 5 columns, got " + std::to_string(fields.size()) +
                ". Suggestion: format as node_id,x_m,y_m,z_m,temperature_C");
            result.error_count++;
            if (result.warnings.size() >= 20) break;
            continue;
        }

        TempNode node;
        try {
            node.node_id = std::stoi(fields[0]);
            node.x = std::stod(fields[1]);
            node.y = std::stod(fields[2]);
            node.z = std::stod(fields[3]);
            node.temperature = std::stod(fields[4]);
        } catch (const std::exception&) {
            result.warnings.push_back("Line " + std::to_string(line_num) +
                ": Could not parse numeric values. Suggestion: check for "
                "non-numeric characters.");
            result.error_count++;
            if (result.warnings.size() >= 20) break;
            continue;
        }

        // Check for non-finite values
        if (!std::isfinite(node.x) || !std::isfinite(node.y) ||
            !std::isfinite(node.z) || !std::isfinite(node.temperature)) {
            result.warnings.push_back("Line " + std::to_string(line_num) +
                ": Non-finite value detected (NaN or Inf). Row skipped.");
            result.error_count++;
            if (result.warnings.size() >= 20) break;
            continue;
        }

        // Check for duplicate IDs
        if (seen_ids.count(node.node_id)) {
            result.warnings.push_back("Line " + std::to_string(line_num) +
                ": Duplicate node_id " + std::to_string(node.node_id));
            result.error_count++;
        }
        seen_ids.insert(node.node_id);

        // Update statistics
        result.temp_min = std::min(result.temp_min, node.temperature);
        result.temp_max = std::max(result.temp_max, node.temperature);
        temp_sum += node.temperature;

        double coords[3] = {node.x, node.y, node.z};
        for (int i = 0; i < 3; ++i) {
            result.bounds_min[i] = std::min(result.bounds_min[i], coords[i]);
            result.bounds_max[i] = std::max(result.bounds_max[i], coords[i]);
        }

        result.nodes.push_back(node);

        if (result.warnings.size() >= 20) {
            result.warnings.push_back("(Further warnings suppressed after 20)");
            break;
        }
    }

    if (result.nodes.empty()) {
        result.error_message = "No valid data rows found";
        return result;
    }

    result.temp_mean = temp_sum / static_cast<double>(result.nodes.size());
    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// Parse CSV from file
// ---------------------------------------------------------------------------
CsvData csv_parse_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        CsvData err;
        err.error_message = "Cannot open file: " + filepath;
        return err;
    }

    auto size = static_cast<size_t>(file.tellg());
    if (size > CSV_MAX_FILE_SIZE) {
        CsvData err;
        err.error_message = "File size (" + std::to_string(size / (1024 * 1024)) +
                           " MiB) exceeds limit of 200 MiB";
        return err;
    }
    file.seekg(0);

    std::string content(size, '\0');
    file.read(&content[0], static_cast<std::streamsize>(size));
    file.close();

    return csv_parse_string(content);
}

}  // namespace wb

