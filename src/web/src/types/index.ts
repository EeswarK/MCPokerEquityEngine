export type EngineMode =
  | "base_python"
  | "numpy"
  | "multiprocessing"
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
