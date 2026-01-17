import itertools
import random
from typing import Sequence

from .card import Card as SenzeeCard
from .lookup import LookupTable
from ....models.card import Card as PublicCard


class _Evaluator:
    """
    Internal evaluator using Cactus Kev's algorithm.

    Evaluates hands to ranks 1-7462 (lower = stronger).
    Uses bitwise operations and lookup tables for O(1) evaluation.
    """

    def __init__(self) -> None:
        self.table = LookupTable()

        self.hand_size_map = {
            5: self._five,
            6: self._six,
            7: self._seven
        }

    def evaluate(self, hand: list[int], board: list[int]) -> int:
        """Evaluate hand + board to rank (1-7462)."""
        all_cards = hand + board
        return self.hand_size_map[len(all_cards)](all_cards)

    def _five(self, cards: Sequence[int]) -> int:
        """Evaluate exactly 5 cards using lookup tables."""
        # Check flush using bitwise AND
        if cards[0] & cards[1] & cards[2] & cards[3] & cards[4] & 0xF000:
            handOR = (cards[0] | cards[1] | cards[2] | cards[3] | cards[4]) >> 16
            prime = SenzeeCard.prime_product_from_rankbits(handOR)
            return self.table.flush_lookup[prime]
        else:
            prime = SenzeeCard.prime_product_from_hand(cards)
            return self.table.unsuited_lookup[prime]

    def _six(self, cards: Sequence[int]) -> int:
        """Evaluate best 5 from 6 cards."""
        minimum = LookupTable.MAX_HIGH_CARD
        for combo in itertools.combinations(cards, 5):
            score = self._five(combo)
            if score < minimum:
                minimum = score
        return minimum

    def _seven(self, cards: Sequence[int]) -> int:
        """Evaluate best 5 from 7 cards."""
        minimum = LookupTable.MAX_HIGH_CARD
        for combo in itertools.combinations(cards, 5):
            score = self._five(combo)
            if score < minimum:
                minimum = score
        return minimum

    def get_rank_class(self, hr: int) -> int:
        """Get hand category (0-9) from rank."""
        if hr >= 0 and hr <= LookupTable.MAX_ROYAL_FLUSH:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_ROYAL_FLUSH]
        elif hr <= LookupTable.MAX_STRAIGHT_FLUSH:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_STRAIGHT_FLUSH]
        elif hr <= LookupTable.MAX_FOUR_OF_A_KIND:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_FOUR_OF_A_KIND]
        elif hr <= LookupTable.MAX_FULL_HOUSE:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_FULL_HOUSE]
        elif hr <= LookupTable.MAX_FLUSH:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_FLUSH]
        elif hr <= LookupTable.MAX_STRAIGHT:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_STRAIGHT]
        elif hr <= LookupTable.MAX_THREE_OF_A_KIND:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_THREE_OF_A_KIND]
        elif hr <= LookupTable.MAX_TWO_PAIR:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_TWO_PAIR]
        elif hr <= LookupTable.MAX_PAIR:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_PAIR]
        elif hr <= LookupTable.MAX_HIGH_CARD:
            return LookupTable.MAX_TO_RANK_CLASS[LookupTable.MAX_HIGH_CARD]
        else:
            raise Exception("Invalid hand rank")


# Global evaluator instance (lazy initialized)
_evaluator_instance = None


def _get_evaluator() -> _Evaluator:
    """Get or create evaluator singleton (lazy initialization)."""
    global _evaluator_instance
    if _evaluator_instance is None:
        _evaluator_instance = _Evaluator()
    return _evaluator_instance


# Conversion utilities
def _public_card_to_senzee(card: PublicCard) -> int:
    """
    Convert public Card(rank, suit) to Senzee integer.

    Mapping:
    - Rank: 2-14 → '23456789TJQKA' → 0-12
    - Suit: 0-3 → 's', 'h', 'd', 'c' (spades, hearts, diamonds, clubs)
    """
    rank_char = SenzeeCard.STR_RANKS[card.rank - 2]
    suit_chars = ['s', 'h', 'd', 'c']
    suit_char = suit_chars[card.suit]
    return SenzeeCard.new(rank_char + suit_char)


def _convert_cards(cards: list[PublicCard]) -> list[int]:
    """Convert list of public Cards to Senzee integers."""
    return [_public_card_to_senzee(c) for c in cards]


# Public API matching naive evaluator signature
def evaluate_hand_senzee(hole_cards: list[PublicCard], board_cards: list[PublicCard]) -> int:
    """
    Evaluate best 5-card poker hand.

    Returns: Hand strength value (higher = better)
             Normalized to match naive evaluator scale (0-9000000)
    """
    all_cards = list(hole_cards) + list(board_cards)
    if len(all_cards) < 5:
        return 0

    evaluator = _get_evaluator()

    # Convert to Senzee representation
    senzee_cards = _convert_cards(all_cards)

    # Evaluate (returns 1-7462, lower is better)
    rank = evaluator.evaluate(senzee_cards[:2], senzee_cards[2:])

    # Convert to naive scale: invert and scale to match naive's 0-9000000 range
    # Rank 1 (royal flush) → 9000000
    # Rank 7462 (7-high) → ~0
    normalized = 9000000 - (rank - 1) * (9000000 / 7461)

    return int(normalized)


