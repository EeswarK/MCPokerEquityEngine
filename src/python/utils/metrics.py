import time
import psutil
from typing import Dict, Optional


class MetricsCollector:
    def __init__(self):
        self.process = psutil.Process()
        self.start_time: Optional[float] = None
        self.simulations_run: int = 0

    def start(self):
        self.start_time = time.perf_counter()
        self.simulations_run = 0

    def record_simulations(self, count: int):
        self.simulations_run += count

    def get_metrics(
        self, mode: str, num_workers: Optional[int] = None
    ) -> Dict:
        if self.start_time is None:
            return {
                "mode": mode,
                "duration_seconds": 0.0,
                "simulations_per_second": 0.0,
            }
        duration = time.perf_counter() - self.start_time
        metrics = {
            "mode": mode,
            "duration_seconds": duration,
            "simulations_per_second": (
                self.simulations_run / duration if duration > 0 else 0.0
            ),
        }
        try:
            metrics["cpu_percent"] = self.process.cpu_percent(interval=0.1)
            metrics["memory_mb"] = self.process.memory_info().rss / 1024 / 1024
        except Exception:
            pass
        if num_workers is not None:
            metrics["num_workers"] = num_workers
        return metrics
