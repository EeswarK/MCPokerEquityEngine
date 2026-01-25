#ifndef CORE_DECK_H
#define CORE_DECK_H

#include "card.h"
#include <unordered_set>
#include <vector>
#include <random>

// Deck represented as unordered_set for O(1) removal
// Matches Python's set-based deck approach
class Deck {
private:
    std::unordered_set<Card, CardHash> cards_;
    std::mt19937 rng_;

public:
    Deck();
    explicit Deck(uint32_t seed);

    // Remove card from deck (Python: deck.discard())
    void remove(const Card& card);

    // Reset deck to full 52 cards
    void reset();

    // Check if deck contains card
    bool contains(const Card& card) const;

    // Get random card (Python: random.choice(list(deck)))
    Card draw_random();

    // Sample N cards without replacement (Python: random.sample(list(deck), n))
    std::vector<Card> sample(size_t n);

    // Remaining card count
    size_t size() const { return cards_.size(); }

    // For testing: get all cards
    std::vector<Card> all_cards() const;
};

#endif // CORE_DECK_H
