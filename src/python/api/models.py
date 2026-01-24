from enum import Enum
from typing import Dict, List, Optional

from pydantic import BaseModel, Field, field_validator

from ..models.card import Card
from ..models.job import JobRequest as InternalJobRequest


class EngineMode(str, Enum):
    BASE_PYTHON = "base_python"
    SENZEE = "senzee"
    NUMPY = "numpy"
    MULTIPROCESSING = "multiprocessing"


class AlgorithmType(str, Enum):
    NAIVE = "naive"
    CACTUS_KEV = "cactus_kev"
    PH_EVALUATOR = "ph_evaluator"
    TWO_PLUS_TWO = "two_plus_two"
    OMP_EVAL = "omp_eval"


class OptimizationType(str, Enum):
    MULTITHREADING = "multithreading"
    SIMD = "simd"
    PERFECT_HASH = "perfect_hash"
    PREFETCHING = "prefetching"


class CardModel(BaseModel):
    rank: int = Field(ge=2, le=14)
    suit: int = Field(ge=0, le=3)


class CreateJobRequest(BaseModel):
    range_spec: Dict[str, List[CardModel]] = Field(min_length=1)
    board: List[CardModel] = Field(default_factory=list)
    num_opponents: int = Field(ge=1, le=9, default=1)
    num_simulations: int = Field(ge=1000, le=10000000, default=100000)
    mode: EngineMode = EngineMode.SENZEE
    algorithm: AlgorithmType = AlgorithmType.NAIVE
    optimizations: List[OptimizationType] = Field(default_factory=list)
    num_workers: Optional[int] = Field(None, gt=0)

    @field_validator("board")
    @classmethod
    def validate_board(cls, v: List[CardModel]) -> List[CardModel]:
        if len(v) not in [0, 3, 4, 5]:
            raise ValueError("board must have 0, 3, 4, or 5 cards")
        return v


class CreateJobResponse(BaseModel):
    job_id: str
    status: str
    created_at: str
    telemetry_ws_url: Optional[str] = None


class JobStatusResponse(BaseModel):
    job_id: str
    status: str
    progress: float = Field(ge=0.0, le=1.0)
    created_at: str
    completed_at: Optional[str] = None
    error: Optional[str] = None


class PerformanceMetricsModel(BaseModel):
    mode: str
    duration_seconds: float
    simulations_per_second: float
    cpu_percent: Optional[float] = None
    memory_mb: Optional[float] = None
    num_workers: Optional[int] = None


class TelemetryUpdate(BaseModel):
    job_id: str
    status: str
    progress: float
    current_results: Dict[str, float]
    metrics: PerformanceMetricsModel
    timestamp: str


class WebSocketMessage(BaseModel):
    type: str
    data: Dict


class ErrorResponse(BaseModel):
    error: str
    code: str
    details: Optional[Dict] = None


def card_model_to_internal(card: CardModel) -> Card:
    return Card(rank=card.rank, suit=card.suit)


def card_internal_to_dict(card: Card) -> Dict[str, int]:
    return {"rank": card.rank, "suit": card.suit}


def create_job_request_to_internal(api_request: CreateJobRequest) -> InternalJobRequest:
    from ..models.card import HandRange

    range_spec: HandRange = {
        hand_name: [card_model_to_internal(c) for c in cards]
        for hand_name, cards in api_request.range_spec.items()
    }

    board = [card_model_to_internal(c) for c in api_request.board]

    return InternalJobRequest(
        range_spec=range_spec,
        board=board,
        num_opponents=api_request.num_opponents,
        num_simulations=api_request.num_simulations,
        mode=api_request.mode.value,
        algorithm=api_request.algorithm.value,
        optimizations=[opt.value for opt in api_request.optimizations],
        num_workers=api_request.num_workers,
    )
