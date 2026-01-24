#include <gtest/gtest.h>
#include "../api/json_utils.h"

TEST(JsonUtilsTest, ParseCreateJobRequest_AllFields) {
    std::string json_str = R"({
        "range_spec": {
            "AA": [{"rank": 14, "suit": 0}, {"rank": 14, "suit": 1}]
        },
        "board": [],
        "num_opponents": 2,
        "num_simulations": 50000,
        "mode": "cpp_optimized",
        "algorithm": "cactus_kev",
        "optimizations": ["simd", "multithreading"],
        "num_workers": 4
    })";

    std::unordered_map<std::string, std::vector<Card>> range_spec;
    std::vector<Card> board;
    int num_opponents, num_simulations, num_workers;
    std::string mode, algorithm;
    std::vector<std::string> optimizations;

    bool result = parse_create_job_request(
        json_str, range_spec, board, num_opponents, num_simulations, 
        mode, algorithm, optimizations, num_workers
    );

    EXPECT_TRUE(result);
    EXPECT_EQ(num_opponents, 2);
    EXPECT_EQ(num_simulations, 50000);
    EXPECT_EQ(mode, "cpp_optimized");
    EXPECT_EQ(algorithm, "cactus_kev");
    EXPECT_EQ(num_workers, 4);
    
    ASSERT_EQ(optimizations.size(), 2);
    EXPECT_EQ(optimizations[0], "simd");
    EXPECT_EQ(optimizations[1], "multithreading");
}

TEST(JsonUtilsTest, ParseCreateJobRequest_Defaults) {
    std::string json_str = R"({
        "range_spec": {
            "KK": [{"rank": 13, "suit": 0}, {"rank": 13, "suit": 1}]
        }
    })";

    std::unordered_map<std::string, std::vector<Card>> range_spec;
    std::vector<Card> board;
    int num_opponents, num_simulations, num_workers;
    std::string mode, algorithm;
    std::vector<std::string> optimizations;

    bool result = parse_create_job_request(
        json_str, range_spec, board, num_opponents, num_simulations, 
        mode, algorithm, optimizations, num_workers
    );

    EXPECT_TRUE(result);
    EXPECT_EQ(num_opponents, 1);
    EXPECT_EQ(num_simulations, 100000);
    EXPECT_EQ(mode, "cpp_naive");
    EXPECT_EQ(algorithm, "naive");
    EXPECT_EQ(num_workers, 0);
    EXPECT_TRUE(optimizations.empty());
}
