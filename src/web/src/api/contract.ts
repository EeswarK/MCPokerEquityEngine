import { EngineMode, HandRange, JobStatus, Card } from "../types";

export interface CreateJobRequest {
  range_spec: HandRange;
  board?: Card[];
  num_opponents: number; // 1-9
  num_simulations: number; // 1000-10000000
  mode: EngineMode;
  num_workers?: number; // Optional, for multiprocessing/threaded modes
}

export interface CreateJobResponse {
  job_id: string;
  status: JobStatus;
  created_at: string; // ISO 8601 timestamp
}

export interface JobStatusResponse {
  job_id: string;
  status: JobStatus;
  progress: number; // 0.0 to 1.0
  created_at: string;
  completed_at?: string;
  error?: string;
}

export interface EquityResult {
  hand_name: string;
  equity: number; // 0.0 to 1.0
  wins: number;
  ties: number;
  losses: number;
  total_simulations: number;
}

export interface PerformanceMetrics {
  mode: EngineMode;
  duration_seconds: number;
  simulations_per_second: number;
  cpu_percent?: number;
  memory_mb?: number;
  num_workers?: number;
  cache_misses?: number; // Future: from perf counters
  branch_misses?: number; // Future: from perf counters
}

export interface TelemetryUpdate {
  job_id: string;
  status: JobStatus;
  progress: number; // 0.0 to 1.0
  current_results: Record<string, number>; // hand_name -> equity
  metrics: PerformanceMetrics;
  timestamp: string; // ISO 8601
}

export interface ErrorResponse {
  error: string;
  code: string;
  details?: Record<string, unknown>;
}

// WebSocket Message Types
export type WebSocketMessage =
  | { type: "telemetry"; data: TelemetryUpdate }
  | { type: "error"; data: ErrorResponse }
  | { type: "ping"; data: { timestamp: string } }
  | { type: "pong"; data: { timestamp: string } };

// WebSocket Connection
// Client connects to: ws://host:port/ws/{job_id}
// Server sends: WebSocketMessage
// Client can send: { type: 'ping' } to keep connection alive
