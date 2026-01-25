#include <gtest/gtest.h>
#include "../evaluators/ph_evaluator.h"
#include "../evaluators/hand_types.h"

using namespace poker_engine;

class PHEvaluatorTest : public ::testing::Test {
protected:
    PHEvaluator evaluator;
};

TEST_F(PHEvaluatorTest, EvaluatesHand) {
    std::vector<Card> hole = {Card(14, 0), Card(14, 1)}; // As, Ah
    std::vector<Card> board = {Card(2, 2), Card(5, 3), Card(9, 0), Card(3, 1), Card(4, 2)}; // 2c, 5s, 9s, 3h, 4c
    
    // Board: 2, 3, 4, 5, 9.  Hole: A, A.
    // A, 2, 3, 4, 5 form a Straight (Wheel).
    
    int32_t score = evaluator.evaluate_hand(hole, board);
    
    EXPECT_GE(score, STRAIGHT_MIN);
    EXPECT_LT(score, FLUSH_MIN);
    EXPECT_EQ(get_hand_type(score), HandType::STRAIGHT);
}

TEST_F(PHEvaluatorTest, EvaluatesStraightFlush) {
    std::vector<Card> hole = {Card(6, 0), Card(7, 0)}; // 6s, 7s
    std::vector<Card> board = {Card(8, 0), Card(9, 0), Card(10, 0), Card(2, 1), Card(3, 2)}; // 8s, 9s, Ts...
    
    int32_t score = evaluator.evaluate_hand(hole, board);
    EXPECT_GE(score, STRAIGHT_FLUSH_MIN);
    EXPECT_EQ(get_hand_type(score), HandType::STRAIGHT_FLUSH);
}
