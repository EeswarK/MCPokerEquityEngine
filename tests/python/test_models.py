import pytest

from src.python.models.card import Card
from src.python.models.job import EquityResult, JobRequest, PerformanceMetrics


def test_card_creation():
    card = Card(rank=14, suit=0)
    assert card.rank == 14
    assert card.suit == 0


def test_card_validation_rank():
    with pytest.raises(ValueError):
        Card(rank=15, suit=0)
    with pytest.raises(ValueError):
        Card(rank=1, suit=0)


def test_card_validation_suit():
    with pytest.raises(ValueError):
        Card(rank=14, suit=4)
    with pytest.raises(ValueError):
        Card(rank=14, suit=-1)


def test_card_serialization():
    card = Card(rank=14, suit=0)
    data = card.to_dict()
    assert data == {"rank": 14, "suit": 0}
    restored = Card.from_dict(data)
    assert restored.rank == 14
    assert restored.suit == 0


def test_job_request_validation():
    range_spec = {"AA": [Card(14, 0), Card(14, 1)]}
    request = JobRequest(range_spec=range_spec)
    assert request.num_opponents == 1
    assert request.num_simulations == 100000


def test_job_request_validation_opponents():
    range_spec = {"AA": [Card(14, 0), Card(14, 1)]}
    with pytest.raises(ValueError):
        JobRequest(range_spec=range_spec, num_opponents=0)
    with pytest.raises(ValueError):
        JobRequest(range_spec=range_spec, num_opponents=10)


def test_job_request_validation_simulations():
    range_spec = {"AA": [Card(14, 0), Card(14, 1)]}
    with pytest.raises(ValueError):
        JobRequest(range_spec=range_spec, num_simulations=999)
    with pytest.raises(ValueError):
        JobRequest(range_spec=range_spec, num_simulations=10000001)


def test_performance_metrics():
    metrics = PerformanceMetrics(
        mode="base_python",
        duration_seconds=1.5,
        simulations_per_second=10000.0,
    )
    data = metrics.to_dict()
    assert data["mode"] == "base_python"
    assert data["duration_seconds"] == 1.5
    assert data["simulations_per_second"] == 10000.0


def test_equity_result():
    result = EquityResult(
        hand_name="AA",
        equity=0.85,
        wins=850,
        ties=0,
        losses=150,
        total_simulations=1000,
    )
    data = result.to_dict()
    assert data["hand_name"] == "AA"
    assert data["equity"] == 0.85
    assert data["wins"] == 850
