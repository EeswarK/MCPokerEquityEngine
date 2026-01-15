from src.python.engine.strategies.base import evaluate_hand_base, simulate_hand_base
from src.python.models.card import Card


def test_evaluate_hand_base():
    hole_cards = [Card(rank=14, suit=0), Card(rank=14, suit=1)]
    board = [Card(rank=13, suit=0), Card(rank=12, suit=0), Card(rank=11, suit=0)]
    value = evaluate_hand_base(hole_cards, board)
    assert value > 0


def test_simulate_hand_base():
    hole_cards = [Card(rank=14, suit=0), Card(rank=14, suit=1)]
    board = []
    outcome, our_type, opp_type, opp_hand = simulate_hand_base(hole_cards, board, 1)
    assert outcome in [-1, 0, 1]
    assert 0 <= our_type <= 9
    assert 0 <= opp_type <= 9
    assert isinstance(opp_hand, str)
