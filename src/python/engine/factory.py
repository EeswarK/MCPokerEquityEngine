from typing import Optional

from .engine import EquityEngine
from .strategies.base import evaluate_hand_base, simulate_hand_base


def create_engine(
    mode: str, num_workers: Optional[int] = None, job_id: Optional[str] = None
) -> EquityEngine:
    if mode == "base_python":
        return EquityEngine(
            evaluate_hand=evaluate_hand_base,
            simulate_hand=simulate_hand_base,
            mode="base_python",
            num_workers=num_workers,
            job_id=job_id,
        )
    elif mode == "numpy":
        raise NotImplementedError("NumPy engine not yet implemented")
    elif mode == "multiprocessing":
        raise NotImplementedError("Multiprocessing engine not yet implemented")
    else:
        raise ValueError(f"Unknown engine mode: {mode}")
