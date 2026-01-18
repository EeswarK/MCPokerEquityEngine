"""
Senzee hand evaluator (Cactus Kev's algorithm).

Based on Treys library - Python 3 port of Deuces.
Uses bitwise operations and Paul Senzee's perfect hash lookup tables
for O(1) hand evaluation.

References:
- Cactus Kev: http://suffe.cool/poker/evaluator.html
- Treys: https://github.com/ihendley/treys
"""
from .evaluator import evaluate_hand_senzee, simulate_hand_senzee

__all__ = ["evaluate_hand_senzee", "simulate_hand_senzee"]
