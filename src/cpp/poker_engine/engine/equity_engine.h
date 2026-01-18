#ifndef ENGINE_EQUITY_ENGINE_H
#define ENGINE_EQUITY_ENGINE_H

#include "core/card.h"
#include "evaluators/naive_evaluator.h"
#include "equity_result.h"
#include "shared_memory_writer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

// Job request (matches Python JobRequest)
struct JobRequest {
    std::unordered_map<std::string, std::vector<Card>> range_spec;
    std::vector<Card> board;
    int num_opponents;
    int num_simulations;
    std::string mode;
};

class EquityEngine {
private:
    NaiveEvaluator evaluator_;
    std::string mode_;
    std::unique_ptr<SharedMemoryWriter> shm_writer_;

    uint64_t simulations_processed_;
    uint64_t update_frequency_;
    uint64_t last_update_count_;

    std::function<void(double, const std::unordered_map<std::string, double>&)> progress_callback_;

public:
    EquityEngine(const std::string& mode, const std::string& job_id = "");

    // Set progress callback
    void set_progress_callback(
        std::function<void(double, const std::unordered_map<std::string, double>&)> callback);

    // Main entry point (matches Python: calculate_range_equity)
    std::unordered_map<std::string, EquityResult> calculate_range_equity(
        const JobRequest& request);

private:
    // Calculate equity for single hand against opponent range
    // Matches: src/python/engine/engine.py:93-213
    EquityResult calculate_hand_equity(
        const std::vector<Card>& hole_cards,
        const std::vector<Card>& board,
        int num_opponents,
        int num_simulations,
        const std::string& hand_name,
        std::unordered_map<std::string, EquityResult>& results);
};

#endif // ENGINE_EQUITY_ENGINE_H
