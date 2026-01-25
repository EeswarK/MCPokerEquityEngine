#ifndef TESTS_EVALUATOR_CONFORMANCE_H
#define TESTS_EVALUATOR_CONFORMANCE_H

#include <gtest/gtest.h>
#include <vector>
#include "../core/card.h"
#include "../evaluators/hand_types.h"

namespace poker_engine {

// Template test fixture for conformance testing
template <typename EvaluatorT>
class EvaluatorConformanceTest : public ::testing::Test {
protected:
    EvaluatorT evaluator;

    void VerifyHand(const std::vector<Card>& hole, const std::vector<Card>& board, 
                    HandType expected_type, int32_t min_score, int32_t max_score) {
        int32_t score = evaluator.evaluate_hand(hole, board);
        EXPECT_GE(score, min_score) << "Score too low for expected hand type";
        EXPECT_LT(score, max_score) << "Score too high for expected hand type";
        EXPECT_EQ(get_hand_type(score), expected_type);
    }
};

TYPED_TEST_SUITE_P(EvaluatorConformanceTest);

TYPED_TEST_P(EvaluatorConformanceTest, DetectsRoyalFlush) {
    using namespace poker_engine;
    // As Ks + Qs Js Ts 2d 3c
    std::vector<Card> hole = {Card(14, 0), Card(13, 0)};
    std::vector<Card> board = {Card(12, 0), Card(11, 0), Card(10, 0), Card(2, 1), Card(3, 2)};
    this->VerifyHand(hole, board, HandType::ROYAL_FLUSH, ROYAL_FLUSH_MIN, 10000000);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsStraightFlush) {
    using namespace poker_engine;
    // 9h 8h + 7h 6h 5h 2d 3c (9-high straight flush)
    std::vector<Card> hole = {Card(9, 2), Card(8, 2)};
    std::vector<Card> board = {Card(7, 2), Card(6, 2), Card(5, 2), Card(2, 1), Card(3, 3)};
    this->VerifyHand(hole, board, HandType::STRAIGHT_FLUSH, STRAIGHT_FLUSH_MIN, ROYAL_FLUSH_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsFourOfAKind) {
    using namespace poker_engine;
    // Ah Ac + As Ad 2h 3c 4d
    std::vector<Card> hole = {Card(14, 2), Card(14, 3)};
    std::vector<Card> board = {Card(14, 0), Card(14, 1), Card(2, 2), Card(3, 3), Card(4, 1)};
    this->VerifyHand(hole, board, HandType::FOUR_OF_KIND, FOUR_KIND_MIN, STRAIGHT_FLUSH_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsFullHouse) {
    using namespace poker_engine;
    // Ah Ac + As 2d 2h 3c 4d (Aces full of Deuces)
    std::vector<Card> hole = {Card(14, 2), Card(14, 3)};
    std::vector<Card> board = {Card(14, 0), Card(2, 1), Card(2, 2), Card(3, 3), Card(4, 1)};
    this->VerifyHand(hole, board, HandType::FULL_HOUSE, FULL_HOUSE_MIN, FOUR_KIND_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsFlush) {
    using namespace poker_engine;
    // Ah 2h + 5h 7h 9h Kd Qc
    std::vector<Card> hole = {Card(14, 2), Card(2, 2)};
    std::vector<Card> board = {Card(5, 2), Card(7, 2), Card(9, 2), Card(13, 1), Card(12, 3)};
    this->VerifyHand(hole, board, HandType::FLUSH, FLUSH_MIN, FULL_HOUSE_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsStraight) {
    using namespace poker_engine;
    // 9s 8c + 7d 6h 5s 2d 2c (9-high straight)
    std::vector<Card> hole = {Card(9, 0), Card(8, 3)};
    std::vector<Card> board = {Card(7, 1), Card(6, 2), Card(5, 0), Card(2, 1), Card(2, 3)};
    this->VerifyHand(hole, board, HandType::STRAIGHT, STRAIGHT_MIN, FLUSH_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsThreeOfAKind) {
    using namespace poker_engine;
    // Ah Ac + As 9d 8h Tc 2d (Trip Aces, no other pairs, no straight)
    std::vector<Card> hole = {Card(14, 2), Card(14, 3)};
    std::vector<Card> board = {Card(14, 0), Card(9, 1), Card(8, 2), Card(10, 3), Card(2, 1)};
    this->VerifyHand(hole, board, HandType::THREE_OF_KIND, THREE_KIND_MIN, STRAIGHT_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsTwoPair) {
    using namespace poker_engine;
    // Ah Ac + 2d 2h 9c Jd 4s (Aces and Deuces, no straight, no trips)
    std::vector<Card> hole = {Card(14, 2), Card(14, 3)};
    std::vector<Card> board = {Card(2, 1), Card(2, 2), Card(9, 3), Card(11, 1), Card(4, 0)};
    this->VerifyHand(hole, board, HandType::TWO_PAIR, TWO_PAIR_MIN, THREE_KIND_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsOnePair) {
    using namespace poker_engine;
    // Ah Ac + 2d 9h Tc Jd Ks (Pair of Aces, no straight)
    std::vector<Card> hole = {Card(14, 2), Card(14, 3)};
    std::vector<Card> board = {Card(2, 1), Card(9, 2), Card(10, 3), Card(11, 1), Card(13, 0)};
    this->VerifyHand(hole, board, HandType::ONE_PAIR, ONE_PAIR_MIN, TWO_PAIR_MIN);
}

TYPED_TEST_P(EvaluatorConformanceTest, DetectsHighCard) {
    using namespace poker_engine;
    // Ah 3c + 5d 7h 9c Jd Ks (Ace High)
    std::vector<Card> hole = {Card(14, 2), Card(3, 3)};
    std::vector<Card> board = {Card(5, 1), Card(7, 2), Card(9, 3), Card(11, 1), Card(13, 0)};
    this->VerifyHand(hole, board, HandType::HIGH_CARD, 0, ONE_PAIR_MIN);
}

REGISTER_TYPED_TEST_SUITE_P(EvaluatorConformanceTest,
                            DetectsRoyalFlush,
                            DetectsStraightFlush,
                            DetectsFourOfAKind,
                            DetectsFullHouse,
                            DetectsFlush,
                            DetectsStraight,
                            DetectsThreeOfAKind,
                            DetectsTwoPair,
                            DetectsOnePair,
                            DetectsHighCard);

} // namespace poker_engine

#endif // TESTS_EVALUATOR_CONFORMANCE_H

