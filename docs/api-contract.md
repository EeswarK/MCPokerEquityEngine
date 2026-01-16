# Poker Equity Engine API Contract

This document defines the API contract that all backend implementations (Python, C++) must follow.

## REST Endpoints

### POST /api/jobs

Create a new equity calculation job.

**Request:**

```typescript
{
  range_spec: { [handName: string]: Card[] },
  board?: Card[],
  num_opponents: number,      // 1-9
  num_simulations: number,    // 1000-10000000
  mode: EngineMode,
  num_workers?: number
}
```

**Response:**

```typescript
{
  job_id: string,
  status: "pending" | "running" | "completed" | "failed",
  created_at: string,  // ISO 8601
  telemetry_ws_url?: string  // WebSocket URL for direct telemetry connection (e.g., "ws://localhost:8001/telemetry/{job_id}")
}
```

The `telemetry_ws_url` field contains the WebSocket URL for connecting directly to the C++ telemetry collector. This is separate from the job management WebSocket connection at `ws://host:port/ws/{job_id}`.

### GET /api/jobs/{job_id}/status

Get current status of a job.

**Response:**

```typescript
{
  job_id: string,
  status: "pending" | "running" | "completed" | "failed",
  progress: number,  // 0.0 to 1.0
  created_at: string,
  completed_at?: string,
  error?: string
}
```

## WebSocket Protocol

### Connection

- **URL**: `ws://host:port/ws/{job_id}`
- **Protocol**: Standard WebSocket (text frames, JSON messages)

### Message Format

All messages are JSON objects with `type` and `data` fields:

```typescript
{
  type: "telemetry" | "error" | "ping" | "pong",
  data: TelemetryUpdate | ErrorResponse | { timestamp: string }
}
```

### Telemetry Updates

Server sends periodic updates during job execution:

```typescript
{
  type: "telemetry",
  data: {
    job_id: string,
    status: "running",
    progress: 0.75,
    current_results: { "AA": 0.85, "KK": 0.82, ... },
    metrics: {
      mode: "numpy",
      duration_seconds: 2.5,
      simulations_per_second: 40000,
      cpu_percent: 95.2,
      memory_mb: 512
    },
    timestamp: "2026-01-10T12:34:56.789Z"
  }
}
```

### Error Handling

Errors are sent via WebSocket:

```typescript
{
  type: "error",
  data: {
    error: "Invalid range specification",
    code: "INVALID_RANGE",
    details: { ... }
  }
}
```

## Data Types

### Card

```typescript
{
  rank: number,  // 2-14 (2-10, J=11, Q=12, K=13, A=14)
  suit: number   // 0-3 (hearts, diamonds, clubs, spades)
}
```

### EngineMode

- `"base_python"` - Pure Python implementation
- `"numpy"` - NumPy vectorized Python
- `"multiprocessing"` - Python multiprocessing
- `"cpp_base"` - Base C++ implementation
- `"cpp_simd"` - C++ with SIMD optimizations
- `"cpp_threaded"` - C++ with threading

## Validation Rules

- `num_opponents`: Must be between 1 and 9
- `num_simulations`: Must be between 1000 and 10000000
- `range_spec`: Must contain at least one hand
- `board`: Optional, but if provided must be 0, 3, 4, or 5 cards
- `num_workers`: Optional, only valid for multiprocessing/threaded modes

## Error Codes

- `INVALID_RANGE` - Range specification is invalid
- `INVALID_MODE` - Engine mode not supported
- `JOB_NOT_FOUND` - Job ID does not exist
- `SIMULATION_ERROR` - Error during simulation execution
- `RATE_LIMIT` - Too many requests
