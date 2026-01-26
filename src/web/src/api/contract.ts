import { EngineMode, HandRange, JobStatus, Card, AlgorithmType, OptimizationType } from "../types";

export interface CreateJobRequest {
  range_spec: HandRange;
  board?: Card[];
  num_opponents: number; // 1-9
  num_simulations: number; // 1000-10000000

  // Implementation selector (for routing)
  implementation?: string; // "python" or "cpp"

  // New algorithm selection (preferred approach)
  algorithm?: AlgorithmType; // Core algorithm to use
  optimizations?: OptimizationType[]; // Optional optimizations to apply
  num_workers?: number; // Optional, for multithreading optimization

  // Legacy mode field (deprecated, but kept for backwards compatibility)
  mode?: EngineMode;
}

export interface CreateJobResponse {
  job_id: string;
  status: JobStatus;
  created_at: string; // ISO 8601 timestamp
  telemetry_ws_url?: string;
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
  // Legacy mode field (deprecated)
  mode?: EngineMode;

  // New algorithm fields
  algorithm?: AlgorithmType;
  optimizations?: OptimizationType[];

  duration_seconds: number;
  simulations_per_second: number;
  cpu_percent?: number;
  memory_mb?: number;
  num_workers?: number;
  cpu_cycles?: number; // From perf_event_open hardware counter
  cache_misses?: number; // Future: from perf counters
  branch_misses?: number; // Future: from perf counters
}

export interface TelemetryUpdate {
  job_id: string;
  status: JobStatus;
  progress: number; // 0.0 to 1.0
  current_results: Record<string, number>; // hand_name -> equity
  sample_counts: Record<string, number>; // hand_name -> simulation count
  win_method_matrices: Record<string, number[][]>; // hand_name -> 10x10 matrix [our_type][opp_type]
  loss_method_matrices: Record<string, number[][]>; // hand_name -> 10x10 matrix [opp_type][our_type]
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
