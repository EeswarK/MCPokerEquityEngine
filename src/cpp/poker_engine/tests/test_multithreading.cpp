#include <gtest/gtest.h>
#include "../engine/equity_engine.h"

TEST(EquityEngineTest, MultithreadingWorks) {
    EquityEngine engine("test_mode");
    
    JobRequest request;
    request.range_spec["AA"] = {Card(14, 0), Card(14, 1)};
    request.board = {};
    request.num_opponents = 1;
    request.num_simulations = 10000;
    request.algorithm = "naive";
    request.num_workers = 4; // 4 threads

    auto results = engine.calculate_range_equity(request);
    
    EXPECT_GT(results.size(), 0);
    EXPECT_TRUE(results.count("AA"));
    EXPECT_GT(results["AA"].total_simulations, 0);
}
