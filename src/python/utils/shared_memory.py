import ctypes
import mmap
import os
import time
from ctypes import Structure, c_uint8, c_uint32, c_uint64, c_double, c_char
from typing import Optional, Dict

# Maximum number of poker hands (13x13 matrix)
MAX_HANDS = 169


class HandEquityResult(Structure):
    _fields_ = [
        ("equity", c_double),                    # 8 bytes
        ("wins", c_uint32),                      # 4 bytes
        ("ties", c_uint32),                      # 4 bytes
        ("losses", c_uint32),                    # 4 bytes
        ("simulations", c_uint32),               # 4 bytes
        ("win_method_matrix", (c_uint32 * 10) * 10),  # 400 bytes (10x10 matrix)
        ("_padding", c_uint32 * 6),              # 24 bytes (total 448 bytes)
    ]


class EquityResultsSegment(Structure):
    _fields_ = [
        ("seq", c_uint32),
        ("results_count", c_uint32),
        ("hand_names", (c_char * 8) * MAX_HANDS),
        ("results", HandEquityResult * MAX_HANDS),
    ]


class TelemetrySharedMemory(Structure):
    _fields_ = [
        ("seq", c_uint32),
        ("_padding1", c_uint32),
        ("job_start_ns", c_uint64),
        ("hands_processed", c_uint64),
        ("last_update_ns", c_uint64),
        ("status", c_uint8),
        ("_reserved", c_uint8 * 31),
    ]


class CompleteSharedMemory(Structure):
    _fields_ = [
        ("telemetry", TelemetrySharedMemory),
        ("equity_results", EquityResultsSegment),
    ]


class SharedMemoryWriter:
    def __init__(self, job_id: str):
        self.job_id = job_id
        self.shm_name = f"/poker_telemetry_{job_id}"
        self.shm_fd: Optional[int] = None
        self.shm_mmap: Optional[mmap.mmap] = None
        self.data: Optional[CompleteSharedMemory] = None

    def create(self) -> bool:
        """Create shared memory segment. Returns True on success."""
        try:
            shm_path = f"/dev/shm{self.shm_name}"
            self.shm_fd = os.open(shm_path, os.O_CREAT | os.O_RDWR | os.O_EXCL, 0o600)
            os.ftruncate(self.shm_fd, ctypes.sizeof(CompleteSharedMemory))

            self.shm_mmap = mmap.mmap(
                self.shm_fd, ctypes.sizeof(CompleteSharedMemory), access=mmap.ACCESS_WRITE
            )

            self.data = CompleteSharedMemory.from_buffer(self.shm_mmap)
            self.data.telemetry.seq = 0
            self.data.telemetry.job_start_ns = int(time.time_ns())
            self.data.telemetry.hands_processed = 0
            self.data.telemetry.last_update_ns = int(time.time_ns())
            self.data.telemetry.status = 0
            self.data.equity_results.seq = 0
            self.data.equity_results.results_count = 0

            return True
        except Exception as e:
            import logging

            logging.warning(f"Failed to create shared memory {self.shm_name}: {e}")
            return False

    def update_hands(self, count: int):
        """Update hands_processed counter using sequence lock pattern."""
        if self.data is None:
            return

        self.data.telemetry.seq += 1

        self.data.telemetry.hands_processed = count
        self.data.telemetry.last_update_ns = int(time.time_ns())

        self.data.telemetry.seq += 1

    def set_status(self, status: int):
        """Set status using sequence lock pattern."""
        if self.data is None:
            return

        self.data.telemetry.seq += 1
        self.data.telemetry.status = status
        self.data.telemetry.seq += 1

    def update_equity_results(self, results: Dict[str, any]):
        """Update equity results using sequence lock pattern.

        Args:
            results: Dict mapping hand_name -> EquityResult
        """
        if self.data is None:
            return

        self.data.equity_results.seq += 1

        # Update results count
        self.data.equity_results.results_count = min(len(results), MAX_HANDS)

        # Write each result
        for idx, (hand_name, result) in enumerate(results.items()):
            if idx >= MAX_HANDS:
                break

            # Write hand name (null-terminated, max 7 chars + null)
            hand_name_bytes = hand_name.encode('utf-8')[:7]
            for i in range(8):
                if i < len(hand_name_bytes):
                    self.data.equity_results.hand_names[idx][i] = hand_name_bytes[i]
                else:
                    self.data.equity_results.hand_names[idx][i] = 0

            # Write equity data
            self.data.equity_results.results[idx].equity = result.equity
            self.data.equity_results.results[idx].wins = result.wins
            self.data.equity_results.results[idx].ties = result.ties
            self.data.equity_results.results[idx].losses = result.losses
            self.data.equity_results.results[idx].simulations = result.total_simulations

            # Write win-method matrix
            if result.win_method_matrix is not None:
                for our_type in range(10):
                    for opp_type in range(10):
                        self.data.equity_results.results[idx].win_method_matrix[our_type][opp_type] = \
                            result.win_method_matrix[our_type][opp_type]

        self.data.equity_results.seq += 1

    def close(self):
        """Close shared memory. DO NOT unlink - C++ collector handles cleanup."""
        if self.shm_mmap:
            self.shm_mmap.close()
        if self.shm_fd and self.shm_fd >= 0:
            os.close(self.shm_fd)
