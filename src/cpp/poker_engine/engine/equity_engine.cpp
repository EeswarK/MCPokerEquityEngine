#include "equity_engine.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include "core/deck.h"

namespace poker_engine {

EquityEngine::EquityEngine(const std::string& mode, const std::string& job_id)
    : mode_(mode),
      update_frequency_(1000),
      last_update_count_(0) {
    
    simulations_processed_.store(0);

    // Create shared memory writer if job_id provided
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

std::unordered_map<std::string, EquityResult> EquityEngine::calculate_range_equity(
    const JobRequest& request) {

    std::unordered_map<std::string, EquityResult> results;
    simulations_processed_ = 0;
    last_update_count_ = 0;

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

            EquityResult overall = calculate_hand_equity(
                request,
                hand_name,
                simulations_per_hand,
                results
            );

            results[hand_name] = overall;

            if (shm_writer_) {
                uint64_t expected_total = (idx + 1) * simulations_per_hand;
                if (simulations_processed_ < expected_total) {
                    simulations_processed_ = expected_total;
                }
                shm_writer_->update_hands(simulations_processed_);
                shm_writer_->update_equity_results(results);
            }

            double progress = static_cast<double>(idx + 1) / total_hands;
            if (progress_callback_) {
                std::unordered_map<std::string, double> current_results;
                for (const auto& pair : results) {
                    current_results[pair.first] = pair.second.equity;
                }
                progress_callback_(progress, current_results);
            }
        }

        if (shm_writer_) {
            shm_writer_->update_hands(simulations_processed_);
            shm_writer_->set_status(1);  // Completed
            shm_writer_->close();
        }

    } catch (...) {
        if (shm_writer_) {
            shm_writer_->set_status(2);  // Failed
            shm_writer_->close();
        }
        throw;
    }

    return results;
}

