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
        return;  // Already initialized by another thread
    }

    // Generate all lookup tables
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

int32_t PHEvaluator::evaluate_hand(const std::vector<Card>& hole_cards,
                                   const std::vector<Card>& board_cards) const {
    // Step 1: Build suit masks and counts
    uint8_t suit_counts[4] = {0};
    uint32_t suit_masks[4] = {0};  // 13-bit mask per suit

    auto process_card = [&](const Card& c) {
        suit_counts[c.suit]++;
        suit_masks[c.suit] |= (1 << (c.rank - 2));
    };

    for (const auto& c : hole_cards) process_card(c);
    for (const auto& c : board_cards) process_card(c);

    // Step 2: Check for flush (fast path)
    for (int s = 0; s < 4; s++) {
        if (suit_counts[s] >= 5) {
            uint32_t mask = suit_masks[s];

            // If exactly 5 cards, use mask directly
            if (suit_counts[s] == 5) {
                return flush_table_[mask];
            }

            // More than 5 cards: check for straight flush first
            // Check all possible 5-bit windows for straight flush
            bool has_straight_flush = false;
            for (int r = 12; r >= 4; r--) {
                uint32_t straight_mask = 0x1F << (r - 4);
                if ((mask & straight_mask) == straight_mask) {
                    has_straight_flush = true;
                    mask = straight_mask;  // Use this straight flush
                    break;
                }
            }
            // Check for wheel straight flush (A-2-3-4-5)
            if (!has_straight_flush && (mask & 0x100F) == 0x100F) {
                has_straight_flush = true;
                mask = 0x100F;
            }

            // If no straight flush, take top 5 ranks
            if (!has_straight_flush) {
                std::vector<uint8_t> flush_ranks;
                for (int r = 12; r >= 0; r--) {
                    if ((mask >> r) & 1) {
                        flush_ranks.push_back(r);
                        if (flush_ranks.size() == 5) break;
                    }
                }

                // Rebuild mask with only top 5
                mask = 0;
                for (uint8_t r : flush_ranks) {
                    mask |= (1 << r);
                }
            }

            return flush_table_[mask];
        }
    }

    // Step 3: Non-flush path - extract 7 ranks in descending order
    std::vector<uint8_t> ranks;
    ranks.reserve(7);

    for (int r = 14; r >= 2; r--) {
        for (const auto& c : hole_cards) {
            if (c.rank == r) {
                ranks.push_back(r);
                if (ranks.size() == 7) goto done_collecting;
            }
        }
        for (const auto& c : board_cards) {
            if (c.rank == r) {
                ranks.push_back(r);
                if (ranks.size() == 7) goto done_collecting;
            }
        }
    }
done_collecting:

    // Step 4: Compute combinatorial index
    uint32_t index = compute_rank_index(ranks);

    // Step 5: Lookup and return
    return rank_table_[index];
}

uint32_t PHEvaluator::compute_rank_index(const std::vector<uint8_t>& ranks) const {
    // CRITICAL: ranks come in DESCENDING order (A,K,Q,... or [14,13,12,...])
    // but the hash table expects multisets in NON-DECREASING (ascending) order
    // We must REVERSE the ranks and convert to 0-12 indexing

    // Convert descending ranks to ascending multiset with 0-12 indexing
    uint32_t index = 0;
    for (int i = 0; i < 7; i++) {
        // Reverse: position i in ascending = position 6-i in descending
        // Convert to 0-12: rank - 2
        index += hash_table_[i][ranks[6 - i] - 2];
    }
    return index;
}

}  // namespace poker_engine
