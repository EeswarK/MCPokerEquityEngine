#include <gtest/gtest.h>
#include "../evaluators/ph_evaluator.h"
#include "../evaluators/hand_types.h"

class PHEvaluatorTest : public ::testing::Test {
protected:
    PHEvaluator evaluator;
};

TEST_F(PHEvaluatorTest, EvaluatesHand) {
    std::vector<Card> hole = {Card(14, 0), Card(14, 1)}; // AA
    std::vector<Card> board = {Card(2, 2), Card(5, 3), Card(9, 0), Card(3, 1), Card(4, 2)};
    
    int32_t score = evaluator.evaluate_hand(hole, board);
    EXPECT_GT(score, 0);
}
