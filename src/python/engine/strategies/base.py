import random

from ...models.card import Card


def evaluate_hand_base(hole_cards: list[Card], board_cards: list[Card]) -> int:
    all_cards = list(hole_cards) + list(board_cards)
    if len(all_cards) < 5:
        return 0

    best_value = 0
    for i in range(len(all_cards)):
        for j in range(i + 1, len(all_cards)):
            for k in range(j + 1, len(all_cards)):
                for idx_l in range(k + 1, len(all_cards)):
                    for m in range(idx_l + 1, len(all_cards)):
                        hand = [
                            all_cards[i],
                            all_cards[j],
                            all_cards[k],
                            all_cards[idx_l],
                            all_cards[m],
                        ]
                        value = _evaluate_five_cards(hand)
                        best_value = max(best_value, value)

    return best_value


def simulate_hand_base(hole_cards: list[Card], board: list[Card], num_opponents: int) -> tuple[int, int, int]:
    """
    Simulate a poker hand and return outcome with hand type information.

    Returns:
        tuple[int, int, int]: (outcome, our_hand_type, max_opponent_hand_type)
        - outcome: 1=win, 0=tie, -1=loss
        - our_hand_type: 0-9 (High Card to Royal Flush)
        - max_opponent_hand_type: 0-9 (High Card to Royal Flush)
    """
    deck = _create_deck()
    known_cards = set((c.rank, c.suit) for c in hole_cards + board)

    for card in known_cards:
        deck.discard(card)

    remaining_board = 5 - len(board)
    board_cards = list(board)

    for _ in range(remaining_board):
        if not deck:
            return (0, 0, 0)
        card_tuple = random.choice(list(deck))
        deck.discard(card_tuple)
        board_cards.append(Card(rank=card_tuple[0], suit=card_tuple[1]))

    opponent_hands = []
    for _ in range(num_opponents):
        if len(deck) < 2:
            return (0, 0, 0)
        opp_card_tuples = random.sample(list(deck), 2)
        for card_tuple in opp_card_tuples:
            deck.discard(card_tuple)
        opponent_hands.append([Card(rank=t[0], suit=t[1]) for t in opp_card_tuples])

    our_hand_value = evaluate_hand_base(hole_cards, board_cards)
    opponent_values = [evaluate_hand_base(hand, board_cards) for hand in opponent_hands]

    max_opponent = max(opponent_values) if opponent_values else 0

    our_hand_type = get_hand_type(our_hand_value)
    max_opponent_hand_type = get_hand_type(max_opponent)

    if our_hand_value > max_opponent:
        return (1, our_hand_type, max_opponent_hand_type)
    elif our_hand_value == max_opponent:
        return (0, our_hand_type, max_opponent_hand_type)
    else:
        return (-1, our_hand_type, max_opponent_hand_type)


def _create_deck() -> set:
    deck = set()
    for rank in range(2, 15):
        for suit in range(4):
            deck.add((rank, suit))
    return deck


def _evaluate_five_cards(cards: list) -> int:
    ranks = [card[0] if isinstance(card, tuple) else card.rank for card in cards]
    suits = [card[1] if isinstance(card, tuple) else card.suit for card in cards]

    rank_counts = {}
    for rank in ranks:
        rank_counts[rank] = rank_counts.get(rank, 0) + 1

    counts = sorted(rank_counts.values(), reverse=True)
    is_flush = len(set(suits)) == 1
    is_straight = _is_straight(ranks)

    if is_straight and is_flush:
        if max(ranks) == 14 and min(ranks) == 10:
            return 9000000
        return 8000000 + max(ranks)

    if counts == [4, 1]:
        return 7000000 + max(rank_counts, key=rank_counts.get)

    if counts == [3, 2]:
        return 6000000 + max(rank_counts, key=rank_counts.get)

    if is_flush:
        return 5000000 + max(ranks)

    if is_straight:
        return 4000000 + max(ranks)

    if counts == [3, 1, 1]:
        return 3000000 + max(rank_counts, key=rank_counts.get)

    if counts == [2, 2, 1]:
        pairs = [r for r, c in rank_counts.items() if c == 2]
        return 2000000 + max(pairs) * 100 + min(pairs)

    if counts == [2, 1, 1, 1]:
        pair_rank = max(rank_counts, key=rank_counts.get)
        kickers = sorted([r for r in ranks if r != pair_rank], reverse=True)
        return 1000000 + pair_rank * 10000 + kickers[0] * 100 + kickers[1]

    high_cards = sorted(ranks, reverse=True)
    return (
        high_cards[0] * 10000
        + high_cards[1] * 1000
        + high_cards[2] * 100
        + high_cards[3] * 10
        + high_cards[4]
    )


def _is_straight(ranks: list[int]) -> bool:
    sorted_ranks = sorted(set(ranks))
    if len(sorted_ranks) < 5:
        return False

    for i in range(len(sorted_ranks) - 4):
        if sorted_ranks[i + 4] - sorted_ranks[i] == 4:
            return True

    if 14 in sorted_ranks:
        low_ace = [1 if r == 14 else r for r in sorted_ranks]
        low_ace_sorted = sorted(set(low_ace))
        for i in range(len(low_ace_sorted) - 4):
            if low_ace_sorted[i + 4] - low_ace_sorted[i] == 4:
                return True

    return False


def get_hand_type(hand_value: int) -> int:
    """
    Classify a hand value into one of 10 hand types.

    Returns:
        0: High Card
        1: One Pair
        2: Two Pair
        3: Three of a Kind
        4: Straight
        5: Flush
        6: Full House
        7: Four of a Kind
        8: Straight Flush
        9: Royal Flush
    """
    if hand_value >= 9000000:
        return 9  # Royal Flush
    elif hand_value >= 8000000:
        return 8  # Straight Flush
    elif hand_value >= 7000000:
        return 7  # Four of a Kind
    elif hand_value >= 6000000:
        return 6  # Full House
    elif hand_value >= 5000000:
        return 5  # Flush
    elif hand_value >= 4000000:
        return 4  # Straight
    elif hand_value >= 3000000:
        return 3  # Three of a Kind
    elif hand_value >= 2000000:
        return 2  # Two Pair
    elif hand_value >= 1000000:
        return 1  # One Pair
    else:
        return 0  # High Card
