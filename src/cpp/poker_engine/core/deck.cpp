#include "core/deck.h"
#include <algorithm>
#include <stdexcept>

Deck::Deck() : rng_(std::random_device{}()) {
    // Create full 52-card deck (matches Python: range(2, 15) x range(4))
    for (uint8_t rank = 2; rank <= 14; ++rank) {
        for (uint8_t suit = 0; suit < 4; ++suit) {
            cards_.insert(Card(rank, suit));
        }
    }
}

Deck::Deck(uint32_t seed) : rng_(seed) {
    for (uint8_t rank = 2; rank <= 14; ++rank) {
        for (uint8_t suit = 0; suit < 4; ++suit) {
            cards_.insert(Card(rank, suit));
        }
    }
}

void Deck::remove(const Card& card) {
    cards_.erase(card);
}

void Deck::reset() {
    cards_.clear();
    for (uint8_t r = 2; r <= 14; ++r) {
        for (uint8_t s = 0; s < 4; ++s) {
            cards_.insert(Card(r, s));
        }
    }
}

bool Deck::contains(const Card& card) const {
    return cards_.find(card) != cards_.end();
}

Card Deck::draw_random() {
    if (cards_.empty()) {
        throw std::runtime_error("Cannot draw from empty deck");
    }

    // Convert set to vector and select random element
    std::vector<Card> card_vec(cards_.begin(), cards_.end());
    std::uniform_int_distribution<size_t> dist(0, card_vec.size() - 1);
    Card drawn = card_vec[dist(rng_)];
    cards_.erase(drawn);
    return drawn;
}

std::vector<Card> Deck::sample(size_t n) {
    if (n > cards_.size()) {
        throw std::runtime_error("Cannot sample more cards than available");
    }

    std::vector<Card> result;
    result.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        result.push_back(draw_random());
    }

    return result;
}

std::vector<Card> Deck::all_cards() const {
    return std::vector<Card>(cards_.begin(), cards_.end());
}
