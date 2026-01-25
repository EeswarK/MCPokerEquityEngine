#include "naive_evaluator.h"
#include "core/deck.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace poker_engine {

// Matches: src/python/engine/strategies/naive/evaluator.py:6-27
int32_t NaiveEvaluator::evaluate_hand(
    const std::vector<Card>& hole_cards,
    const std::vector<Card>& board_cards) const {

    // Combine hole cards and board
    std::vector<Card> all_cards = hole_cards;
    all_cards.insert(all_cards.end(), board_cards.begin(), board_cards.end());

    if (all_cards.size() < 5) {
        return 0;
    }

    int32_t best_value = 0;
    size_t n = all_cards.size();

    // Iterate all C(n,5) combinations using 5 nested loops
    // EXACT match of Python lines 12-25
    for (size_t i = 0; i < n - 4; ++i) {
        for (size_t j = i + 1; j < n - 3; ++j) {
            for (size_t k = j + 1; k < n - 2; ++k) {
                for (size_t l = k + 1; l < n - 1; ++l) {
                    for (size_t m = l + 1; m < n; ++m) {
                        std::vector<Card> hand = {
                            all_cards[i],
                            all_cards[j],
                            all_cards[k],
                            all_cards[l],
                            all_cards[m]
                        };

                        int32_t value = evaluate_five_cards(hand);
                        best_value = std::max(best_value, value);
                    }
                }
            }
        }
    }

    return best_value;
}

// Matches: src/python/engine/strategies/naive/evaluator.py:94-142
int32_t NaiveEvaluator::evaluate_five_cards(const std::vector<Card>& cards) const {
    // Extract ranks and suits
    std::vector<uint8_t> ranks;
    std::vector<uint8_t> suits;
    for (const auto& card : cards) {
        ranks.push_back(card.rank);
        suits.push_back(card.suit);
    }

    // Count rank occurrences
    std::unordered_map<uint8_t, int> rank_counts;
    for (uint8_t rank : ranks) {
        rank_counts[rank]++;
    }

    // Get sorted count pattern (e.g., [3, 2] for full house)
    std::vector<int> counts;
    for (const auto& pair : rank_counts) {
        counts.push_back(pair.second);
    }
    std::sort(counts.rbegin(), counts.rend());  // Descending

    bool is_flush = (std::unordered_set<uint8_t>(suits.begin(), suits.end()).size() == 1);
    bool is_str = is_straight(ranks);

    // Royal/Straight Flush (Python lines 106-109)
    if (is_str && is_flush) {
        uint8_t max_rank = *std::max_element(ranks.begin(), ranks.end());
        uint8_t min_rank = *std::min_element(ranks.begin(), ranks.end());
        if (max_rank == 14 && min_rank == 10) {
            return 9000000;  // Royal flush
        }
        return 8000000 + max_rank;  // Straight flush
    }

    // Four of a Kind (Python lines 111-112)
    if (counts == std::vector<int>{4, 1}) {
        uint8_t quad_rank = 0;
        for (const auto& pair : rank_counts) {
            if (pair.second == 4) quad_rank = pair.first;
        }
        return 7000000 + quad_rank;
    }

    // Full House (Python lines 114-115)
    if (counts == std::vector<int>{3, 2}) {
        uint8_t trips_rank = 0;
        for (const auto& pair : rank_counts) {
            if (pair.second == 3) trips_rank = pair.first;
        }
        return 6000000 + trips_rank;
    }

    // Flush (Python lines 117-118)
    if (is_flush) {
        return 5000000 + *std::max_element(ranks.begin(), ranks.end());
    }

    // Straight (Python lines 120-121)
    if (is_str) {
        return 4000000 + *std::max_element(ranks.begin(), ranks.end());
    }

    // Three of a Kind (Python lines 123-124)
    if (counts == std::vector<int>{3, 1, 1}) {
        uint8_t trips_rank = 0;
        for (const auto& pair : rank_counts) {
            if (pair.second == 3) trips_rank = pair.first;
        }
        return 3000000 + trips_rank;
    }

    // Two Pair (Python lines 126-128)
    if (counts == std::vector<int>{2, 2, 1}) {
        std::vector<uint8_t> pairs;
        for (const auto& pair : rank_counts) {
            if (pair.second == 2) pairs.push_back(pair.first);
        }
        uint8_t max_pair = *std::max_element(pairs.begin(), pairs.end());
        uint8_t min_pair = *std::min_element(pairs.begin(), pairs.end());
        return 2000000 + max_pair * 100 + min_pair;
    }

    // One Pair (Python lines 130-133)
    if (counts == std::vector<int>{2, 1, 1, 1}) {
        uint8_t pair_rank = 0;
        for (const auto& pair : rank_counts) {
            if (pair.second == 2) pair_rank = pair.first;
        }

        std::vector<uint8_t> kickers;
        for (uint8_t rank : ranks) {
            if (rank != pair_rank) kickers.push_back(rank);
        }
        std::sort(kickers.rbegin(), kickers.rend());

        return 1000000 + pair_rank * 10000 + kickers[0] * 100 + kickers[1];
    }

    // High Card (Python lines 135-142)
    std::vector<uint8_t> sorted_ranks = ranks;
    std::sort(sorted_ranks.rbegin(), sorted_ranks.rend());
    return sorted_ranks[0] * 10000
         + sorted_ranks[1] * 1000
         + sorted_ranks[2] * 100
         + sorted_ranks[3] * 10
         + sorted_ranks[4];
}