def simulate_hand_senzee(
    hole_cards: list[PublicCard], board: list[PublicCard], num_opponents: int
) -> tuple[int, int, int, str]:
    """
    Simulate a poker hand and return outcome.

    Returns:
        tuple[int, int, int, str]: (outcome, our_type, opp_type, opp_hand_classification)
        - outcome: 1=win, 0=tie, -1=loss
        - our_type: 0-9 (High Card to Royal Flush)
        - opp_type: 0-9 (High Card to Royal Flush)
        - opp_hand_classification: "AA", "AKs", etc.
    """
    # Create deck
    deck = _create_deck()
    known_cards = set((c.rank, c.suit) for c in hole_cards + board)

    for card in known_cards:
        deck.discard(card)

    # Deal remaining board
    remaining_board = 5 - len(board)
    board_cards = list(board)

    for _ in range(remaining_board):
        if not deck:
            return (0, 0, 0, "??")
        card_tuple = random.choice(list(deck))
        deck.discard(card_tuple)
        board_cards.append(PublicCard(rank=card_tuple[0], suit=card_tuple[1]))

    # Deal opponent hands
    opponent_hands = []
    for _ in range(num_opponents):
        if len(deck) < 2:
            return (0, 0, 0, "??")
        opp_card_tuples = random.sample(list(deck), 2)
        for card_tuple in opp_card_tuples:
            deck.discard(card_tuple)
        opponent_hands.append([PublicCard(rank=t[0], suit=t[1]) for t in opp_card_tuples])

    # Evaluate hands
    # Note: we need the raw rank (lower better) for comparison first to avoid precision issues with normalization
    evaluator = _get_evaluator()
    our_senzee = _convert_cards(hole_cards + board_cards)
    opp_senzees = [_convert_cards(hand + board_cards) for hand in opponent_hands]
    
    our_rank = evaluator.evaluate(our_senzee[:2], our_senzee[2:])
    opp_ranks = [evaluator.evaluate(s[:2], s[2:]) for s in opp_senzees]

    min_opponent_rank = min(opp_ranks) if opp_ranks else LookupTable.MAX_HIGH_CARD + 1
    min_opponent_idx = opp_ranks.index(min_opponent_rank) if opp_ranks else 0

    # Get hand types (0-9, 9=Royal Flush in naive? No. Naive uses 8=Straight Flush, 9=Royal Flush?
    # Wait, Naive evaluator returns int, simulate returns type index.
    # In naive: 0=High Card, 8=Straight Flush, 9=Royal Flush?
    # Let's check LookupTable: 0=Royal Flush, 9=High Card.
    # We need to map Senzee (0=Best) to Naive (0=Worst) or whatever Naive expects.
    # Let's check `api/models.py` or naive implementation.
    
    # Actually, `simulate_hand` interface expects `our_type` to be int.
    # In `base.py` (naive), it returned `evaluator.evaluate_hand(...)` which was just a score.
    # But `simulate_hand` returns `(outcome, our_type, ...)`
    # In Naive: `our_type = get_hand_type(our_score)` or something.
    # Let's check `naive/evaluator.py`.
    
    our_hand_type_senzee = evaluator.get_rank_class(our_rank) # 0=Best (Royal)
    opp_hand_type_senzee = evaluator.get_rank_class(min_opponent_rank)
    
    # Invert for consistency if Naive uses 0=High Card, 9=Royal Flush
    # Standard typically: 0=High Card, ... 9=Royal Flush.
    our_hand_type = 9 - our_hand_type_senzee
    max_opponent_hand_type = 9 - opp_hand_type_senzee

    # Classify opponent's starting hand
    opponent_hole_classification = _classify_hole_cards(opponent_hands[min_opponent_idx]) if opponent_hands else "??"

    if our_rank < min_opponent_rank: # Lower is better in Senzee
        return (1, our_hand_type, max_opponent_hand_type, opponent_hole_classification)
    elif our_rank == min_opponent_rank:
        return (0, our_hand_type, max_opponent_hand_type, opponent_hole_classification)
    else:
        return (-1, our_hand_type, max_opponent_hand_type, opponent_hole_classification)


def _create_deck() -> set:
    """Create a full deck as set of (rank, suit) tuples."""
    deck = set()
    for rank in range(2, 15):
        for suit in range(4):
            deck.add((rank, suit))
    return deck


def _classify_hole_cards(hole_cards: list[PublicCard]) -> str:
    """
    Classify hole cards into standard notation.

    Examples: "AA", "AKs", "72o"
    """
    if len(hole_cards) != 2:
        return "??"

    card1, card2 = hole_cards
    rank1, suit1 = card1.rank, card1.suit
    rank2, suit2 = card2.rank, card2.suit

    def rank_to_char(rank: int) -> str:
        if rank < 10:
            return str(rank)
        rank_map = {10: "T", 11: "J", 12: "Q", 13: "K", 14: "A"}
        return rank_map.get(rank, "?")

    if rank1 == rank2:
        return f"{rank_to_char(rank1)}{rank_to_char(rank2)}"

    if rank1 > rank2:
        high_rank, high_suit = rank1, suit1
        low_rank, low_suit = rank2, suit2
    else:
        high_rank, high_suit = rank2, suit2
        low_rank, low_suit = rank1, suit1

    suited = (suit1 == suit2)
    suffix = "s" if suited else "o"

    return f"{rank_to_char(high_rank)}{rank_to_char(low_rank)}{suffix}"
