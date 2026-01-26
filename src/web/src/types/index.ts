// Core algorithm types (matches backend AlgorithmType enum - lowercase)
export type AlgorithmType =
  | "naive"
  | "cactus_kev"
  | "ph_evaluator"
  | "two_plus_two"
  | "omp_eval";

// Optimization flags (matches backend OptimizationType enum - lowercase)
export type OptimizationType =
  | "multithreading"
  | "simd"
  | "perfect_hash"
  | "prefetching";

// Implementation type for routing requests
export type ImplementationType = "python" | "cpp";

// Legacy engine mode for backwards compatibility
export type EngineMode =
  | "base_python"
  | "senzee"
  | "numpy"
  | "multiprocessing"
  | "cpp_naive"
  | "cpp_base"
  | "cpp_simd"
  | "cpp_threaded";

export interface Card {
  rank: number; // 2-14 (2-10, J=11, Q=12, K=13, A=14)
  suit: number; // 0-3 (hearts, diamonds, clubs, spades)
}

export interface HandRange {
  [handName: string]: Card[]; // e.g., "AA" -> [Card(14,0), Card(14,1)]
}

export type JobStatus = "pending" | "running" | "completed" | "failed";
