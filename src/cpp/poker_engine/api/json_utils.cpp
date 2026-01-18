#include "json_utils.h"
#include <sstream>
#include <iomanip>

bool parse_create_job_request(
    const std::string& json_str,
    std::unordered_map<std::string, std::vector<Card>>& range_spec,
    std::vector<Card>& board,
    int& num_opponents,
    int& num_simulations,
    std::string& mode,
    int& num_workers) {

    using namespace rapidjson;

    Document doc;
    if (doc.Parse(json_str.c_str()).HasParseError()) {
        return false;
    }

    // Parse range_spec (required)
    if (!doc.HasMember("range_spec") || !doc["range_spec"].IsObject()) {
        return false;
    }

    const Value& range_obj = doc["range_spec"];
    for (auto it = range_obj.MemberBegin(); it != range_obj.MemberEnd(); ++it) {
        std::string hand_name = it->name.GetString();
        const Value& cards_arr = it->value;

        std::vector<Card> cards;
        for (SizeType i = 0; i < cards_arr.Size(); ++i) {
            const Value& card_obj = cards_arr[i];
            uint8_t rank = card_obj["rank"].GetInt();
            uint8_t suit = card_obj["suit"].GetInt();
            cards.push_back(Card(rank, suit));
        }
        range_spec[hand_name] = cards;
    }

    // Parse board (optional, default empty)
    if (doc.HasMember("board") && doc["board"].IsArray()) {
        const Value& board_arr = doc["board"];
        for (SizeType i = 0; i < board_arr.Size(); ++i) {
            const Value& card_obj = board_arr[i];
            uint8_t rank = card_obj["rank"].GetInt();
            uint8_t suit = card_obj["suit"].GetInt();
            board.push_back(Card(rank, suit));
        }
    }

    // Parse other fields
    num_opponents = doc.HasMember("num_opponents") ? doc["num_opponents"].GetInt() : 1;
    num_simulations = doc.HasMember("num_simulations") ? doc["num_simulations"].GetInt() : 100000;
    mode = doc.HasMember("mode") ? doc["mode"].GetString() : "cpp_naive";
    num_workers = doc.HasMember("num_workers") ? doc["num_workers"].GetInt() : 0;

    return true;
}

std::string serialize_create_job_response(
    const std::string& job_id,
    const std::string& status,
    const std::string& created_at,
    const std::string& telemetry_ws_url) {

    using namespace rapidjson;

    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("job_id", Value(job_id.c_str(), allocator), allocator);
    doc.AddMember("status", Value(status.c_str(), allocator), allocator);
    doc.AddMember("created_at", Value(created_at.c_str(), allocator), allocator);
    doc.AddMember("telemetry_ws_url", Value(telemetry_ws_url.c_str(), allocator), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

std::string serialize_job_status_response(const JobState& state) {
    using namespace rapidjson;

    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    // Convert status enum to string
    const char* status_str;
    switch (state.status) {
        case JobStatus::PENDING: status_str = "pending"; break;
        case JobStatus::RUNNING: status_str = "running"; break;
        case JobStatus::COMPLETED: status_str = "completed"; break;
        case JobStatus::FAILED: status_str = "failed"; break;
        default: status_str = "unknown";
    }

    // Format timestamps as ISO 8601
    auto format_time = [](const std::chrono::system_clock::time_point& tp) -> std::string {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    };

    doc.AddMember("job_id", Value(state.job_id.c_str(), allocator), allocator);
    doc.AddMember("status", Value(status_str, allocator), allocator);
    doc.AddMember("progress", state.progress, allocator);
    doc.AddMember("created_at", Value(format_time(state.created_at).c_str(), allocator), allocator);

    if (state.status == JobStatus::COMPLETED || state.status == JobStatus::FAILED) {
        doc.AddMember("completed_at", Value(format_time(state.completed_at).c_str(), allocator), allocator);
    }

    if (!state.error.empty()) {
        doc.AddMember("error", Value(state.error.c_str(), allocator), allocator);
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

std::string serialize_health_response() {
    return R"({"status":"healthy","version":"0.1.0"})";
}

std::string serialize_error_response(const std::string& message) {
    using namespace rapidjson;

    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("detail", Value(message.c_str(), allocator), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}
