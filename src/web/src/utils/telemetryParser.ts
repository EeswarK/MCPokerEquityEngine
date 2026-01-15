import * as flatbuffers from "flatbuffers";
import { TelemetryPacket } from "../api/telemetry/telemetry-packet.js";
import {
  PerformanceMetrics,
  TelemetryUpdate,
  JobStatus,
} from "../api/contract";

interface BinaryTelemetryData {
  timestamp_ns: bigint;
  job_start_ns: bigint;
  hands_processed: bigint;
  cpu_percent: number;
  memory_rss_kb: bigint;
  memory_vms_kb: bigint;
  thread_count: number;
  cpu_cycles: bigint;
  status: number;
}

export function parseTelemetryPacket(
  buffer: ArrayBuffer,
  jobId: string,
  previousHandsProcessed: bigint,
  previousTimestampNs: bigint
): TelemetryUpdate | null {
  try {
    const bytes = new Uint8Array(buffer);
    const bb = new flatbuffers.ByteBuffer(bytes);

    const packet = TelemetryPacket.getRootAsTelemetryPacket(bb);

    const data: BinaryTelemetryData = {
      timestamp_ns: packet.timestampNs(),
      job_start_ns: packet.jobStartNs(),
      hands_processed: packet.handsProcessed(),
      cpu_percent: packet.cpuPercent(),
      memory_rss_kb: packet.memoryRssKb(),
      memory_vms_kb: packet.memoryVmsKb(),
      thread_count: packet.threadCount(),
      cpu_cycles: packet.cpuCycles(),
      status: packet.status(),
    };

    // Helper function to convert specific hands (e.g., "AsKh") to general types (e.g., "AKs")
    const normalizeHandName = (handName: string): string => {
      if (!handName || handName.length < 2) return handName;

      // Parse hand name (e.g., "AsKh" or "AA" or "AKs")
      // If it's already a general type (2-3 chars), return as-is
      if (handName.length <= 3) return handName;

      // Extract ranks and suits from specific hand (e.g., "AsKh" -> A, s, K, h)
      const rank1Char = handName[0];
      const suit1Char = handName[1];
      const rank2Char = handName[2];
      const suit2Char = handName[3];

      // Check if it's a pair
      if (rank1Char === rank2Char) {
        return `${rank1Char}${rank1Char}`;
      }

      // Check if suited (same suit)
      const suited = suit1Char === suit2Char;

      // Sort ranks (higher rank first)
      const ranks = [rank1Char, rank2Char];
      const rankOrder = "23456789TJQKA";
      ranks.sort((a, b) => rankOrder.indexOf(b) - rankOrder.indexOf(a));

      return `${ranks[0]}${ranks[1]}${suited ? "s" : "o"}`;
    };

    // Extract equity results, sample counts, and win-method matrices
    const currentResults: Record<string, number> = {};
    const sampleCounts: Record<string, number> = {};
    const winMethodMatrices: Record<string, number[][]> = {};
    const equityResultsLength = packet.equityResultsLength();

    for (let i = 0; i < equityResultsLength; i++) {
      const handEquity = packet.equityResults(i);
      if (handEquity) {
        const originalHandName = handEquity.handName();
        if (originalHandName) {
          // Normalize hand name for heatmap display
          const normalizedHandName = normalizeHandName(originalHandName);
          currentResults[normalizedHandName] = handEquity.equity();
          sampleCounts[normalizedHandName] = handEquity.simulations();

          // Parse win-method matrix (flattened 100-element array)
          if (typeof handEquity.winMethodMatrixArray === 'function') {
            const matrixFlat = handEquity.winMethodMatrixArray();
            if (matrixFlat && matrixFlat.length === 100) {
              const matrix: number[][] = [];
              for (let our_type = 0; our_type < 10; our_type++) {
                matrix[our_type] = [];
                for (let opp_type = 0; opp_type < 10; opp_type++) {
                  matrix[our_type][opp_type] = matrixFlat[our_type * 10 + opp_type];
                }
              }
              winMethodMatrices[normalizedHandName] = matrix;
            }
          }
        }
      }
    }

    // Use cumulative calculation instead of delta for smoother, more stable metrics
    // This avoids the spike on first packet and smooths out noisy delta calculations
    const totalHandsProcessed = Number(data.hands_processed);
    const totalTimeElapsedNs = Number(data.timestamp_ns - data.job_start_ns);
    const totalTimeElapsedSeconds = totalTimeElapsedNs / 1e9;

    // Calculate cumulative simulations per second
    // This gives a smooth average over the entire job duration
    const simulationsPerSecond =
      totalTimeElapsedSeconds > 0 ? totalHandsProcessed / totalTimeElapsedSeconds : 0;

    const statusMap: Record<number, JobStatus> = {
      0: "running",
      1: "completed",
      2: "failed",
    };
    const jobStatus = statusMap[data.status] || "running";

    const metrics: PerformanceMetrics = {
      mode: "base_python",
      duration_seconds: Number(data.timestamp_ns - data.job_start_ns) / 1e9,
      simulations_per_second: simulationsPerSecond,
      cpu_percent: data.cpu_percent,
      memory_mb: Number(data.memory_rss_kb) / 1024,
      num_workers: data.thread_count > 1 ? data.thread_count : undefined,
      cpu_cycles: data.cpu_cycles > BigInt(0) ? Number(data.cpu_cycles) : undefined,
    };

    const telemetryUpdate: TelemetryUpdate = {
      job_id: jobId,
      status: jobStatus,
      progress: jobStatus === "completed" ? 1.0 : 0.0,
      current_results: currentResults,
      sample_counts: sampleCounts,
      win_method_matrices: winMethodMatrices,
      metrics,
      timestamp: new Date(
        Number(data.timestamp_ns / BigInt(1e6))
      ).toISOString(),
    };

    console.log("[Telemetry] Parsed update:", {
      equityResultsCount: equityResultsLength,
      currentResultsKeys: Object.keys(currentResults),
      sampleCountsKeys: Object.keys(sampleCounts),
      winMethodMatricesKeys: Object.keys(winMethodMatrices),
      metricsSimsPerSec: metrics.simulations_per_second,
    });

    return telemetryUpdate;
  } catch (error) {
    console.error("Failed to parse telemetry packet:", error);
    return null;
  }
}

export function extractPacketData(buffer: ArrayBuffer): {
  handsProcessed: bigint;
  timestampNs: bigint;
} | null {
  try {
    const bytes = new Uint8Array(buffer);
    const bb = new flatbuffers.ByteBuffer(bytes);
    const packet = TelemetryPacket.getRootAsTelemetryPacket(bb);
    return {
      handsProcessed: packet.handsProcessed(),
      timestampNs: packet.timestampNs(),
    };
  } catch (error) {
    console.error("Failed to extract packet data:", error);
    return null;
  }
}
