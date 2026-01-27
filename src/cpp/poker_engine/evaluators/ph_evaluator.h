#ifndef EVALUATORS_PH_EVALUATOR_H
#define EVALUATORS_PH_EVALUATOR_H

#include "core/card.h"
#include <vector>
#include <cstdint>

namespace poker_engine {

/**
 * @brief True PHEvaluator implementation using pre-computed lookup tables
 *
 * This evaluator uses combinatorial indexing and lookup tables for fast
 * 7-card hand evaluation. Tables are generated once at first use.
 *
 * Memory footprint: ~228 KB (fits in L2 cache)
 * - Flush table: 8,192 entries (32 KB)
 * - Rank table: 50,388 entries (196 KB)
 * - Hash table: 91 entries (364 bytes)
 */
class PHEvaluator {
 public:
  PHEvaluator();

  // Evaluate best 5-card hand from 7 cards (native 7-card evaluator)
  int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                        const std::vector<Card>& board_cards) const;

 private:
  // Initialize tables on first use (one-time cost ~80ms)
  static void init_tables();

  // Compute combinatorial index for 7 ranks using hash table
  uint32_t compute_rank_index(const std::vector<uint8_t>& ranks) const;

  // Static lookup tables (shared across all instances)
  static bool tables_initialized_;
  static int32_t flush_table_[8192];      // Maps 13-bit rank masks to flush scores
  static int32_t rank_table_[50388];      // Maps rank multisets to scores
  static uint32_t hash_table_[7][13];     // Binomial coefficients for indexing
};

}  // namespace poker_engine

#endif  // EVALUATORS_PH_EVALUATOR_H
