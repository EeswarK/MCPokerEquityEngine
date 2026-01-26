from typing import Optional, List

from .engine import EquityEngine
from .strategies.naive import evaluate_hand_naive, simulate_hand_naive
from .strategies.senzee import evaluate_hand_senzee, simulate_hand_senzee


def create_engine(
    mode: Optional[str] = None,
    algorithm: Optional[str] = None,
    optimizations: Optional[List[str]] = None,
    num_workers: Optional[int] = None,
    job_id: Optional[str] = None
) -> EquityEngine:
    """
    Create an equity engine based on algorithm selection.

    Args:
        mode: Legacy mode parameter (deprecated, maps to algorithm)
        algorithm: Core algorithm to use (NAIVE, CACTUS_KEV, etc.)
        optimizations: List of optimization flags (MULTITHREADING, SIMD, etc.)
        num_workers: Number of worker threads (for MULTITHREADING optimization)
        job_id: Job identifier for telemetry

    Returns:
        Configured EquityEngine instance
    """
    optimizations = optimizations or []

    # Determine algorithm from either new parameter or legacy mode
    if algorithm:
        algo = algorithm.upper()
    elif mode:
        # Map legacy mode to algorithm
        mode_to_algo = {
            "base_python": "NAIVE",
            "senzee": "CACTUS_KEV",
            "cpp_naive": "NAIVE",
        }
        algo = mode_to_algo.get(mode, "NAIVE")
    else:
        algo = "NAIVE"  # Default

    # Select evaluator based on algorithm
    if algo == "NAIVE":
        return EquityEngine(
            evaluate_hand=evaluate_hand_naive,
            simulate_hand=simulate_hand_naive,
            mode="naive",
            num_workers=num_workers if "MULTITHREADING" in optimizations else None,
            job_id=job_id,
        )
    elif algo == "CACTUS_KEV":
        return EquityEngine(
            evaluate_hand=evaluate_hand_senzee,
            simulate_hand=simulate_hand_senzee,
            mode="cactus_kev",
            num_workers=num_workers if "MULTITHREADING" in optimizations else None,
            job_id=job_id,
        )
    elif algo in ["PH_EVALUATOR", "TWO_PLUS_TWO", "OMP_EVAL"]:
        # These algorithms are only available in C++
        raise NotImplementedError(
            f"Algorithm '{algo}' is only available in C++ implementation. "
            f"Please use the C++ server on port 8080."
        )
    else:
        raise ValueError(f"Unknown algorithm: {algo}")