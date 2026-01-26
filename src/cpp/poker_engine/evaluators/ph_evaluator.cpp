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
    uint32_t ranks_mask = 0;
    uint8_t rank_counts[15] = {0};
    uint8_t suit_counts[4] = {0};
    uint32_t suit_masks[4] = {0};
    std::vector<uint8_t> all_ranks;

    auto process_card = [&](const Card& c) {
        ranks_mask |= (1 << (c.rank - 2));
        rank_counts[c.rank]++;
        suit_counts[c.suit]++;
        suit_masks[c.suit] |= (1 << (c.rank - 2));
        all_ranks.push_back(c.rank);
    };

    for (const auto& c : hole_cards) process_card(c);
    for (const auto& c : board_cards) process_card(c);

    // 1. Flush Check
    for (int s = 0; s < 4; s++) {
        if (suit_counts[s] >= 5) {
            uint32_t mask = suit_masks[s];
            // SF check
            for (int r = 12; r >= 4; r--) {
                if ((mask & (0x1F << (r - 4))) == (0x1F << (r - 4))) {
                    if (r == 12) return encode_score(HandType::ROYAL_FLUSH, {14, 13, 12, 11, 10});
                    return encode_score(HandType::STRAIGHT_FLUSH, {static_cast<uint8_t>(r + 2)});
                }
            }
            if ((mask & 0x100F) == 0x100F) return encode_score(HandType::STRAIGHT_FLUSH, {5});
            
            std::vector<uint8_t> flush_ranks;
            for(int r=14; r>=2; r--) if((mask >> (r-2)) & 1) flush_ranks.push_back(r);
            return encode_score(HandType::FLUSH, flush_ranks);
        }
    }

    // 2. Quads
    for (int r = 14; r >= 2; r--) {
        if (rank_counts[r] == 4) {
            uint8_t kicker = 0;
            for(int k=14; k>=2; k--) if(rank_counts[k] > 0 && k != r) { kicker = k; break; }
            return encode_score(HandType::FOUR_OF_KIND, {static_cast<uint8_t>(r), kicker});
        }
    }

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
    if (trips && pair) return encode_score(HandType::FULL_HOUSE, {static_cast<uint8_t>(trips), static_cast<uint8_t>(pair)});

    // 4. Straight
    for (int r = 12; r >= 4; r--) {
        if ((ranks_mask & (0x1F << (r - 4))) == (0x1F << (r - 4))) return encode_score(HandType::STRAIGHT, {static_cast<uint8_t>(r + 2)});
    }
    if ((ranks_mask & 0x100F) == 0x100F) return encode_score(HandType::STRAIGHT, {5});

    // 5. Trips
    if (trips) {
        std::vector<uint8_t> kickers;
        for(int k=14; k>=2; k--) if(rank_counts[k] > 0 && k != trips) kickers.push_back(k);
        return encode_score(HandType::THREE_OF_KIND, {static_cast<uint8_t>(trips), kickers[0], kickers[1]});
    }

    // 6. Two Pair
    int p1 = 0, p2 = 0;
    for (int r = 14; r >= 2; r--) {
        if (rank_counts[r] == 2) {
            if (p1 == 0) p1 = r;
            else if (p2 == 0) p2 = r;
        }
    }
    if (p1 && p2) {
        uint8_t kicker = 0;
        for(int k=14; k>=2; k--) if(rank_counts[k] > 0 && k != p1 && k != p2) { kicker = k; break; }
        return encode_score(HandType::TWO_PAIR, {static_cast<uint8_t>(p1), static_cast<uint8_t>(p2), kicker});
    }

    // 7. One Pair
    if (p1) {
        std::vector<uint8_t> kickers;
        for(int k=14; k>=2; k--) if(rank_counts[k] > 0 && k != p1) kickers.push_back(k);
        return encode_score(HandType::ONE_PAIR, {static_cast<uint8_t>(p1), kickers[0], kickers[1], kickers[2]});
    }

    // 8. High Card
    std::vector<uint8_t> sorted = all_ranks;
    std::sort(sorted.rbegin(), sorted.rend());
    return encode_score(HandType::HIGH_CARD, sorted);
}

void PHEvaluator::prefetch(const std::vector<Card>& cards) const {
    // Multiplier: Prefetching data into cache
    // __builtin_prefetch(ptr);
}
}  // namespace poker_engine
