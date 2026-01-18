#ifndef ENGINE_EQUITY_RESULT_H
#define ENGINE_EQUITY_RESULT_H

#include <string>
#include <vector>
#include <cstdint>

struct EquityResult {
    std::string hand_name;
    double equity;  // 0.0 to 1.0
    uint32_t wins;
    uint32_t ties;
    uint32_t losses;
    uint32_t total_simulations;

    // 10x10 matrices for win/loss method tracking
    // [our_type][opp_type] for wins, [opp_type][our_type] for losses
    uint32_t win_method_matrix[10][10];
    uint32_t loss_method_matrix[10][10];

    EquityResult()
        : equity(0.0), wins(0), ties(0), losses(0), total_simulations(0) {
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 10; ++j) {
                win_method_matrix[i][j] = 0;
                loss_method_matrix[i][j] = 0;
            }
        }
    }
};

#endif // ENGINE_EQUITY_RESULT_H
