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
                # Store overall equity result (for API response) but don't add to results dict
                # because results dict now contains opponent-specific equity data
                # results[hand_name] = equity_result  # REMOVED - would overwrite opponent data
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
        # Track equity by opponent hand type
        opponent_stats: Dict[str, Dict[str, int]] = {}  # opponent_hand -> {wins, ties, losses, total}
        win_method_matrix = [[0] * 10 for _ in range(10)]  # [our_type][opp_type] when we win
        loss_method_matrix = [[0] * 10 for _ in range(10)]  # [opp_type][our_type] when we lose
        update_interval = 1000  # Update shared memory every 1000 simulations

        for sim_num in range(num_simulations):
            outcome, our_type, opp_type, opp_hand_classification = self.simulate_hand(hole_cards, board, num_opponents)

            # Initialize stats for this opponent hand type if not seen before
            if opp_hand_classification not in opponent_stats:
                opponent_stats[opp_hand_classification] = {"wins": 0, "ties": 0, "losses": 0, "total": 0}

            stats = opponent_stats[opp_hand_classification]
            stats["total"] += 1

            if outcome == 1:
                stats["wins"] += 1
                win_method_matrix[our_type][opp_type] += 1
            elif outcome == 0:
                stats["ties"] += 1
            else:  # outcome == -1 (loss)
                stats["losses"] += 1
                loss_method_matrix[opp_type][our_type] += 1  # Note: reversed indices

            # Periodically update shared memory with partial results
            if self.shm_writer and results is not None and (sim_num + 1) % update_interval == 0:
                # Update hands processed counter
                self.simulations_processed += update_interval
                if (self.simulations_processed - self.last_update_count) >= self.update_frequency:
                    self.shm_writer.update_hands(self.simulations_processed)
                    self.last_update_count = self.simulations_processed

                # Create EquityResult for each opponent hand type
                for opp_hand, stats_dict in opponent_stats.items():
                    total = stats_dict["total"]
                    equity = (stats_dict["wins"] + stats_dict["ties"] * 0.5) / total if total > 0 else 0.0

                    results[opp_hand] = EquityResult(
                        hand_name=opp_hand,
                        equity=equity,
                        wins=stats_dict["wins"],
                        ties=stats_dict["ties"],
                        losses=stats_dict["losses"],
                        total_simulations=total,
                        win_method_matrix=win_method_matrix,
                        loss_method_matrix=loss_method_matrix,
                    )

                self.shm_writer.update_equity_results(results)

        # Create final EquityResults for each opponent hand type
        final_results = {}
        for opp_hand, stats_dict in opponent_stats.items():
            total = stats_dict["total"]
            equity = (stats_dict["wins"] + stats_dict["ties"] * 0.5) / total if total > 0 else 0.0

            final_results[opp_hand] = EquityResult(
                hand_name=opp_hand,
                equity=equity,
                wins=stats_dict["wins"],
                ties=stats_dict["ties"],
                losses=stats_dict["losses"],
                total_simulations=total,
                win_method_matrix=win_method_matrix,
                loss_method_matrix=loss_method_matrix,
            )

        # Debug logging
        import logging
        logging.info(f"[ENGINE] Opponent stats collected: {len(opponent_stats)} unique hands")
        logging.info(f"[ENGINE] Sample opponent hands: {list(opponent_stats.keys())[:20]}")

        # Update results dict with all opponent-specific equities
        if results is not None:
            results.update(final_results)
            logging.info(f"[ENGINE] Results dict now has {len(results)} entries")

        # Return overall equity as a summary (for backward compatibility)
        total_sims = sum(stats["total"] for stats in opponent_stats.values())
        total_wins = sum(stats["wins"] for stats in opponent_stats.values())
        total_ties = sum(stats["ties"] for stats in opponent_stats.values())
        total_losses = sum(stats["losses"] for stats in opponent_stats.values())
        overall_equity = (total_wins + total_ties * 0.5) / total_sims if total_sims > 0 else 0.0

        return EquityResult(
            hand_name=hand_name,
            equity=overall_equity,
            wins=total_wins,
            ties=total_ties,
            losses=total_losses,
            total_simulations=total_sims,
            win_method_matrix=win_method_matrix,
            loss_method_matrix=loss_method_matrix,
        )

    def get_mode(self) -> str:
        return self.mode

    def get_num_workers(self) -> Optional[int]:
        return self.num_workers
