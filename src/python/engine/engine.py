from typing import Callable, Dict, Optional

from ..models.card import Card
from ..models.job import EquityResult, JobRequest
from ..utils.shared_memory import SharedMemoryWriter


class EquityEngine:
    def __init__(
        self,
        evaluate_hand: Callable[[list[Card], list[Card]], int],
        simulate_hand: Callable[[list[Card], list[Card], int], int],
        mode: str,
        num_workers: Optional[int] = None,
        job_id: Optional[str] = None,
    ):
        self.evaluate_hand = evaluate_hand
        self.simulate_hand = simulate_hand
        self.mode = mode
        self.num_workers = num_workers
        self._progress_callback: Optional[Callable[[float, Dict[str, float]], None]] = None

        self.shm_writer: Optional[SharedMemoryWriter] = None
        if job_id:
            self.shm_writer = SharedMemoryWriter(job_id)
            if not self.shm_writer.create():
                import logging

                logging.warning(f"Failed to create shared memory for job {job_id}")
                self.shm_writer = None
        self.simulations_processed = 0
        self.update_frequency = 1000
        self.last_update_count = 0

    def set_progress_callback(self, callback: Callable[[float, Dict[str, float]], None]):
        self._progress_callback = callback

    def _report_progress(self, progress: float, results: Dict[str, float]):
        if self._progress_callback:
            self._progress_callback(progress, results)

    def calculate_range_equity(self, request: JobRequest) -> Dict[str, EquityResult]:
        results: Dict[str, EquityResult] = {}
        self.simulations_processed = 0
        self.last_update_count = 0

        hand_names = list(request.range_spec.keys())
        total_hands = len(hand_names)
        simulations_per_hand = request.num_simulations // total_hands

        try:
            for idx, hand_name in enumerate(hand_names):
                hole_cards = request.range_spec[hand_name]
                equity_result = self._calculate_hand_equity(
                    hole_cards,
                    request.board,
                    request.num_opponents,
                    simulations_per_hand,
                    hand_name,
                    results,
                )
                results[hand_name] = equity_result
                # Note: simulations_processed is now updated inside _calculate_hand_equity

                # Final update for this hand with complete results
                if self.shm_writer:
                    # Ensure hands_processed reflects the total for this hand
                    expected_total = (idx + 1) * simulations_per_hand
                    if self.simulations_processed < expected_total:
                        self.simulations_processed = expected_total
                    self.shm_writer.update_hands(self.simulations_processed)
                    self.shm_writer.update_equity_results(results)

                progress = (idx + 1) / total_hands
                current_results = {name: result.equity for name, result in results.items()}
                self._report_progress(progress, current_results)

            if self.shm_writer:
                self.shm_writer.update_hands(self.simulations_processed)
                self.shm_writer.set_status(1)
                self.shm_writer.close()

        except Exception:
            if self.shm_writer:
                self.shm_writer.set_status(2)
                self.shm_writer.close()
            raise

        return results

    def _calculate_hand_equity(
        self,
        hole_cards: list[Card],
        board: list[Card],
        num_opponents: int,
        num_simulations: int,
        hand_name: str = "",
        results: Optional[Dict[str, EquityResult]] = None,
    ) -> EquityResult:
        wins = 0
        ties = 0
        losses = 0
        win_method_matrix = [[0] * 10 for _ in range(10)]
        update_interval = 1000  # Update shared memory every 1000 simulations

        for sim_num in range(num_simulations):
            outcome, our_type, opp_type = self.simulate_hand(hole_cards, board, num_opponents)
            if outcome == 1:
                wins += 1
                win_method_matrix[our_type][opp_type] += 1
            elif outcome == 0:
                ties += 1
            else:
                losses += 1

            # Periodically update shared memory with partial results
            if self.shm_writer and results is not None and hand_name and (sim_num + 1) % update_interval == 0:
                total = wins + ties + losses
                equity = (wins + ties * 0.5) / total if total > 0 else 0.0

                # Update hands processed counter
                self.simulations_processed += update_interval
                if (self.simulations_processed - self.last_update_count) >= self.update_frequency:
                    self.shm_writer.update_hands(self.simulations_processed)
                    self.last_update_count = self.simulations_processed

                # Update partial equity results
                partial_result = EquityResult(
                    hand_name=hand_name,
                    equity=equity,
                    wins=wins,
                    ties=ties,
                    losses=losses,
                    total_simulations=sim_num + 1,
                    win_method_matrix=win_method_matrix,
                )
                results[hand_name] = partial_result
                self.shm_writer.update_equity_results(results)

        total = wins + ties + losses
        equity = (wins + ties * 0.5) / total if total > 0 else 0.0

        return EquityResult(
            hand_name=hand_name,
            equity=equity,
            wins=wins,
            ties=ties,
            losses=losses,
            total_simulations=num_simulations,
            win_method_matrix=win_method_matrix,
        )

    def get_mode(self) -> str:
        return self.mode

    def get_num_workers(self) -> Optional[int]:
        return self.num_workers
