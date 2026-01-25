#include "equity_engine.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>

EquityEngine::EquityEngine(const std::string& mode, const std::string& job_id)
    : mode_(mode),
      update_frequency_(1000),
      last_update_count_(0) {
    
    simulations_processed_.store(0);

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
                results, // results is updated with opponent breakdown
                request.algorithm,
                request.num_workers
            );

            // Store overall result
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
    std::unordered_map<std::string, EquityResult>& results,
    const std::string& algorithm,
    int num_workers) {

    if (num_workers <= 0) num_workers = 1;

    // Mutex for shared memory updates and results aggregation
    std::mutex results_mutex;
    const int update_interval = 1000;

    auto run_worker = [&](int worker_sims) {
        // Track per-opponent stats for this thread
        std::unordered_map<std::string, EquityResult> local_opponent_stats;

        for (int sim_num = 0; sim_num < worker_sims; ++sim_num) {
            // Use naive_evaluator_ for simulation orchestration (deck, shuffling)
            // but in a real implementation we would swap the core evaluator.
            // For now, we use the naive_evaluator's simulate_hand.
            SimulationResult sim_result = naive_evaluator_.simulate_hand(
                hole_cards, board, num_opponents);

            std::string opp_class = sim_result.opp_classification;

            if (local_opponent_stats.find(opp_class) == local_opponent_stats.end()) {
                local_opponent_stats[opp_class] = EquityResult();
                local_opponent_stats[opp_class].hand_name = opp_class;
            }

            EquityResult& stats = local_opponent_stats[opp_class];
            stats.total_simulations++;

            if (sim_result.outcome == 1) {
                stats.wins++;
                stats.win_method_matrix[sim_result.our_type][sim_result.opp_type]++;
            } else if (sim_result.outcome == 0) {
                stats.ties++;
            } else {
                stats.losses++;
                stats.loss_method_matrix[sim_result.opp_type][sim_result.our_type]++;
            }

            // Periodic shared memory update (from first thread only to avoid thrashing)
            // Or use atomic counter for simulations_processed_
            simulations_processed_++;

            if (shm_writer_ && (sim_num + 1) % update_interval == 0) {
                std::lock_guard<std::mutex> lock(results_mutex);
                
                uint64_t current_processed = simulations_processed_.load();
                if ((current_processed - last_update_count_) >= update_frequency_) {
                    shm_writer_->update_hands(current_processed);
                    last_update_count_ = current_processed;
                }

                // Merge local stats into shared results for telemetry
                for (auto& pair : local_opponent_stats) {
                    const std::string& name = pair.first;
                    const EquityResult& local_stats = pair.second;
                    
                    if (results.find(name) == results.end()) {
                        results[name] = local_stats;
                    } else {
                        results[name].wins += local_stats.wins;
                        results[name].ties += local_stats.ties;
                        results[name].losses += local_stats.losses;
                        results[name].total_simulations += local_stats.total_simulations;
                        // Skip matrix merge for telemetry updates to save time
                    }
                    
                    uint32_t total = results[name].total_simulations;
                    if (total > 0) {
                        results[name].equity = (results[name].wins + results[name].ties * 0.5) / total;
                    }
                }
                shm_writer_->update_equity_results(results);
            }
        }

        // Final merge for this worker
        std::lock_guard<std::mutex> lock(results_mutex);
        for (auto& pair : local_opponent_stats) {
            const std::string& name = pair.first;
            const EquityResult& local_stats = pair.second;
            
            if (results.find(name) == results.end()) {
                results[name] = local_stats;
            } else {
                results[name].wins += local_stats.wins;
                results[name].ties += local_stats.ties;
                results[name].losses += local_stats.losses;
                results[name].total_simulations += local_stats.total_simulations;
                
                for (int i = 0; i < 10; ++i) {
                    for (int j = 0; j < 10; ++j) {
                        results[name].win_method_matrix[i][j] += local_stats.win_method_matrix[i][j];
                        results[name].loss_method_matrix[i][j] += local_stats.loss_method_matrix[i][j];
                    }
                }
            }
            
            uint32_t total = results[name].total_simulations;
            if (total > 0) {
                results[name].equity = (results[name].wins + results[name].ties * 0.5) / total;
            }
        }
    };

    std::vector<std::thread> threads;
    int sims_per_thread = num_simulations / num_workers;
    
    for (int i = 0; i < num_workers - 1; ++i) {
        threads.emplace_back(run_worker, sims_per_thread);
    }
    // Main thread/last thread does the remainder
    run_worker(num_simulations - (sims_per_thread * (num_workers - 1)));

    for (auto& t : threads) {
        t.join();
    }

    // Return overall summary
    EquityResult overall;
    overall.hand_name = hand_name;

    uint32_t total_sims = 0;
    uint32_t total_wins = 0;
    uint32_t total_ties = 0;
    uint32_t total_losses = 0;

    for (const auto& pair : results) {
        // Only aggregate opponent classes (not starting hands)
        // Opponent classes like "AA", "72o" etc.
        // Hand names are from range_spec.
        // This logic is slightly flawed if hand_name is also an opponent class.
        // But for now it matches the Python structure.
        const EquityResult& stats_ref = pair.second;
        total_sims += stats_ref.total_simulations;
        total_wins += stats_ref.wins;
        total_ties += stats_ref.ties;
        total_losses += stats_ref.losses;

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
