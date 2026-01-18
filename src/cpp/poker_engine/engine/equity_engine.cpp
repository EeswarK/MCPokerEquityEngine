#include "equity_engine.h"
#include <iostream>
#include <algorithm>

EquityEngine::EquityEngine(const std::string& mode, const std::string& job_id)
    : mode_(mode),
      simulations_processed_(0),
      update_frequency_(1000),
      last_update_count_(0) {

    // Create shared memory writer if job_id provided (Python: engine.py:23-30)
    if (!job_id.empty()) {
        shm_writer_ = std::make_unique<SharedMemoryWriter>(job_id);
        if (!shm_writer_->create()) {
            std::cerr << "Failed to create shared memory for job " << job_id << std::endl;
            shm_writer_.reset();
        }
    }
}

void EquityEngine::set_progress_callback(
    std::function<void(double, const std::unordered_map<std::string, double>&)> callback) {
    progress_callback_ = callback;
}

// Matches: src/python/engine/engine.py:42-91
std::unordered_map<std::string, EquityResult> EquityEngine::calculate_range_equity(
    const JobRequest& request) {

    std::unordered_map<std::string, EquityResult> results;
    simulations_processed_ = 0;
    last_update_count_ = 0;

    // Extract hand names (Python line 47)
    std::vector<std::string> hand_names;
    for (const auto& pair : request.range_spec) {
        hand_names.push_back(pair.first);
    }

    size_t total_hands = hand_names.size();
    if (total_hands == 0) return results;

    int simulations_per_hand = request.num_simulations / total_hands;

    try {
        for (size_t idx = 0; idx < hand_names.size(); ++idx) {
            const std::string& hand_name = hand_names[idx];
            const std::vector<Card>& hole_cards = request.range_spec.at(hand_name);

            // Calculate equity for this hand (Python lines 54-61)
            EquityResult overall = calculate_hand_equity(
                hole_cards,
                request.board,
                request.num_opponents,
                simulations_per_hand,
                hand_name,
                results
            );

            // Store overall result (FIX: This was missing)
            results[hand_name] = overall;

            // Final update for this hand (Python lines 68-74)
            if (shm_writer_) {
                uint64_t expected_total = (idx + 1) * simulations_per_hand;
                if (simulations_processed_ < expected_total) {
                    simulations_processed_ = expected_total;
                }
                shm_writer_->update_hands(simulations_processed_);
                shm_writer_->update_equity_results(results);
            }

            // Report progress (Python lines 76-78)
            double progress = static_cast<double>(idx + 1) / total_hands;
            if (progress_callback_) {
                std::unordered_map<std::string, double> current_results;
                for (const auto& pair : results) {
                    current_results[pair.first] = pair.second.equity;
                }
                progress_callback_(progress, current_results);
            }
        }

        // Mark as completed (Python lines 80-83)
        if (shm_writer_) {
            shm_writer_->update_hands(simulations_processed_);
            shm_writer_->set_status(1);  // Completed
            shm_writer_->close();
        }

    } catch (...) {
        // Mark as failed (Python lines 85-89)
        if (shm_writer_) {
            shm_writer_->set_status(2);  // Failed
            shm_writer_->close();
        }
        throw;
    }

    return results;
}

// Matches: src/python/engine/engine.py:93-213
EquityResult EquityEngine::calculate_hand_equity(
    const std::vector<Card>& hole_cards,
    const std::vector<Card>& board,
    int num_opponents,
    int num_simulations,
    const std::string& hand_name,
    std::unordered_map<std::string, EquityResult>& results) {

    // Track per-opponent stats (Python line 104)
    std::unordered_map<std::string, EquityResult> opponent_stats;

    const int update_interval = 1000;  // Python line 105

    // Run simulations (Python line 107)
    for (int sim_num = 0; sim_num < num_simulations; ++sim_num) {
        SimulationResult sim_result = evaluator_.simulate_hand(
            hole_cards, board, num_opponents);

        std::string opp_class = sim_result.opp_classification;

        // Initialize stats if first time seeing this opponent (Python lines 111-119)
        if (opponent_stats.find(opp_class) == opponent_stats.end()) {
            opponent_stats[opp_class] = EquityResult();
            opponent_stats[opp_class].hand_name = opp_class;
        }

        EquityResult& stats = opponent_stats[opp_class];
        stats.total_simulations++;

        // Update counters and matrices (Python lines 124-131)
        if (sim_result.outcome == 1) {
            stats.wins++;
            stats.win_method_matrix[sim_result.our_type][sim_result.opp_type]++;
        } else if (sim_result.outcome == 0) {
            stats.ties++;
        } else {  // Loss
            stats.losses++;
            stats.loss_method_matrix[sim_result.opp_type][sim_result.our_type]++;  // Reversed
        }

        // Periodic shared memory update (Python lines 134-157)
        if (shm_writer_ && (sim_num + 1) % update_interval == 0) {
            simulations_processed_ += update_interval;

            if ((simulations_processed_ - last_update_count_) >= update_frequency_) {
                shm_writer_->update_hands(simulations_processed_);
                last_update_count_ = simulations_processed_;
            }

            // Calculate current equity for each opponent type
            for (auto& pair : opponent_stats) {
                EquityResult& stats_ref = pair.second;
                uint32_t total = stats_ref.total_simulations;
                if (total > 0) {
                    stats_ref.equity = (stats_ref.wins + stats_ref.ties * 0.5) / total;
                }
            }

            // Update results dict (for shared memory write)
            for (const auto& pair : opponent_stats) {
                results[pair.first] = pair.second;
            }

            shm_writer_->update_equity_results(results);
        }
    }

    // Finalize opponent-specific results (Python lines 159-184)
    for (auto& pair : opponent_stats) {
        EquityResult& stats_ref = pair.second;
        uint32_t total = stats_ref.total_simulations;
        if (total > 0) {
            stats_ref.equity = (stats_ref.wins + stats_ref.ties * 0.5) / total;
        }
        results[pair.first] = stats_ref;
    }

    // Return overall summary (Python lines 186-213)
    EquityResult overall;
    overall.hand_name = hand_name;

    uint32_t total_sims = 0;
    uint32_t total_wins = 0;
    uint32_t total_ties = 0;
    uint32_t total_losses = 0;

    for (const auto& pair : opponent_stats) {
        const EquityResult& stats_ref = pair.second;
        total_sims += stats_ref.total_simulations;
        total_wins += stats_ref.wins;
        total_ties += stats_ref.ties;
        total_losses += stats_ref.losses;

        // Aggregate matrices
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                overall.win_method_matrix[i][j] += stats_ref.win_method_matrix[i][j];
                overall.loss_method_matrix[i][j] += stats_ref.loss_method_matrix[i][j];
            }
        }
    }

    overall.total_simulations = total_sims;
    overall.wins = total_wins;
    overall.ties = total_ties;
    overall.losses = total_losses;

    if (total_sims > 0) {
        overall.equity = static_cast<double>(total_wins + total_ties * 0.5) / total_sims;
    }

    return overall;
}
