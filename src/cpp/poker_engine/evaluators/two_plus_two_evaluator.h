#ifndef EVALUATORS_TWO_PLUS_TWO_EVALUATOR_H
#define EVALUATORS_TWO_PLUS_TWO_EVALUATOR_H

#include "core/card.h"
#include "hand_types.h"
#include <vector>

namespace poker_engine {

class TwoPlusTwoEvaluator {
 public:
  TwoPlusTwoEvaluator();

  // NEW: Direct 7-card evaluation (zero-copy API)
  inline int32_t evaluate_7(const Card cards[7]) const noexcept {
    std::vector<Card> all_cards(cards, cards + 7);
    return evaluate_hand(all_cards, {});
  }

  // Evaluate best 5-card hand from 7 cards (lookup table state machine)
  int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                        const std::vector<Card>& board_cards) const;

 private:
  void load_table(const char* filename);
  void prefetch(const std::vector<Card>& cards) const;

  // Fallback logic for when table is missing
  int32_t evaluate_fallback(const std::vector<Card>& hole,
                            const std::vector<Card>& board) const;

  std::vector<int32_t> lookup_table_;
  bool table_loaded_;
};

}  // namespace poker_engine

#endif  // EVALUATORS_TWO_PLUS_TWO_EVALUATOR_H
