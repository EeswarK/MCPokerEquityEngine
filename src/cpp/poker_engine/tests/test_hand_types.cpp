#include <gtest/gtest.h>
#include "../evaluators/hand_types.h"

TEST(HandTypesTest, EvaluatorTypeEnumExists) {
    // This should fail to compile if EvaluatorType is not defined
    EvaluatorType type = EvaluatorType::NAIVE;
    EXPECT_EQ(type, EvaluatorType::NAIVE);
    
    type = EvaluatorType::CACTUS_KEV;
    EXPECT_EQ(type, EvaluatorType::CACTUS_KEV);

    type = EvaluatorType::PH_EVALUATOR;
    EXPECT_EQ(type, EvaluatorType::PH_EVALUATOR);
    
    type = EvaluatorType::TWO_PLUS_TWO;
    EXPECT_EQ(type, EvaluatorType::TWO_PLUS_TWO);

    type = EvaluatorType::OMP_EVAL;
    EXPECT_EQ(type, EvaluatorType::OMP_EVAL);
}

TEST(HandTypesTest, OptimizationFlagsExist) {
    // This should fail to compile if OptimizationFlags is not defined
    uint8_t flags = OptimizationFlags::NONE;
    EXPECT_EQ(flags, 0);

    flags = OptimizationFlags::MULTITHREADING;
    EXPECT_TRUE(flags & OptimizationFlags::MULTITHREADING);

    flags = OptimizationFlags::SIMD;
    EXPECT_TRUE(flags & OptimizationFlags::SIMD);

    flags = OptimizationFlags::PERFECT_HASH;
    EXPECT_TRUE(flags & OptimizationFlags::PERFECT_HASH);
    
    flags = OptimizationFlags::PREFETCHING;
    EXPECT_TRUE(flags & OptimizationFlags::PREFETCHING);
}
