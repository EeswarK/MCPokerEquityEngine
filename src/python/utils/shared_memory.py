import ctypes
import mmap
import os
import time
from ctypes import Structure, c_uint8, c_uint32, c_uint64
from typing import Optional


class TelemetrySharedMemory(Structure):
    _fields_ = [
        ("seq", c_uint32),
        ("_padding1", c_uint32),
        ("hands_processed", c_uint64),
        ("last_update_ns", c_uint64),
        ("status", c_uint8),
        ("_reserved", c_uint8 * 39),
    ]


class SharedMemoryWriter:
    def __init__(self, job_id: str):
        self.job_id = job_id
        self.shm_name = f"/poker_telemetry_{job_id}"
        self.shm_fd: Optional[int] = None
        self.shm_mmap: Optional[mmap.mmap] = None
        self.data: Optional[TelemetrySharedMemory] = None

    def create(self) -> bool:
        """Create shared memory segment. Returns True on success."""
        try:
            shm_path = f"/dev/shm{self.shm_name}"
            self.shm_fd = os.open(shm_path, os.O_CREAT | os.O_RDWR | os.O_EXCL, 0o600)
            os.ftruncate(self.shm_fd, ctypes.sizeof(TelemetrySharedMemory))

            self.shm_mmap = mmap.mmap(
                self.shm_fd, ctypes.sizeof(TelemetrySharedMemory), access=mmap.ACCESS_WRITE
            )

            self.data = TelemetrySharedMemory.from_buffer(self.shm_mmap)
            self.data.seq = 0
            self.data.hands_processed = 0
            self.data.last_update_ns = int(time.time_ns())
            self.data.status = 0

            return True
        except Exception as e:
            import logging

            logging.warning(f"Failed to create shared memory {self.shm_name}: {e}")
            return False

    def update_hands(self, count: int):
        """Update hands_processed counter using sequence lock pattern."""
        if self.data is None:
            return

        self.data.seq += 1

        self.data.hands_processed = count
        self.data.last_update_ns = int(time.time_ns())

        self.data.seq += 1

    def set_status(self, status: int):
        """Set status using sequence lock pattern."""
        if self.data is None:
            return

        self.data.seq += 1
        self.data.status = status
        self.data.seq += 1

    def close(self):
        """Close shared memory. DO NOT unlink - C++ collector handles cleanup."""
        if self.shm_mmap:
            self.shm_mmap.close()
        if self.shm_fd and self.shm_fd >= 0:
            os.close(self.shm_fd)
