#include "evaluator_conformance.h"
#include "../evaluators/naive_evaluator.h"
#include "../evaluators/cactus_kev_evaluator.h"
#include "../evaluators/ph_evaluator.h"
#include "../evaluators/two_plus_two_evaluator.h"
#include "../evaluators/omp_eval.h"

using namespace poker_engine;

// Define types to test
using EvaluatorTypes = ::testing::Types<
    NaiveEvaluator,
    CactusKevEvaluator,
    PHEvaluator,
    TwoPlusTwoEvaluator,
    OMPEval
>;

// Instantiate the test suite
INSTANTIATE_TYPED_TEST_SUITE_P(AllEvaluators, EvaluatorConformanceTest, EvaluatorTypes);
