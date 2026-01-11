import asyncio
from typing import Dict
from datetime import datetime

from ..models.job import JobRequest, EquityResult, PerformanceMetrics
from .job_manager import JobState
from .websocket import ConnectionManager
from .models import TelemetryUpdate, PerformanceMetricsModel, WebSocketMessage, ErrorResponse


def execute_job_sync(
    job_id: str,
    request: JobRequest,
    job_state: JobState,
    connection_manager: ConnectionManager,
):
    try:
        job_state.start()

        from ..engine import create_engine
        engine = create_engine(request.mode, request.num_workers)

        def progress_callback(progress: float, current_results: Dict[str, float]):
            job_state.update_progress(progress, current_results)

            metrics = engine.get_metrics()
            metrics_model = PerformanceMetricsModel(
                mode=metrics.mode,
                duration_seconds=metrics.duration_seconds,
                simulations_per_second=metrics.simulations_per_second,
                cpu_percent=metrics.cpu_percent,
                memory_mb=metrics.memory_mb,
                num_workers=metrics.num_workers,
            )

            telemetry = TelemetryUpdate(
                job_id=job_id,
                status=job_state.status.value,
                progress=progress,
                current_results=current_results,
                metrics=metrics_model,
                timestamp=datetime.utcnow().isoformat(),
            )

            message = WebSocketMessage(
                type="telemetry",
                data=telemetry.model_dump(),
            )

            connection_manager.broadcast(job_id, message.model_dump_json())

        engine.set_progress_callback(progress_callback)

        results = engine.calculate_range_equity(request)
        metrics = engine.get_metrics()

        job_state.complete(results, metrics)

        final_telemetry = TelemetryUpdate(
            job_id=job_id,
            status="completed",
            progress=1.0,
            current_results={name: result.equity for name, result in results.items()},
            metrics=PerformanceMetricsModel(
                mode=metrics.mode,
                duration_seconds=metrics.duration_seconds,
                simulations_per_second=metrics.simulations_per_second,
                cpu_percent=metrics.cpu_percent,
                memory_mb=metrics.memory_mb,
                num_workers=metrics.num_workers,
            ),
            timestamp=datetime.utcnow().isoformat(),
        )

        final_message = WebSocketMessage(
            type="telemetry",
            data=final_telemetry.model_dump(),
        )
        connection_manager.broadcast(job_id, final_message.model_dump_json())

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
