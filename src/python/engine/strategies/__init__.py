"""
Hand evaluation strategies for poker equity calculation.

Available strategies:
- naive: Brute-force evaluation with nested loops
- senzee: Fast evaluation with bitwise operations and lookup tables
"""
from .naive import evaluate_hand_naive, simulate_hand_naive
from .senzee import evaluate_hand_senzee, simulate_hand_senzee

__all__ = [
    "evaluate_hand_naive",
    "simulate_hand_naive",
    "evaluate_hand_senzee",
    "simulate_hand_senzee",
]
