from dataclasses import dataclass
from typing import Dict, List, Optional

from .card import Card, HandRange


@dataclass
class JobRequest:
    range_spec: HandRange
    board: List[Card] = None
    num_opponents: int = 1
    num_simulations: int = 100000
    mode: str = "base_python"
    num_workers: Optional[int] = None

    def __post_init__(self):
        if self.board is None:
            self.board = []
        if not (1 <= self.num_opponents <= 9):
            raise ValueError(f"num_opponents must be 1-9, got {self.num_opponents}")
        if not (1000 <= self.num_simulations <= 10000000):
            raise ValueError(f"num_simulations must be 1000-10000000, got {self.num_simulations}")


@dataclass
class PerformanceMetrics:
    mode: str
    duration_seconds: float
    simulations_per_second: float
    cpu_percent: Optional[float] = None
    memory_mb: Optional[float] = None
    num_workers: Optional[int] = None

    def to_dict(self) -> Dict:
        result = {
            "mode": self.mode,
            "duration_seconds": self.duration_seconds,
            "simulations_per_second": self.simulations_per_second,
        }
        if self.cpu_percent is not None:
            result["cpu_percent"] = self.cpu_percent
        if self.memory_mb is not None:
            result["memory_mb"] = self.memory_mb
        if self.num_workers is not None:
            result["num_workers"] = self.num_workers
        return result


@dataclass
class EquityResult:
    hand_name: str
    equity: float
    wins: int = 0
    ties: int = 0
    losses: int = 0
    total_simulations: int = 0

    def to_dict(self) -> Dict:
        return {
            "hand_name": self.hand_name,
            "equity": self.equity,
            "wins": self.wins,
            "ties": self.ties,
            "losses": self.losses,
            "total_simulations": self.total_simulations,
        }
