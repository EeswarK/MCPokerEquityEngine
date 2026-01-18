#ifndef EVALUATORS_NAIVE_EVALUATOR_H
#define EVALUATORS_NAIVE_EVALUATOR_H

#include "core/card.h"
#include "hand_types.h"
#include <vector>
#include <string>
#include <random>

// Simulation result (matches Python tuple return)
struct SimulationResult {
    int outcome;           // 1=win, 0=tie, -1=loss
    HandType our_type;     // 0-9
    HandType opp_type;     // 0-9
    std::string opp_classification;  // "AA", "AKs", "72o", etc.
};

class NaiveEvaluator {
public:
    NaiveEvaluator() = default;

    // Evaluate best 5-card hand from hole cards + board
    // Matches: src/python/engine/strategies/naive/evaluator.py:6-27
    int32_t evaluate_hand(const std::vector<Card>& hole_cards,
                          const std::vector<Card>& board_cards) const;

    // Simulate one hand against N opponents
    // Matches: src/python/engine/strategies/naive/evaluator.py:30-83
    SimulationResult simulate_hand(const std::vector<Card>& hole_cards,
                                   const std::vector<Card>& board,
                                   int num_opponents) const;

private:
    // Evaluate specific 5-card combination
    // Matches: src/python/engine/strategies/naive/evaluator.py:94-142
    int32_t evaluate_five_cards(const std::vector<Card>& cards) const;

    // Check if ranks form a straight
    // Matches: src/python/engine/strategies/naive/evaluator.py:145-161
    bool is_straight(const std::vector<uint8_t>& ranks) const;

    // Classify hole cards ("AA", "AKs", "72o")
    // Matches: src/python/engine/strategies/naive/evaluator.py:202-245
    std::string classify_hole_cards(const std::vector<Card>& hole_cards) const;

    // Thread-local RNG for deck shuffling
    mutable std::mt19937 rng_{std::random_device{}()};
};

#endif // EVALUATORS_NAIVE_EVALUATOR_H
