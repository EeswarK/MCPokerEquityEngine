#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <vector>
#include "../core/card.h"
#include "../core/deck.h"
#include "../evaluators/naive_evaluator.h"
#include "../evaluators/cactus_kev_evaluator.h"
#include "../evaluators/ph_evaluator.h"
#include "../evaluators/two_plus_two_evaluator.h"
#include "../evaluators/omp_eval.h"

namespace poker_engine {

class EvaluatorConsistencyTest : public ::testing::Test {
 protected:
  NaiveEvaluator naive_;
  CactusKevEvaluator cactus_;
  PHEvaluator ph_;
  TwoPlusTwoEvaluator tpt_;
  OMPEval omp_;

  void CompareEvaluators(const std::vector<Card>& hole, const std::vector<Card>& board) {
    int32_t expected = naive_.evaluate_hand(hole, board);
    
    EXPECT_EQ(cactus_.evaluate_hand(hole, board), expected) << "Cactus Kev mismatch";
    EXPECT_EQ(ph_.evaluate_hand(hole, board), expected) << "PH Evaluator mismatch";
    EXPECT_EQ(tpt_.evaluate_hand(hole, board), expected) << "Two Plus Two mismatch";
    EXPECT_EQ(omp_.evaluate_hand(hole, board), expected) << "OMP Eval mismatch";
  }
};

TEST_F(EvaluatorConsistencyTest, RandomHandsMatchNaive) {
  Deck deck;
  const int kNumIterations = 1000; // Thorough enough for a unit test

  for (int i = 0; i < kNumIterations; ++i) {
    deck.reset();
    std::vector<Card> hole = deck.sample(2);
    std::vector<Card> board = deck.sample(5);
    CompareEvaluators(hole, board);
  }
}

TEST_F(EvaluatorConsistencyTest, BenchmarkEvaluators) {
  Deck deck;
  const int kNumBenchmarks = 100000;
  std::vector<std::vector<Card>> holes;
  std::vector<std::vector<Card>> boards;

  for (int i = 0; i < kNumBenchmarks; ++i) {
    deck.reset();
    holes.push_back(deck.sample(2));
    boards.push_back(deck.sample(5));
  }

  auto benchmark = [&](const std::string& name, auto& evaluator) {
    auto start = std::chrono::high_resolution_clock::now();
    volatile int32_t sink = 0;
    for (int i = 0; i < kNumBenchmarks; ++i) {
      sink = evaluator.evaluate_hand(holes[i], boards[i]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "[ BENCHMARK ] " << name << ": " 
              << (kNumBenchmarks / diff.count()) / 1e6 << "M evals/sec" << std::endl;
  };

  benchmark("Naive         ", naive_);
  benchmark("Cactus Kev    ", cactus_);
  benchmark("PH Evaluator  ", ph_);
  benchmark("Two Plus Two  ", tpt_);
  benchmark("OMP Eval      ", omp_);
}

} // namespace poker_engine
