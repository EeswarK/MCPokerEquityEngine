#ifndef CORE_CARD_H
#define CORE_CARD_H

#include <cstdint>
#include <string>

// Matches Python Card class (rank 2-14, suit 0-3)
struct Card {
    uint8_t rank;  // 2-14 (J=11, Q=12, K=13, A=14)
    uint8_t suit;  // 0-3 (hearts, diamonds, clubs, spades)

    Card() : rank(0), suit(0) {}
    Card(uint8_t r, uint8_t s) : rank(r), suit(s) {}

    bool operator==(const Card& other) const {
        return rank == other.rank && suit == other.suit;
    }

    // For debugging
    std::string to_string() const;
};

// Hash function for std::unordered_set
struct CardHash {
    std::size_t operator()(const Card& c) const {
        return (static_cast<std::size_t>(c.rank) << 8) | c.suit;
    }
};

#endif // CORE_CARD_H
