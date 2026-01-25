#include <gtest/gtest.h>
#include "../evaluators/omp_eval.h"

using namespace poker_engine;

class OMPEvalTest : public ::testing::Test {
protected:
    OMPEval evaluator;
};

TEST_F(OMPEvalTest, EvaluatesHand) {
    std::vector<Card> hole = {Card(14, 0), Card(14, 1)};
    std::vector<Card> board = {Card(2, 2), Card(5, 3), Card(9, 0), Card(3, 1), Card(4, 2)};
    
    int32_t score = evaluator.evaluate_hand(hole, board);
    EXPECT_GT(score, 0);
}
