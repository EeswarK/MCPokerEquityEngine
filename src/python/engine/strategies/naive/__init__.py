"""
Naive brute-force hand evaluator.

Uses nested loops to evaluate all possible 5-card combinations
without optimization. Slow but straightforward.
"""
from .evaluator import evaluate_hand_naive, simulate_hand_naive

__all__ = ["evaluate_hand_naive", "simulate_hand_naive"]
