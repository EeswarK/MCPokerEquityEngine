import pytest
from src.python.engine import EquityEngine, create_engine
from src.python.engine.strategies.base import evaluate_hand_base, simulate_hand_base
from src.python.models.card import Card
from src.python.models.job import JobRequest


def test_engine_instantiation():
    engine = EquityEngine(
        evaluate_hand=evaluate_hand_base,
        simulate_hand=simulate_hand_base,
        mode="base_python",
    )
    assert engine.get_mode() == "base_python"


def test_factory_creates_engine():
    engine = create_engine("base_python")
    assert isinstance(engine, EquityEngine)
    assert engine.get_mode() == "base_python"


def test_factory_raises_for_unknown_mode():
    with pytest.raises(ValueError):
        create_engine("invalid_mode")


def test_factory_raises_for_unimplemented_modes():
    with pytest.raises(NotImplementedError):
        create_engine("numpy")
    with pytest.raises(NotImplementedError):
        create_engine("multiprocessing")


def test_engine_calculates_equity():
    engine = create_engine("base_python")
    range_spec = {
        "AA": [Card(rank=14, suit=0), Card(rank=14, suit=1)],
    }
    request = JobRequest(
        range_spec=range_spec,
        num_opponents=1,
        num_simulations=1000,
    )
    results = engine.calculate_range_equity(request)
    assert len(results) > 0
    # The engine now returns opponent hand breakdown, so keys are opponent hands
    # We check that keys look like hand classifications (e.g. "72o", "AA")
    first_key = list(results.keys())[0]
    assert len(first_key) >= 2


def test_engine_progress_callback():
    progress_updates = []

    def callback(progress: float, results: dict):
        progress_updates.append((progress, results))

    engine = create_engine("base_python")
    engine.set_progress_callback(callback)
    range_spec = {
        "AA": [Card(rank=14, suit=0), Card(rank=14, suit=1)],
        "KK": [Card(rank=13, suit=0), Card(rank=13, suit=1)],
    }
    request = JobRequest(
        range_spec=range_spec,
        num_opponents=1,
        num_simulations=1000,
    )
    engine.calculate_range_equity(request)
    assert len(progress_updates) > 0
    assert progress_updates[-1][0] == 1.0


def test_engine_function_injection():
    def mock_evaluate(hole_cards, board_cards):
        return 1000000

    def mock_simulate(hole_cards, board, num_opponents):
        return (1, 1, 0, "AKo")  # outcome, our_type, opp_type, opp_hand

    engine = EquityEngine(
        evaluate_hand=mock_evaluate,
        simulate_hand=mock_simulate,
        mode="test",
    )
    assert engine.evaluate_hand == mock_evaluate
    assert engine.simulate_hand == mock_simulate
