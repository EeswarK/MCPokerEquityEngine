#ifndef EVALUATORS_OMP_EVAL_H
#define EVALUATORS_OMP_EVAL_H

#include "core/card.h"
#include <vector>

class OMPEval {
public:
    OMPEval();

    // Evaluate best 5-card hand from 7 cards (pure bit math)
    int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                          const std::vector<Card>& board_cards) const;

private:
    // Helper for SIMD-ready evaluation
    void evaluate_batch(const std::vector<std::vector<Card>>& hands, std::vector<int32_t>& scores) const;
};

#endif // EVALUATORS_OMP_EVAL_H
