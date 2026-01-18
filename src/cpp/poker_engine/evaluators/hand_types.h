#ifndef EVALUATORS_HAND_TYPES_H
#define EVALUATORS_HAND_TYPES_H

#include <cstdint>

// Hand type values (matches Python get_hand_type)
enum HandType : uint8_t {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_KIND = 7,
    STRAIGHT_FLUSH = 8,
    ROYAL_FLUSH = 9
};

// Hand value thresholds (matches Python evaluator ranges)
constexpr int32_t ROYAL_FLUSH_MIN = 9000000;
constexpr int32_t STRAIGHT_FLUSH_MIN = 8000000;
constexpr int32_t FOUR_KIND_MIN = 7000000;
constexpr int32_t FULL_HOUSE_MIN = 6000000;
constexpr int32_t FLUSH_MIN = 5000000;
constexpr int32_t STRAIGHT_MIN = 4000000;
constexpr int32_t THREE_KIND_MIN = 3000000;
constexpr int32_t TWO_PAIR_MIN = 2000000;
constexpr int32_t ONE_PAIR_MIN = 1000000;

// Convert hand value to hand type (0-9)
inline HandType get_hand_type(int32_t hand_value) {
    // Matches Python: src/python/engine/strategies/naive/evaluator.py:164-199
    if (hand_value >= ROYAL_FLUSH_MIN) return ROYAL_FLUSH;
    if (hand_value >= STRAIGHT_FLUSH_MIN) return STRAIGHT_FLUSH;
    if (hand_value >= FOUR_KIND_MIN) return FOUR_OF_KIND;
    if (hand_value >= FULL_HOUSE_MIN) return FULL_HOUSE;
    if (hand_value >= FLUSH_MIN) return FLUSH;
    if (hand_value >= STRAIGHT_MIN) return STRAIGHT;
    if (hand_value >= THREE_KIND_MIN) return THREE_OF_KIND;
    if (hand_value >= TWO_PAIR_MIN) return TWO_PAIR;
    if (hand_value >= ONE_PAIR_MIN) return ONE_PAIR;
    return HIGH_CARD;
}

#endif // EVALUATORS_HAND_TYPES_H
