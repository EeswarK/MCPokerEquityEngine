from typing import Dict, Callable, Optional
from ..models.card import Card
from ..models.job import JobRequest, PerformanceMetrics, EquityResult
from ..utils.metrics import MetricsCollector


class EquityEngine:
    def __init__(
        self,
        evaluate_hand: Callable[[list[Card], list[Card]], int],
        simulate_hand: Callable[[list[Card], list[Card], int], int],
        mode: str,
        num_workers: Optional[int] = None,
    ):
        self.evaluate_hand = evaluate_hand
        self.simulate_hand = simulate_hand
        self.mode = mode
        self.num_workers = num_workers
        self.metrics_collector = MetricsCollector()
        self._progress_callback: Optional[Callable[[float, Dict[str, float]], None]] = None

    def set_progress_callback(
        self, callback: Callable[[float, Dict[str, float]], None]
    ):
        self._progress_callback = callback

    def _report_progress(self, progress: float, results: Dict[str, float]):
        if self._progress_callback:
            self._progress_callback(progress, results)

    def calculate_range_equity(
        self, request: JobRequest
    ) -> Dict[str, EquityResult]:
        self.metrics_collector.start()
        results: Dict[str, EquityResult] = {}

        hand_names = list(request.range_spec.keys())
        total_hands = len(hand_names)
        simulations_per_hand = request.num_simulations // total_hands

        for idx, hand_name in enumerate(hand_names):
            hole_cards = request.range_spec[hand_name]
            equity_result = self._calculate_hand_equity(
                hole_cards,
                request.board,
                request.num_opponents,
                simulations_per_hand,
            )
            equity_result.hand_name = hand_name
            results[hand_name] = equity_result

            progress = (idx + 1) / total_hands
            current_results = {
                name: result.equity for name, result in results.items()
            }
            self._report_progress(progress, current_results)

        return results

    def _calculate_hand_equity(
        self,
        hole_cards: list[Card],
        board: list[Card],
        num_opponents: int,
        num_simulations: int,
    ) -> EquityResult:
        wins = 0
        ties = 0
        losses = 0

        for _ in range(num_simulations):
            outcome = self.simulate_hand(hole_cards, board, num_opponents)
            if outcome == 1:
                wins += 1
            elif outcome == 0:
                ties += 1
            else:
                losses += 1

        self.metrics_collector.record_simulations(num_simulations)

        total = wins + ties + losses
        equity = (wins + ties * 0.5) / total if total > 0 else 0.0

        return EquityResult(
            hand_name="",
            equity=equity,
            wins=wins,
            ties=ties,
            losses=losses,
            total_simulations=num_simulations,
        )

    def get_metrics(self) -> PerformanceMetrics:
        metrics_dict = self.metrics_collector.get_metrics(
            self.mode, self.num_workers
        )
        return PerformanceMetrics(**metrics_dict)

    def get_mode(self) -> str:
        return self.mode

    def get_num_workers(self) -> Optional[int]:
        return self.num_workers