// Matches: src/python/engine/strategies/naive/evaluator.py:145-161
bool NaiveEvaluator::is_straight(const std::vector<uint8_t>& ranks) const {
    std::vector<uint8_t> sorted_ranks(ranks.begin(), ranks.end());
    std::sort(sorted_ranks.begin(), sorted_ranks.end());
    auto last = std::unique(sorted_ranks.begin(), sorted_ranks.end());
    sorted_ranks.erase(last, sorted_ranks.end());

    if (sorted_ranks.size() < 5) {
        return false;
    }

    // Check regular straights
    for (size_t i = 0; i <= sorted_ranks.size() - 5; ++i) {
        if (sorted_ranks[i + 4] - sorted_ranks[i] == 4) {
            return true;
        }
    }

    // Check Ace-low straight (A-2-3-4-5)
    if (std::find(sorted_ranks.begin(), sorted_ranks.end(), 14) != sorted_ranks.end()) {
        std::vector<uint8_t> low_ace_ranks;
        for (uint8_t rank : sorted_ranks) {
            low_ace_ranks.push_back(rank == 14 ? 1 : rank);
        }
        std::sort(low_ace_ranks.begin(), low_ace_ranks.end());
        auto last_ace = std::unique(low_ace_ranks.begin(), low_ace_ranks.end());
        low_ace_ranks.erase(last_ace, low_ace_ranks.end());

        for (size_t i = 0; i <= low_ace_ranks.size() - 5; ++i) {
            if (low_ace_ranks[i + 4] - low_ace_ranks[i] == 4) {
                return true;
            }
        }
    }

    return false;
}

// Matches: src/python/engine/strategies/naive/evaluator.py:30-83
SimulationResult NaiveEvaluator::simulate_hand(
    const std::vector<Card>& hole_cards,
    const std::vector<Card>& board,
    int num_opponents) const {

    Deck deck;

    // Remove known cards from deck
    for (const auto& card : hole_cards) {
        deck.remove(card);
    }
    for (const auto& card : board) {
        deck.remove(card);
    }

    // Complete the board (deal remaining cards)
    std::vector<Card> board_cards = board;
    int remaining_board = 5 - static_cast<int>(board.size());

    for (int i = 0; i < remaining_board; ++i) {
        if (deck.size() == 0) {
            return {0, HIGH_CARD, HIGH_CARD, "??"};
        }
        board_cards.push_back(deck.draw_random());
    }

    // Deal opponent hands
    std::vector<std::vector<Card>> opponent_hands;
    for (int i = 0; i < num_opponents; ++i) {
        if (deck.size() < 2) {
            return {0, HIGH_CARD, HIGH_CARD, "??"};
        }
        opponent_hands.push_back(deck.sample(2));
    }

    // Evaluate hands
    int32_t our_value = evaluate_hand(hole_cards, board_cards);

    std::vector<int32_t> opponent_values;
    for (const auto& opp_hand : opponent_hands) {
        opponent_values.push_back(evaluate_hand(opp_hand, board_cards));
    }

    int32_t max_opponent = opponent_values.empty() ? 0 :
        *std::max_element(opponent_values.begin(), opponent_values.end());

    size_t max_opp_idx = 0;
    if (!opponent_values.empty()) {
        max_opp_idx = std::distance(
            opponent_values.begin(),
            std::max_element(opponent_values.begin(), opponent_values.end())
        );
    }

    HandType our_type = get_hand_type(our_value);
    HandType max_opp_type = get_hand_type(max_opponent);

    std::string opp_classification = opponent_hands.empty() ?
        "??" : classify_hole_cards(opponent_hands[max_opp_idx]);

    // Determine outcome
    int outcome;
    if (our_value > max_opponent) {
        outcome = 1;   // Win
    } else if (our_value == max_opponent) {
        outcome = 0;   // Tie
    } else {
        outcome = -1;  // Loss
    }

    return {outcome, our_type, max_opp_type, opp_classification};
}

// Matches: src/python/engine/strategies/naive/evaluator.py:202-245
std::string NaiveEvaluator::classify_hole_cards(const std::vector<Card>& hole_cards) const {
    if (hole_cards.size() != 2) {
        return "??";
    }

    uint8_t rank1 = hole_cards[0].rank;
    uint8_t rank2 = hole_cards[1].rank;
    uint8_t suit1 = hole_cards[0].suit;
    uint8_t suit2 = hole_cards[1].suit;

    // Convert rank to character
    auto rank_to_char = [](uint8_t rank) -> char {
        if (rank < 10) return '0' + rank;
        switch (rank) {
            case 10: return 'T';
            case 11: return 'J';
            case 12: return 'Q';
            case 13: return 'K';
            case 14: return 'A';
            default: return '?';
        }
    };

    // Pocket pair
    if (rank1 == rank2) {
        return std::string(1, rank_to_char(rank1)) + rank_to_char(rank2);
    }

    // Sort by rank (higher first)
    uint8_t high_rank = std::max(rank1, rank2);
    uint8_t low_rank = std::min(rank1, rank2);

    // Suited or offsuit
    bool suited = (suit1 == suit2);
    char suffix = suited ? 's' : 'o';

    return std::string(1, rank_to_char(high_rank)) + rank_to_char(low_rank) + suffix;
}
}  // namespace poker_engine
