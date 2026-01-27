#include "ph_evaluator.h"
#include "ph_evaluator_tables.h"
#include "hand_types.h"
#include <algorithm>
#include <mutex>

namespace poker_engine {

// Static table definitions
bool PHEvaluator::tables_initialized_ = false;
int32_t PHEvaluator::flush_table_[8192];
int32_t PHEvaluator::rank_table_[50388];
uint32_t PHEvaluator::hash_table_[7][13];

// Thread-safe table initialization
static std::mutex init_mutex;

void PHEvaluator::init_tables() {
    std::lock_guard<std::mutex> lock(init_mutex);

    if (tables_initialized_) {
        return;
    }

    populate_hash_table(hash_table_);
    populate_flush_table(flush_table_);
    populate_rank_table(rank_table_, hash_table_);

    tables_initialized_ = true;
}

PHEvaluator::PHEvaluator() {
    if (!tables_initialized_) {
        init_tables();
    }
}

// LEGACY: Backward compatibility wrapper (inline implementations moved to header)
int32_t PHEvaluator::evaluate_hand(const std::vector<Card>& hole_cards,
                                   const std::vector<Card>& board_cards) const {
    // Build fixed-size array and call optimized evaluate_7()
    Card cards[7];
    int idx = 0;
    for (const auto& c : hole_cards) cards[idx++] = c;
    for (const auto& c : board_cards) cards[idx++] = c;
    return evaluate_7(cards);
}

}  // namespace poker_engine
