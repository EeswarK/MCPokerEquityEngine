#ifndef API_JSON_UTILS_H
#define API_JSON_UTILS_H

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "core/card.h"
#include "engine/equity_result.h"
#include "job_manager.h"
#include <string>
#include <vector>

// Parse POST /api/jobs request body
// Matches: src/python/api/models.py:22-36
bool parse_create_job_request(
    const std::string& json_str,
    std::unordered_map<std::string, std::vector<Card>>& range_spec,
    std::vector<Card>& board,
    int& num_opponents,
    int& num_simulations,
    std::string& mode,
    int& num_workers);

// Serialize CreateJobResponse
// Matches: src/python/api/models.py:38-42
std::string serialize_create_job_response(
    const std::string& job_id,
    const std::string& status,
    const std::string& created_at,
    const std::string& telemetry_ws_url);

// Serialize JobStatusResponse
// Matches: src/python/api/models.py:45-51
std::string serialize_job_status_response(const JobState& state);

// Serialize health check response
std::string serialize_health_response();

// Serialize error response
std::string serialize_error_response(const std::string& message);

#endif // API_JSON_UTILS_H
