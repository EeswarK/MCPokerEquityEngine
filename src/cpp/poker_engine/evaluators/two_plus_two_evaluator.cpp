#include "two_plus_two_evaluator.h"
#include "hand_types.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>

namespace poker_engine {

TwoPlusTwoEvaluator::TwoPlusTwoEvaluator() : table_loaded_(false) {
    load_table("HandRanks.dat");
}

void TwoPlusTwoEvaluator::load_table(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open " << filename << ". Using fallback evaluator." << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    lookup_table_.resize(size / sizeof(int32_t));
    if (file.read(reinterpret_cast<char*>(lookup_table_.data()), size)) {
        table_loaded_ = true;
        std::cout << "Successfully loaded Two Plus Two lookup table (" << size / (1024*1024) << " MB)." << std::endl;
    }
}

int32_t TwoPlusTwoEvaluator::evaluate_hand(const std::vector<Card>& hole_cards,
                                           const std::vector<Card>& board_cards) const {
    if (!table_loaded_) {
        return evaluate_fallback(hole_cards, board_cards);
    }

    // TwoPlusTwo Traversal Logic
    int p = 53; // Start index

    auto process_card = [&](const Card& c) {
        int card_val = (c.rank - 2) * 4 + c.suit + 1;
        // No bounds check for speed, assuming table is correct.
        p = lookup_table_[p + card_val];
    };

    for (const auto& c : hole_cards) process_card(c);
    for (const auto& c : board_cards) process_card(c);

    // The result is in p after 7 transitions
    int res = p;

    // Convert 2+2 internal HandRank to our engine score

    
    // Convert 2+2 internal HandRank to our engine score
    // hhhhrrrrrrrrrrrr format (from generate_table.cpp comment)
    int hand_type = res >> 12;
    int rank_within_type = res & 0x0FFF;
    
    // Map 2+2 types (1-9) to our thresholds
    // 2+2: 1=HC, 2=Pair, 3=2Pair, 4=3Kind, 5=Straight, 6=Flush, 7=FH, 8=4Kind, 9=SF
    // Ours: HC=0, Pair=1M, 2P=2M, 3K=3M, Str=4M, Fl=5M, FH=6M, 4K=7M, SF=8M, RF=9M
    
    switch (hand_type) {
        case 9: // Straight Flush
            if (rank_within_type == 10) return 9000000; // Royal Flush
            return 8000000 + rank_within_type;
        case 8: return 7000000 + rank_within_type; // Four of a Kind
        case 7: return 6000000 + rank_within_type; // Full House
        case 6: return 5000000 + rank_within_type; // Flush
        case 5: return 4000000 + rank_within_type; // Straight
        case 4: return 3000000 + rank_within_type; // Three of a Kind
        case 3: return 2000000 + rank_within_type; // Two Pair
        case 2: return 1000000 + rank_within_type; // One Pair
        case 1: return rank_within_type;           // High Card
        default: return 0;
    }
}

int32_t TwoPlusTwoEvaluator::evaluate_fallback(const std::vector<Card>& hole, const std::vector<Card>& board) const {
    // Correct fallback evaluation logic
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

    for (const auto& c : hole) process_card(c);
    for (const auto& c : board) process_card(c);

    // 1. Flush Check
    for (int s = 0; s < 4; s++) {
        if (suit_counts[s] >= 5) {
            uint32_t mask = suit_masks[s];
            for (int r = 12; r >= 4; r--) {
                if ((mask & (0x1F << (r - 4))) == (0x1F << (r - 4))) {
                    if (r == 12) return 9000000; // Royal Flush
                    return 8000000 + (r + 2);    // Straight Flush
                }
            }
            if ((mask & 0x100F) == 0x100F) return 8000005; // Steel Wheel
            for (int r = 12; r >= 0; r--) if ((mask >> r) & 1) return 5000000 + (r + 2);
        }
    }

    // 2. Quads
    for (int r = 14; r >= 2; r--) if (rank_counts[r] == 4) return 7000000 + r;

    // 3. Full House
    int trips = 0, pair = 0;
    for (int r = 14; r >= 2; r--) {
        if (rank_counts[r] == 3) {
            if (trips == 0) trips = r;
            else if (pair == 0) pair = r;
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

void TwoPlusTwoEvaluator::prefetch(const std::vector<Card>& cards) const {
    // __builtin_prefetch
}
}  // namespace poker_engine
