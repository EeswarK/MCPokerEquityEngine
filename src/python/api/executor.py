import asyncio
import os
import subprocess
from typing import Dict, Optional

from ..models.job import JobRequest
from .job_manager import JobState


def execute_job_sync(
    job_id: str,
    request: JobRequest,
    job_state: JobState,
):
    telemetry_process: Optional[subprocess.Popen] = None

    try:
        job_state.start()

        from ..engine import create_engine

        # Use new algorithm/optimizations if provided, otherwise fall back to legacy mode
        engine = create_engine(
            mode=request.mode,
            algorithm=request.algorithm,
            optimizations=request.optimizations,
            num_workers=request.num_workers,
            job_id=job_id
        )

        current_pid = os.getpid()
        telemetry_port = int(os.getenv("TELEMETRY_PORT", "8001"))

        telemetry_binary = os.getenv(
            "TELEMETRY_COLLECTOR_BINARY",
            os.path.join(
                os.path.dirname(__file__),
                "../../cpp/telemetry_collector/build/telemetry_collector",
            ),
        )

        if os.path.exists(telemetry_binary) and os.access(telemetry_binary, os.X_OK):
            try:
                telemetry_process = subprocess.Popen(
                    [telemetry_binary, job_id, str(current_pid), str(telemetry_port)],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    stdin=subprocess.DEVNULL,
                )
                import logging

                logger = logging.getLogger("uvicorn.error")
                logger.info(
                    f"Spawned telemetry collector for job {job_id} (PID: {telemetry_process.pid})"
                )
            except Exception as e:
                import logging

                logger = logging.getLogger("uvicorn.error")
                logger.warning(f"Failed to spawn telemetry collector: {e}")

        def progress_callback(progress: float, current_results: Dict[str, float]):
            job_state.update_progress(progress, current_results)

        engine.set_progress_callback(progress_callback)

        results = engine.calculate_range_equity(request)
        job_state.complete(results, None)

    except Exception as e:
        error_msg = str(e)
        job_state.fail(error_msg)

    finally:
        if telemetry_process:
            try:
                telemetry_process.terminate()
                try:
                    telemetry_process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    telemetry_process.kill()
                    telemetry_process.wait()
            except Exception as e:
                import logging

                logger = logging.getLogger("uvicorn.error")
                logger.warning(f"Error killing telemetry process: {e}")


async def execute_job_async(
    job_id: str,
    request: JobRequest,
    job_state: JobState,
):
    loop = asyncio.get_event_loop()
    await loop.run_in_executor(
        None,
        execute_job_sync,
        job_id,
        request,
        job_state,
    )
