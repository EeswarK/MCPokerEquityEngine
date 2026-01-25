#include "ph_evaluator.h"
#include "hand_types.h"
#include <algorithm>
#include <array>

namespace poker_engine {

PHEvaluator::PHEvaluator() {
    // Initialize rank/suit lookup tables if needed
}

int32_t PHEvaluator::evaluate_hand(const std::vector<Card>& hole_cards,
                                   const std::vector<Card>& board_cards) const {
    // Combine cards
    std::array<Card, 7> hand;
    size_t idx = 0;
    for (const auto& c : hole_cards) hand[idx++] = c;
    for (const auto& c : board_cards) hand[idx++] = c;
    size_t n = idx; // Should be 7

    // 1. Check Flush (optimized)
    uint16_t suit_counts[4] = {0};
    uint16_t suit_masks[4] = {0}; // Holds rank bits for each suit
    
    for (size_t i = 0; i < n; i++) {
        suit_counts[hand[i].suit]++;
        suit_masks[hand[i].suit] |= (1 << (hand[i].rank - 2));
    }
    
    int flush_suit = -1;
    for (int s = 0; s < 4; s++) {
        if (suit_counts[s] >= 5) {
            flush_suit = s;
            break;
        }
    }
    
    if (flush_suit != -1) {
        // Evaluate Flush / Straight Flush
        uint16_t mask = suit_masks[flush_suit];
        
        // Straight check on mask
        // Check for 5 consecutive bits
        // Algorithm: AND with shifted versions
        // 0x1F = 11111 (5 bits)
        // Check if any 5-bit window matches
        // But we need the High Card.
        
        // Simplest: Iterate high to low
        for (int r = 12; r >= 4; r--) { // Ace down to 6
            if ((mask & (0x1F << (r-4))) == (0x1F << (r-4))) {
                // Found straight flush ending at r+2
                if (r == 12) return 9000000; // Royal
                return 8000000 + (r + 2);
            }
        }
        // Wheel check: A,5,4,3,2 (Bits 12, 3, 2, 1, 0)
        // Mask 0x100F = 1 0000 0000 1111
        if ((mask & 0x100F) == 0x100F) {
            return 8000000 + 5; // Steel Wheel
        }
        
        // Regular Flush
        // Find highest card
        int high = 0;
        for (int r = 12; r >= 0; r--) {
            if ((mask >> r) & 1) { high = r + 2; break; }
        }
        return 5000000 + high;
    }
    
    // 2. Rank Decomposition (Quads, Full House, Trips, Two Pair, Pair, High Card)
    uint8_t rank_counts[15] = {0};
    uint32_t ranks_mask = 0; // Bitmask of present ranks
    
    for (size_t i = 0; i < n; i++) {
        rank_counts[hand[i].rank]++;
        ranks_mask |= (1 << (hand[i].rank - 2));
    }
    
    int quads = 0, trips = 0, pairs = 0;
    int trips_kicker = 0; // Secondary trips for FH
    int pairs_kicker = 0; // Secondary pair
    
    for (int r = 14; r >= 2; r--) {
        int count = rank_counts[r];
        if (count == 4) { quads = r; break; } // Max 1 quad possible
        if (count == 3) {
            if (trips == 0) trips = r;
            else if (trips_kicker == 0) trips_kicker = r;
        }
        if (count == 2) {
            if (pairs == 0) pairs = r;
            else if (pairs_kicker == 0) pairs_kicker = r;
        }
    }
    
    // Four of a Kind
    if (quads) {
        // Find kicker
        for (int r = 14; r >= 2; r--) {
            if (r != quads && rank_counts[r] > 0) {
                return 7000000 + quads; // + kicker logic
            }
        }
        return 7000000 + quads;
    }
    
    // Full House (3+2 or 3+3)
    if (trips && (trips_kicker || pairs)) {
        return 6000000 + trips;
    }
    
    // 3. Straight Check (Non-flush)
    // Use ranks_mask
    // Check consecutive 5 bits
     for (int r = 12; r >= 4; r--) {
        if ((ranks_mask & (0x1F << (r-4))) == (0x1F << (r-4))) {
            return 4000000 + (r + 2);
        }
    }
    // Wheel
    if ((ranks_mask & 0x100F) == 0x100F) {
        return 4000000 + 5;
    }
    
    // Trips
    if (trips) return 3000000 + trips;
    
    // Two Pair
    if (pairs && pairs_kicker) return 2000000 + pairs * 100 + pairs_kicker;
    
    // One Pair
    if (pairs) return 1000000 + pairs * 10000;
    
    // High Card
    // Find highest
    for (int r = 14; r >= 2; r--) {
        if (rank_counts[r] > 0) return r * 10000; // Simplified
    }
    
    return 0;
}

void PHEvaluator::prefetch(const std::vector<Card>& cards) const {
    // Multiplier: Prefetching data into cache
    // __builtin_prefetch(ptr);
}
}  // namespace poker_engine
