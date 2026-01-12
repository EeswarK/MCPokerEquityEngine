import threading
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from typing import Dict, Optional

from ..models.job import EquityResult, PerformanceMetrics


class JobStatus(str, Enum):
    PENDING = "pending"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"


@dataclass
class JobState:
    job_id: str
    status: JobStatus = JobStatus.PENDING
    progress: float = 0.0
    created_at: datetime = field(default_factory=datetime.utcnow)
    completed_at: Optional[datetime] = None
    error: Optional[str] = None
    results: Dict[str, EquityResult] = field(default_factory=dict)
    metrics: Optional[PerformanceMetrics] = None
    current_results: Dict[str, float] = field(default_factory=dict)
    _lock: threading.Lock = field(default_factory=threading.Lock)

    def update_progress(self, progress: float, current_results: Dict[str, float]):
        with self._lock:
            self.progress = progress
            self.current_results = current_results.copy()

    def complete(
        self, results: Dict[str, EquityResult], metrics: Optional[PerformanceMetrics] = None
    ):
        with self._lock:
            self.status = JobStatus.COMPLETED
            self.results = results
            self.metrics = metrics
            self.completed_at = datetime.utcnow()
            self.progress = 1.0

    def fail(self, error: str):
        with self._lock:
            self.status = JobStatus.FAILED
            self.error = error
            self.completed_at = datetime.utcnow()

    def start(self):
        with self._lock:
            self.status = JobStatus.RUNNING


class JobManager:
    def __init__(self):
        self._jobs: Dict[str, JobState] = {}
        self._lock = threading.Lock()

    def create_job(self, job_id: str) -> JobState:
        with self._lock:
            job = JobState(job_id=job_id)
            self._jobs[job_id] = job
            return job

    def get_job(self, job_id: str) -> Optional[JobState]:
        with self._lock:
            return self._jobs.get(job_id)

    def delete_job(self, job_id: str):
        with self._lock:
            self._jobs.pop(job_id, None)
