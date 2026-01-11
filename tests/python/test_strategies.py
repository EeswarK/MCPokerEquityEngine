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
    outcome = simulate_hand_base(hole_cards, board, 1)
    assert outcome in [-1, 0, 1]
