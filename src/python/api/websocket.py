import asyncio
from queue import Queue
from typing import Dict, Optional, Set

from fastapi import WebSocket


class ConnectionManager:
    def __init__(self):
        self.active_connections: Dict[str, Set[WebSocket]] = {}
        self._lock = asyncio.Lock()
        self._broadcast_queue: Queue = Queue()
        self._worker_task: Optional[asyncio.Task] = None

    async def start_worker(self):
        if self._worker_task is None:
            self._worker_task = asyncio.create_task(self._broadcast_worker())

    async def _broadcast_worker(self):
        while True:
            try:
                item = await asyncio.to_thread(self._broadcast_queue.get, True, 1.0)
                if len(item) == 3:
                    job_id, message, is_binary = item
                    await self._send_to_job(job_id, message, is_binary)
                else:
                    job_id, message = item
                    await self._send_to_job(job_id, message, False)
                self._broadcast_queue.task_done()
            except:
                await asyncio.sleep(0.1)

    async def connect(self, websocket: WebSocket, job_id: str):
        await websocket.accept()
        async with self._lock:
            if job_id not in self.active_connections:
                self.active_connections[job_id] = set()
            self.active_connections[job_id].add(websocket)

    def disconnect(self, websocket: WebSocket, job_id: str):
        async def _disconnect():
            async with self._lock:
                if job_id in self.active_connections:
                    self.active_connections[job_id].discard(websocket)
                    if not self.active_connections[job_id]:
                        del self.active_connections[job_id]

        asyncio.create_task(_disconnect())

    def broadcast(self, job_id: str, message: str):
        self._broadcast_queue.put((job_id, message))

    def broadcast_binary(self, job_id: str, message: bytes):
        self._broadcast_queue.put((job_id, message, True))

    async def _send_to_job(self, job_id: str, message, is_binary: bool = False):
        async with self._lock:
            connections = self.active_connections.get(job_id, set()).copy()

        disconnected = set()
        for connection in connections:
            try:
                if is_binary:
                    await connection.send_bytes(message)
                else:
                    await connection.send_text(message)
            except Exception:
                disconnected.add(connection)

        if disconnected:
            async with self._lock:
                if job_id in self.active_connections:
                    self.active_connections[job_id] -= disconnected