EquityResult EquityEngine::calculate_hand_equity(
    const JobRequest& request,
    const std::string& hand_name,
    int simulations_per_hand,
    std::unordered_map<std::string, EquityResult>& results) {

  // Check if MULTITHREADING optimization is enabled
  auto has_multithreading = std::any_of(request.optimizations.begin(), request.optimizations.end(),
                                         [](const std::string& opt) {
                                           std::string opt_lower = opt;
                                           std::transform(opt_lower.begin(), opt_lower.end(), opt_lower.begin(), ::tolower);
                                           return opt_lower == "multithreading";
                                         });

  // Only use multiple workers if MULTITHREADING optimization is enabled
  int num_workers = (has_multithreading && request.num_workers > 0) ? request.num_workers : 1;
  std::mutex results_mutex;
  const int update_interval = 1000;

  const std::vector<Card>& hole_cards = request.range_spec.at(hand_name);

  auto run_worker = [&](int worker_sims) {
    std::unordered_map<std::string, EquityResult> local_opponent_stats;
    Deck deck;

    // Check if SIMD optimization is enabled (case-insensitive)
    std::string algo_lower = request.algorithm;
    std::transform(algo_lower.begin(), algo_lower.end(), algo_lower.begin(), ::tolower);

    auto has_simd = std::any_of(request.optimizations.begin(), request.optimizations.end(),
                                 [](const std::string& opt) {
                                   std::string opt_lower = opt;
                                   std::transform(opt_lower.begin(), opt_lower.end(), opt_lower.begin(), ::tolower);
                                   return opt_lower == "simd";
                                 });

    bool use_simd = ((algo_lower == "omp" || algo_lower == "omp_eval") && has_simd);

    int sim_num = 0;
    while (sim_num < worker_sims) {
      if (use_simd && (worker_sims - sim_num) >= SIMDConfig::kBatchSize) {
        // SIMD Path: Process 8 simulations in a batch
        HandBatch our_batch;
        HandBatch opp_batch;
        std::vector<std::string> opp_classes(SIMDConfig::kBatchSize);

        for (int b = 0; b < SIMDConfig::kBatchSize; ++b) {
          deck.reset();
          for (const auto& card : hole_cards) deck.remove(card);
          for (const auto& card : request.board) deck.remove(card);

          std::vector<Card> board_cards = request.board;
          int remaining_board = 5 - static_cast<int>(request.board.size());
          for (int i = 0; i < remaining_board; ++i) {
            board_cards.push_back(deck.draw_random());
          }

          std::vector<std::vector<Card>> opponent_hands;
          for (int i = 0; i < request.num_opponents; ++i) {
            opponent_hands.push_back(deck.sample(2));
          }

          size_t opp_idx = 0;  // Assume 1 opponent for batching test
          const std::vector<Card>& opp_hand = opponent_hands[opp_idx];
          opp_classes[b] = naive_evaluator_.classify_hole_cards(opp_hand);

          // Pack into SoA HandBatch
          for (int i = 0; i < 2; ++i) {
            our_batch.ranks[i][b] = hole_cards[i].rank;
            our_batch.suits[i][b] = hole_cards[i].suit;
            opp_batch.ranks[i][b] = opp_hand[i].rank;
            opp_batch.suits[i][b] = opp_hand[i].suit;
          }
          for (int i = 0; i < 5; ++i) {
            our_batch.ranks[i + 2][b] = board_cards[i].rank;
            our_batch.suits[i + 2][b] = board_cards[i].suit;
            opp_batch.ranks[i + 2][b] = board_cards[i].rank;
            opp_batch.suits[i + 2][b] = board_cards[i].suit;
          }
        }

        int32_t our_results[SIMDConfig::kBatchSize];
        int32_t opp_results[SIMDConfig::kBatchSize];
        omp_evaluator_.evaluate_batch(our_batch, our_results);
        omp_evaluator_.evaluate_batch(opp_batch, opp_results);

        for (int b = 0; b < SIMDConfig::kBatchSize; ++b) {
          std::string& opp_class = opp_classes[b];
          if (local_opponent_stats.find(opp_class) ==
              local_opponent_stats.end()) {
            local_opponent_stats[opp_class] = EquityResult();
            local_opponent_stats[opp_class].hand_name = opp_class;
          }
          EquityResult& stats = local_opponent_stats[opp_class];
          stats.total_simulations++;

          if (our_results[b] > opp_results[b]) {
            stats.wins++;
            stats.win_method_matrix[get_hand_type(our_results[b])]
                                   [get_hand_type(opp_results[b])]++;
          } else if (our_results[b] == opp_results[b]) {
            stats.ties++;
          } else {
            stats.losses++;
            stats.loss_method_matrix[get_hand_type(opp_results[b])]
                                    [get_hand_type(our_results[b])]++;
          }
        }
        sim_num += SIMDConfig::kBatchSize;
        simulations_processed_ += SIMDConfig::kBatchSize;
      } else {
        // Scalar Path
        deck.reset();
        for (const auto& card : hole_cards) deck.remove(card);
        for (const auto& card : request.board) deck.remove(card);

        std::vector<Card> board_cards = request.board;
        int remaining_board = 5 - static_cast<int>(request.board.size());
        for (int i = 0; i < remaining_board; ++i) {
          board_cards.push_back(deck.draw_random());
        }

        std::vector<std::vector<Card>> opponent_hands;
        for (int i = 0; i < request.num_opponents; ++i) {
          opponent_hands.push_back(deck.sample(2));
        }

        int32_t our_value =
            evaluate_with_algorithm(request.algorithm, hole_cards, board_cards);

        int32_t max_opponent = 0;
        size_t max_opp_idx = 0;
        for (size_t i = 0; i < opponent_hands.size(); ++i) {
          int32_t val = evaluate_with_algorithm(request.algorithm,
                                                 opponent_hands[i], board_cards);
          if (val > max_opponent) {
            max_opponent = val;
            max_opp_idx = i;
          }
        }

        std::string opp_class =
            opponent_hands.empty()
                ? "??"
                : naive_evaluator_.classify_hole_cards(opponent_hands[max_opp_idx]);

        if (local_opponent_stats.find(opp_class) == local_opponent_stats.end()) {
          local_opponent_stats[opp_class] = EquityResult();
          local_opponent_stats[opp_class].hand_name = opp_class;
        }

        EquityResult& stats = local_opponent_stats[opp_class];
        stats.total_simulations++;

        if (our_value > max_opponent) {
          stats.wins++;
          stats.win_method_matrix[get_hand_type(our_value)]
                                 [get_hand_type(max_opponent)]++;
        } else if (our_value == max_opponent) {
          stats.ties++;
        } else {
          stats.losses++;
          stats.loss_method_matrix[get_hand_type(max_opponent)]
                                  [get_hand_type(our_value)]++;
        }

        sim_num++;
        simulations_processed_++;
      }

      if (shm_writer_ && (sim_num % update_interval == 0)) {
        std::lock_guard<std::mutex> lock(results_mutex);
        uint64_t current_processed = simulations_processed_.load();
        if ((current_processed - last_update_count_) >= update_frequency_) {
          shm_writer_->update_hands(current_processed);
          last_update_count_ = current_processed;
        }

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
          }
          uint32_t total = results[name].total_simulations;
          if (total > 0) {
            results[name].equity =
                (results[name].wins + results[name].ties * 0.5) / total;
          }
        }
        shm_writer_->update_equity_results(results);
      }
    }

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
            results[name].win_method_matrix[i][j] +=
                local_stats.win_method_matrix[i][j];
            results[name].loss_method_matrix[i][j] +=
                local_stats.loss_method_matrix[i][j];
          }
        }
      }
      uint32_t total = results[name].total_simulations;
      if (total > 0) {
        results[name].equity =
            (results[name].wins + results[name].ties * 0.5) / total;
      }
    }
  };

  std::vector<std::thread> threads;
  int sims_per_thread = simulations_per_hand / num_workers;
  for (int i = 0; i < num_workers - 1; ++i) {
    threads.emplace_back(run_worker, sims_per_thread);
  }
  run_worker(simulations_per_hand - (sims_per_thread * (num_workers - 1)));

  for (auto& t : threads) t.join();

  EquityResult overall;
  overall.hand_name = hand_name;
  uint32_t total_sims = 0, total_wins = 0, total_ties = 0, total_losses = 0;

  for (const auto& pair : results) {
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
    overall.equity =
        static_cast<double>(total_wins + total_ties * 0.5) / total_sims;
  }

  return overall;
}

int32_t EquityEngine::evaluate_with_algorithm(const std::string& algorithm,
                                               const std::vector<Card>& hole,
                                               const std::vector<Card>& board) const {
    // Support both lowercase and uppercase enum values from frontend
    std::string algo_lower = algorithm;
    std::transform(algo_lower.begin(), algo_lower.end(), algo_lower.begin(), ::tolower);

    if (algo_lower == "cactus_kev") return cactus_kev_evaluator_.evaluate_hand(hole, board);
    if (algo_lower == "ph_evaluator" || algo_lower == "perfect_hash") return ph_evaluator_.evaluate_hand(hole, board);
    if (algo_lower == "two_plus_two") return tpt_evaluator_.evaluate_hand(hole, board);
    if (algo_lower == "omp_eval" || algo_lower == "omp") return omp_evaluator_.evaluate_hand(hole, board);
    if (algo_lower == "naive") return naive_evaluator_.evaluate_hand(hole, board);

    // Default to naive if unknown algorithm
    return naive_evaluator_.evaluate_hand(hole, board);
}

}  // namespace poker_engine
