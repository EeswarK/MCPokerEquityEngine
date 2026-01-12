import asyncio
import json
import logging
import uuid

from fastapi import FastAPI, HTTPException, Request, WebSocket, WebSocketDisconnect, status
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
from .websocket import ConnectionManager

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
connection_manager = ConnectionManager()


@app.on_event("startup")
async def startup_event():
    await connection_manager.start_worker()


@app.post("/api/jobs", response_model=CreateJobResponse, status_code=status.HTTP_201_CREATED)
async def create_job(request: CreateJobRequest):
    job_id = str(uuid.uuid4())
    job_state = job_manager.create_job(job_id)

    internal_request = create_job_request_to_internal(request)

    asyncio.create_task(execute_job_async(job_id, internal_request, job_state, connection_manager))

    return CreateJobResponse(
        job_id=job_id,
        status=job_state.status.value,
        created_at=job_state.created_at.isoformat(),
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


@app.websocket("/ws/{job_id}")
async def websocket_endpoint(websocket: WebSocket, job_id: str):
    job_state = job_manager.get_job(job_id)
    if job_state is None:
        await websocket.close(code=status.WS_1008_POLICY_VIOLATION)
        return

    await connection_manager.connect(websocket, job_id)

    try:
        while True:
            data = await websocket.receive_text()
            try:
                message = json.loads(data)
                if message.get("type") == "ping":
                    from datetime import datetime

                    await websocket.send_text(
                        json.dumps(
                            {"type": "pong", "data": {"timestamp": datetime.utcnow().isoformat()}}
                        )
                    )
            except json.JSONDecodeError:
                pass
    except WebSocketDisconnect:
        connection_manager.disconnect(websocket, job_id)
