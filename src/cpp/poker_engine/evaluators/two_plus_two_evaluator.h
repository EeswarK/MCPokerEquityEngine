#ifndef EVALUATORS_TWO_PLUS_TWO_EVALUATOR_H
#define EVALUATORS_TWO_PLUS_TWO_EVALUATOR_H

#include "core/card.h"
#include <vector>

class TwoPlusTwoEvaluator {
public:
    TwoPlusTwoEvaluator();

    // Evaluate best 5-card hand from 7 cards (lookup table state machine)
    int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                          const std::vector<Card>& board_cards) const;

private:
    void load_table(const char* filename);
    void prefetch(const std::vector<Card>& cards) const;

    // The massive lookup table (would normally be loaded from disk or mmap)
    // For now, we stub it or use a simplified mock.
    // std::vector<int> lookup_table_;
    bool table_loaded_;
};

#endif // EVALUATORS_TWO_PLUS_TWO_EVALUATOR_H
