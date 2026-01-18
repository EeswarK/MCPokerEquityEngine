import asyncio
import json
import logging
import os
import uuid

from fastapi import FastAPI, HTTPException, Request, status
from fastapi.middleware.cors import CORSMiddleware
from starlette.middleware.base import BaseHTTPMiddleware

from .executor import execute_job_async
from .job_manager import JobManager
from .models import (
    CreateJobRequest,
    CreateJobResponse,
    JobStatusResponse,
    create_job_request_to_internal,
)

logger = logging.getLogger("uvicorn.error")


class RequestLoggingMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next):
        try:
            client = request.client.host if request.client else "unknown"
            method = request.method
            url = str(request.url)
            headers = dict(request.headers)

            logger.warning(f"Request: {method} {url} | Client: {client} | Headers: {headers}")
        except Exception as e:
            logger.error(f"Failed to log request: {e}")

        return await call_next(request)


app = FastAPI(
    title="Poker Equity Engine API",
    version="0.1.0",
    description="High-performance poker equity calculation engine",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.add_middleware(RequestLoggingMiddleware)

job_manager = JobManager()


@app.post("/api/jobs", response_model=CreateJobResponse, status_code=status.HTTP_201_CREATED)
async def create_job(request: CreateJobRequest):
    job_id = str(uuid.uuid4())
    job_state = job_manager.create_job(job_id)

    internal_request = create_job_request_to_internal(request)

    host = os.getenv("TELEMETRY_HOST", "localhost")
    protocol = os.getenv("TELEMETRY_WS_PROTOCOL", "ws")

    if protocol == "wss":
        telemetry_ws_url = f"{protocol}://{host}/telemetry/{job_id}"
    else:
        telemetry_port = int(os.getenv("TELEMETRY_PORT", "8001"))
        telemetry_ws_url = f"{protocol}://{host}:{telemetry_port}/telemetry/{job_id}"

    asyncio.create_task(execute_job_async(job_id, internal_request, job_state))

    return CreateJobResponse(
        job_id=job_id,
        status=job_state.status.value,
        created_at=job_state.created_at.isoformat(),
        telemetry_ws_url=telemetry_ws_url,
    )


@app.get("/api/jobs/{job_id}/status", response_model=JobStatusResponse)
async def get_job_status(job_id: str):
    job_state = job_manager.get_job(job_id)
    if job_state is None:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Job not found",
        )

    response = JobStatusResponse(
        job_id=job_state.job_id,
        status=job_state.status.value,
        progress=job_state.progress,
        created_at=job_state.created_at.isoformat(),
        completed_at=job_state.completed_at.isoformat() if job_state.completed_at else None,
        error=job_state.error,
    )
    return response


@app.get("/health")
async def health_check():
    return {"status": "healthy", "version": "0.1.0"}
