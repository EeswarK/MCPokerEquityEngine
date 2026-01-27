#ifndef EVALUATORS_CACTUS_KEV_EVALUATOR_H
#define EVALUATORS_CACTUS_KEV_EVALUATOR_H

#include "core/card.h"
#include <vector>
#include <array>

namespace poker_engine {

class CactusKevEvaluator {
 public:
  CactusKevEvaluator();

  // NEW: Direct 7-card evaluation (zero-copy API)
  inline int32_t evaluate_7(const Card cards[7]) const noexcept {
    std::vector<Card> all_cards(cards, cards + 7);
    return evaluate_hand(all_cards, {});
  }

  // Evaluate best 5-card hand from hole cards + board
  int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                        const std::vector<Card>& board_cards) const;

 private:
  int32_t evaluate_5_cards(const std::vector<Card>& cards) const;
  int find_fast(uint32_t u) const;

  void init_tables();
  void populate_flushes();
  void populate_unique5();

  std::array<int, 8192> flush_lookup_;
  std::array<int, 4888> unique5_lookup_;
  std::array<uint32_t, 4888> perfect_hash_keys_;
};

}  // namespace poker_engine

#endif  // EVALUATORS_CACTUS_KEV_EVALUATOR_H
