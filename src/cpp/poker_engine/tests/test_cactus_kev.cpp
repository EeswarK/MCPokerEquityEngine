#include <gtest/gtest.h>
#include "../evaluators/cactus_kev_evaluator.h"
#include "../evaluators/hand_types.h"

using namespace poker_engine;

class CactusKevTest : public ::testing::Test {
protected:
    CactusKevEvaluator evaluator;
};

TEST_F(CactusKevTest, EvaluatesRoyalFlush) {
    std::vector<Card> hole = {Card(14, 0), Card(13, 0)}; // As, Ks
    std::vector<Card> board = {
        Card(12, 0), Card(11, 0), Card(10, 0), // Qs, Js, Ts
        Card(2, 1), Card(3, 2)                 // 2d, 3c (irrelevant)
    };

    int32_t score = evaluator.evaluate_hand(hole, board);
    
    // Should be at least ROYAL_FLUSH_MIN
    EXPECT_GE(score, ROYAL_FLUSH_MIN);
    EXPECT_EQ(get_hand_type(score), HandType::ROYAL_FLUSH);
}

TEST_F(CactusKevTest, EvaluatesHighCard) {
    std::vector<Card> hole = {Card(2, 1), Card(3, 2)}; // 2d, 3c
    std::vector<Card> board = {
        Card(5, 3), Card(7, 0), Card(9, 1), // 5s, 7h, 9d
        Card(11, 2), Card(13, 3)            // Jc, Ks
    };
    // Best hand: K, J, 9, 7, 5 (High Card)

    int32_t score = evaluator.evaluate_hand(hole, board);
    
    EXPECT_LT(score, ONE_PAIR_MIN);
    EXPECT_EQ(get_hand_type(score), HandType::HIGH_CARD);
}
