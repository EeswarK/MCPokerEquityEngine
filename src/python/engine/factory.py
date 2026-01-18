from typing import Optional

from .engine import EquityEngine
from .strategies.naive import evaluate_hand_naive, simulate_hand_naive
from .strategies.senzee import evaluate_hand_senzee, simulate_hand_senzee


def create_engine(
    mode: str, num_workers: Optional[int] = None, job_id: Optional[str] = None
) -> EquityEngine:
    if mode == "base_python":
        return EquityEngine(
            evaluate_hand=evaluate_hand_naive,
            simulate_hand=simulate_hand_naive,
            mode="base_python",
            num_workers=num_workers,
            job_id=job_id,
        )
    elif mode == "senzee":
        # Fast bitwise evaluator
        return EquityEngine(
            evaluate_hand=evaluate_hand_senzee,
            simulate_hand=simulate_hand_senzee,
            mode="senzee",
            num_workers=num_workers,
            job_id=job_id,
        )
    elif mode == "numpy":
        raise NotImplementedError("NumPy engine not yet implemented")
    elif mode == "multiprocessing":
        raise NotImplementedError("Multiprocessing engine not yet implemented")
    else:
        raise ValueError(f"Unknown engine mode: {mode}")