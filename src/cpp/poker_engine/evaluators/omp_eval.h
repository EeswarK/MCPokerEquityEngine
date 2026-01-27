#ifndef EVALUATORS_OMP_EVAL_H
#define EVALUATORS_OMP_EVAL_H

#include "core/card.h"
#include "../engine/simd_helper.h"
#include <vector>

namespace poker_engine {

class OMPEval {
 public:
  OMPEval();

  // NEW: Direct 7-card evaluation (zero-copy API)
  inline int32_t evaluate_7(const Card cards[7]) const noexcept {
    std::vector<Card> all_cards(cards, cards + 7);
    return evaluate_hand(all_cards, {});
  }

  // Evaluate best 5-card hand from 7 cards (pure bit math)
  int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                        const std::vector<Card>& board_cards) const;

  // Batch evaluation using SIMD Framework
  void evaluate_batch(const HandBatch& batch, int32_t* results) const;
};

}  // namespace poker_engine

#endif // EVALUATORS_OMP_EVAL_H
