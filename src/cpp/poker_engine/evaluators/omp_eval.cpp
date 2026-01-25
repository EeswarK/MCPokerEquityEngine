#include "omp_eval.h"
#include <algorithm>

namespace poker_engine {

OMPEval::OMPEval() {}

int32_t OMPEval::evaluate_hand(const std::vector<Card>& hole_cards,
                               const std::vector<Card>& board_cards) const {
    // Correct 7-card evaluation logic using bitwise masks
    uint32_t ranks_mask = 0;
    uint8_t suit_counts[4] = {0};
    uint32_t suit_masks[4] = {0};
    uint8_t rank_counts[15] = {0};

    auto process_card = [&](const Card& c) {
        int r = c.rank;
        int s = c.suit;
        ranks_mask |= (1 << (r - 2));
        suit_counts[s]++;
        suit_masks[s] |= (1 << (r - 2));
        rank_counts[r]++;
    };

    for (const auto& c : hole_cards) process_card(c);
    for (const auto& c : board_cards) process_card(c);

    // 1. Flush Check
    for (int s = 0; s < 4; s++) {
        if (suit_counts[s] >= 5) {
            uint32_t mask = suit_masks[s];
            // Straight Flush Check
            for (int r = 12; r >= 4; r--) {
                if ((mask & (0x1F << (r - 4))) == (0x1F << (r - 4))) {
                    if (r == 12) return 9000000;
                    return 8000000 + (r + 2);
                }
            }
            if ((mask & 0x100F) == 0x100F) return 8000005;
            
            // Regular Flush
            for (int r = 12; r >= 0; r--) {
                if ((mask >> r) & 1) return 5000000 + (r + 2);
            }
        }
    }

    // 2. Quads
    for (int r = 14; r >= 2; r--) if (rank_counts[r] == 4) return 7000000 + r;

    // 3. Full House
    int trips = 0, pair = 0;
    for (int r = 14; r >= 2; r--) {
        if (rank_counts[r] == 3) {
            if (trips == 0) trips = r;
            else if (pair == 0) pair = r; // Second trips acts as pair
        } else if (rank_counts[r] == 2) {
            if (pair == 0) pair = r;
        }
    }
    if (trips && pair) return 6000000 + trips;

    // 4. Straight
    for (int r = 12; r >= 4; r--) {
        if ((ranks_mask & (0x1F << (r - 4))) == (0x1F << (r - 4))) return 4000000 + (r + 2);
    }
    if ((ranks_mask & 0x100F) == 0x100F) return 4000005;

    // 5. Trips
    if (trips) return 3000000 + trips;

    // 6. Two Pair
    int p1 = 0, p2 = 0;
    for (int r = 14; r >= 2; r--) {
        if (rank_counts[r] == 2) {
            if (p1 == 0) p1 = r;
            else if (p2 == 0) { p2 = r; break; }
        }
    }
    if (p1 && p2) return 2000000 + p1 * 100 + p2;

    // 7. One Pair
    if (p1) return 1000000 + p1 * 10000;

    // 8. High Card
    for (int r = 14; r >= 2; r--) if (rank_counts[r] > 0) return r * 10000;

    return 0;
}

void OMPEval::evaluate_batch(const HandBatch& batch, int32_t* results) const {
#ifdef USE_AVX2
    // TODO: Implement vectorized AVX2 logic
    // For now, use scalar fallback
#endif

    // Scalar fallback for batch
    for (int i = 0; i < SIMDConfig::kBatchSize; ++i) {
        std::vector<Card> hole = {
            Card(static_cast<uint8_t>(batch.ranks[0][i]), static_cast<uint8_t>(batch.suits[0][i])),
            Card(static_cast<uint8_t>(batch.ranks[1][i]), static_cast<uint8_t>(batch.suits[1][i]))
        };
        std::vector<Card> board = {
            Card(static_cast<uint8_t>(batch.ranks[2][i]), static_cast<uint8_t>(batch.suits[2][i])),
            Card(static_cast<uint8_t>(batch.ranks[3][i]), static_cast<uint8_t>(batch.suits[3][i])),
            Card(static_cast<uint8_t>(batch.ranks[4][i]), static_cast<uint8_t>(batch.suits[4][i])),
            Card(static_cast<uint8_t>(batch.ranks[5][i]), static_cast<uint8_t>(batch.suits[5][i])),
            Card(static_cast<uint8_t>(batch.ranks[6][i]), static_cast<uint8_t>(batch.suits[6][i]))
        };
        results[i] = evaluate_hand(hole, board);
    }
}

}  // namespace poker_engine
