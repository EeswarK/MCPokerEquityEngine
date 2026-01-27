#include "ph_evaluator_tables.h"
#include "hand_types.h"
#include <algorithm>
#include <vector>
#include <cstdint>

namespace poker_engine {

// Binomial coefficient calculation: C(n, k) = n! / (k! * (n-k)!)
// Used for combinatorial indexing in the hash table
uint32_t binomial_coefficient(uint32_t n, uint32_t k) {
    if (k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k;  // Optimization: C(n,k) = C(n,n-k)

    uint64_t result = 1;
    for (uint32_t i = 0; i < k; ++i) {
        result = result * (n - i) / (i + 1);
    }
    return static_cast<uint32_t>(result);
}

// Populate the hash table for combinatorial indexing
// hash[i][j] = C(j + i, i + 1)
// This maps each unique 7-card multiset to a dense index 0-50387
void populate_hash_table(uint32_t hash_table[7][13]) {
    for (int i = 0; i < 7; ++i) {
        for (int j = 0; j < 13; ++j) {
            hash_table[i][j] = binomial_coefficient(j + i, i + 1);
        }
    }
}

// Helper: Count set bits in a 32-bit integer
inline int popcount(uint32_t x) {
    return __builtin_popcount(x);
}

// Helper: Check if mask contains a straight (5 consecutive bits)
// Returns the high card rank (2-14) if straight found, 0 otherwise
uint8_t get_straight_high(uint32_t mask) {
    // Check for standard straights (5 consecutive bits)
    for (int r = 12; r >= 4; r--) {
        uint32_t straight_mask = 0x1F << (r - 4);  // 5 bits starting at position r-4
        if ((mask & straight_mask) == straight_mask) {
            return r + 2;  // Convert bit position to rank
        }
    }

    // Check for wheel straight (A-2-3-4-5): bits 12,3,2,1,0
    if ((mask & 0x100F) == 0x100F) {
        return 5;  // Ace acts as low card, 5 is high
    }

    return 0;  // No straight
}

// Helper: Extract top N ranks from mask in descending order
std::vector<uint8_t> extract_top_ranks(uint32_t mask, int count) {
    std::vector<uint8_t> ranks;
    for (int r = 12; r >= 0; r--) {  // Bit 12 = Ace, bit 0 = Deuce
        if ((mask >> r) & 1) {
            ranks.push_back(r + 2);  // Convert bit position to rank
            if (ranks.size() == static_cast<size_t>(count)) break;
        }
    }
    return ranks;
}

// Populate flush table: 8192 entries for all 13-bit masks
// Each entry encodes the score for a flush hand with that rank pattern
void populate_flush_table(int32_t flush_table[8192]) {
    for (uint32_t mask = 0; mask < 8192; mask++) {
        int bits = popcount(mask);

        // Skip invalid masks (< 5 cards)
        if (bits < 5) {
            flush_table[mask] = 0;  // Invalid
            continue;
        }

        // Check for straight flush
        uint8_t straight_high = get_straight_high(mask);
        if (straight_high > 0) {
            // Royal flush: A-K-Q-J-T (high card = 14)
            if (straight_high == 14) {
                flush_table[mask] = encode_score(HandType::ROYAL_FLUSH, {14, 13, 12, 11, 10});
            } else {
                // Regular straight flush
                flush_table[mask] = encode_score(HandType::STRAIGHT_FLUSH, {straight_high});
            }
        } else {
            // Regular flush - take top 5 ranks
            std::vector<uint8_t> ranks = extract_top_ranks(mask, 5);
            flush_table[mask] = encode_score(HandType::FLUSH, ranks);
        }
    }
}

// Helper: Build rank histogram from multiset
void build_histogram(const std::vector<uint8_t>& multiset, uint8_t histogram[13]) {
    for (int i = 0; i < 13; i++) histogram[i] = 0;
    for (uint8_t rank : multiset) {
        histogram[rank]++;
    }
}

// Helper: Check if ranks form a straight
bool is_straight(const std::vector<uint8_t>& unique_ranks) {
    if (unique_ranks.size() < 5) return false;

    // Build bitmask of present ranks
    // unique_ranks contains ranks in 2-14 format, convert to bit positions 0-12
    uint32_t mask = 0;
    for (uint8_t r : unique_ranks) {
        mask |= (1 << (r - 2));  // Convert rank to bit position
    }

    return get_straight_high(mask) > 0;
}

// Evaluate a rank multiset (7 ranks) and return its score
// multiset is in non-decreasing order with 0-12 indexing (0=Deuce, 12=Ace)
int32_t evaluate_rank_multiset(const std::vector<uint8_t>& multiset) {
    uint8_t histogram[13];
    build_histogram(multiset, histogram);

    // Extract ranks by count (for classification)
    // Convert to 2-14 indexing for encode_score
    // Note: multisets can have impossible counts like 7 of a kind (used in table generation)
    std::vector<uint8_t> quads, trips, pairs, singles;
    for (int r = 12; r >= 0; r--) {  // Check in descending order (Ace to Deuce)
        uint8_t count = histogram[r];
        if (count >= 4) {
            quads.push_back(r + 2);  // 4+ of a kind (treat as quads for best 5-card hand)
        } else if (count == 3) {
            trips.push_back(r + 2);
        } else if (count == 2) {
            pairs.push_back(r + 2);
        } else if (count == 1) {
            singles.push_back(r + 2);
        }
        // count == 0: skip
    }

    // Four of a kind
    if (!quads.empty()) {
        std::vector<uint8_t> key_ranks = {quads[0]};
        if (!trips.empty()) key_ranks.push_back(trips[0]);
        else if (!pairs.empty()) key_ranks.push_back(pairs[0]);
        else if (!singles.empty()) key_ranks.push_back(singles[0]);
        return encode_score(HandType::FOUR_OF_KIND, key_ranks);
    }

    // Full house
    if (!trips.empty() && !pairs.empty()) {
        return encode_score(HandType::FULL_HOUSE, {trips[0], pairs[0]});
    }
    if (trips.size() >= 2) {
        return encode_score(HandType::FULL_HOUSE, {trips[0], trips[1]});
    }

    // Straight (no flush in non-flush table)
    std::vector<uint8_t> unique_ranks;
    for (int r = 12; r >= 0; r--) {
        if (histogram[r] > 0) unique_ranks.push_back(r + 2);
    }
    if (is_straight(unique_ranks)) {
        // Build mask and get straight high
        uint32_t mask = 0;
        for (uint8_t r : unique_ranks) {
            mask |= (1 << (r - 2));
        }
        uint8_t straight_high = get_straight_high(mask);
        return encode_score(HandType::STRAIGHT, {straight_high});
    }

    // Three of a kind
    if (!trips.empty()) {
        std::vector<uint8_t> key_ranks = {trips[0]};
        if (!pairs.empty()) key_ranks.push_back(pairs[0]);
        if (key_ranks.size() < 2 && !singles.empty()) key_ranks.push_back(singles[0]);
        if (key_ranks.size() < 3 && singles.size() > 1) key_ranks.push_back(singles[1]);
        return encode_score(HandType::THREE_OF_KIND, key_ranks);
    }

    // Two pair
    if (pairs.size() >= 2) {
        std::vector<uint8_t> key_ranks = {pairs[0], pairs[1]};

        // Best kicker is the highest of: third pair (if exists) or best single
        uint8_t best_kicker = 0;
        if (pairs.size() >= 3) best_kicker = pairs[2];
        if (!singles.empty() && singles[0] > best_kicker) best_kicker = singles[0];

        if (best_kicker > 0) key_ranks.push_back(best_kicker);
        return encode_score(HandType::TWO_PAIR, key_ranks);
    }

    // One pair
    if (pairs.size() == 1) {
        std::vector<uint8_t> key_ranks = {pairs[0]};
        for (int i = 0; i < 3 && i < static_cast<int>(singles.size()); i++) {
            key_ranks.push_back(singles[i]);
        }
        return encode_score(HandType::ONE_PAIR, key_ranks);
    }

    // High card
    return encode_score(HandType::HIGH_CARD, singles);
}

// Advance multiset to next colexicographical combination
// Multiset stored as [rank0, rank1, ..., rank6] where rank is 0-12
bool next_colex_combination(std::vector<uint8_t>& multiset) {
    int i = multiset.size() - 1;

    // Find rightmost element that can be incremented
    while (i >= 0 && multiset[i] == 12) {  // 12 = Ace
        i--;
    }

    if (i < 0) return false;  // No more combinations

    // Increment this element
    multiset[i]++;

    // Reset all elements to the right
    for (size_t j = i + 1; j < multiset.size(); j++) {
        multiset[j] = multiset[i];
    }

    return true;
}

// Populate rank table: 50,388 entries for all 7-card non-flush combinations
// Uses colexicographical enumeration of multisets
void populate_rank_table(int32_t rank_table[50388], const uint32_t hash_table[7][13]) {
    std::vector<uint8_t> multiset(7, 0);  // Start with seven deuces [0,0,0,0,0,0,0]

    do {
        // CRITICAL: Compute the hash index for this multiset
        // The index is NOT just a sequential counter!
        uint32_t index = 0;
        for (int i = 0; i < 7; i++) {
            index += hash_table[i][multiset[i]];
        }

        // Store the evaluation at the computed hash index
        rank_table[index] = evaluate_rank_multiset(multiset);
    } while (next_colex_combination(multiset));
}

}  // namespace poker_engine
