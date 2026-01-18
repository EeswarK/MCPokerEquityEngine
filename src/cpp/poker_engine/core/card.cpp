#include "core/card.h"

std::string Card::to_string() const {
    static const char* ranks = "??23456789TJQKA";
    static const char* suits = "hdcs";

    std::string result;
    if (rank >= 2 && rank <= 14) {
        result += ranks[rank];
    } else {
        result += '?';
    }
    if (suit <= 3) {
        result += suits[suit];
    } else {
        result += '?';
    }
    return result;
}
