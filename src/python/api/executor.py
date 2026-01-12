import asyncio
from typing import Dict
from datetime import datetime

from ..models.job import JobRequest, EquityResult
from .job_manager import JobState
from .websocket import ConnectionManager
from .models import WebSocketMessage, ErrorResponse


def execute_job_sync(
    job_id: str,
    request: JobRequest,
    job_state: JobState,
    connection_manager: ConnectionManager,
):
    try:
        job_state.start()

        from ..engine import create_engine
        engine = create_engine(request.mode, request.num_workers, job_id=job_id)

        def progress_callback(progress: float, current_results: Dict[str, float]):
            job_state.update_progress(progress, current_results)

        engine.set_progress_callback(progress_callback)

        results = engine.calculate_range_equity(request)
        job_state.complete(results, None)

    except Exception as e:
        error_msg = str(e)
        job_state.fail(error_msg)

        error_response = ErrorResponse(
            error=error_msg,
            code="SIMULATION_ERROR",
        )

        error_message = WebSocketMessage(
            type="error",
            data=error_response.model_dump(),
        )
        connection_manager.broadcast(job_id, error_message.model_dump_json())


async def execute_job_async(
    job_id: str,
    request: JobRequest,
    job_state: JobState,
    connection_manager: ConnectionManager,
):
    loop = asyncio.get_event_loop()
    await loop.run_in_executor(
        None,
        execute_job_sync,
        job_id,
        request,
        job_state,
        connection_manager,
    )
