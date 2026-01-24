#ifndef EVALUATORS_PH_EVALUATOR_H
#define EVALUATORS_PH_EVALUATOR_H

#include "core/card.h"
#include <vector>

class PHEvaluator {
public:
    PHEvaluator();

    // Evaluate best 5-card hand from 7 cards (native 7-card evaluator)
    int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                          const std::vector<Card>& board_cards) const;

private:
    void prefetch(const std::vector<Card>& cards) const;
};

#endif // EVALUATORS_PH_EVALUATOR_H
