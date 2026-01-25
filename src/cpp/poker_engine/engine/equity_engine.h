#ifndef ENGINE_EQUITY_ENGINE_H
#define ENGINE_EQUITY_ENGINE_H

#include "core/card.h"
#include "evaluators/naive_evaluator.h"
#include "evaluators/cactus_kev_evaluator.h"
#include "evaluators/ph_evaluator.h"
#include "evaluators/two_plus_two_evaluator.h"
#include "evaluators/omp_eval.h"
#include "equity_result.h"
#include "shared_memory_writer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <atomic>

// Job request (matches Python JobRequest)
struct JobRequest {
    std::unordered_map<std::string, std::vector<Card>> range_spec;
    std::vector<Card> board;
    int num_opponents;
    int num_simulations;
    std::string mode;
    std::string algorithm;
    std::vector<std::string> optimizations;
    int num_workers;
};

class EquityEngine {
private:
    NaiveEvaluator naive_evaluator_;
    CactusKevEvaluator cactus_kev_evaluator_;
    PHEvaluator ph_evaluator_;
    TwoPlusTwoEvaluator tpt_evaluator_;
    OMPEval omp_evaluator_;

    std::string mode_;
    std::unique_ptr<SharedMemoryWriter> shm_writer_;

    std::atomic<uint64_t> simulations_processed_;
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
        std::unordered_map<std::string, EquityResult>& results,
        const std::string& algorithm = "naive",
        int num_workers = 1);
};

#endif // ENGINE_EQUITY_ENGINE_H
