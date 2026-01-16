import * as flatbuffers from "flatbuffers";
import { Telemetry } from "../api/telemetry_generated";
import {
  PerformanceMetrics,
  TelemetryUpdate,
  JobStatus,
} from "../api/contract";

interface BinaryTelemetryData {
  timestamp_ns: bigint;
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

    const packet = Telemetry.TelemetryPacket.getRootAsTelemetryPacket(bb);

    const data: BinaryTelemetryData = {
      timestamp_ns: packet.timestampNs(),
      hands_processed: packet.handsProcessed(),
      cpu_percent: packet.cpuPercent(),
      memory_rss_kb: packet.memoryRssKb(),
      memory_vms_kb: packet.memoryVmsKb(),
      thread_count: packet.threadCount(),
      cpu_cycles: packet.cpuCycles(),
      status: packet.status(),
    };

    // Extract equity results
    const currentResults: Record<string, number> = {};
    const equityResultsLength = packet.equityResultsLength();
    for (let i = 0; i < equityResultsLength; i++) {
      const handEquity = packet.equityResults(i);
      if (handEquity) {
        const handName = handEquity.handName();
        if (handName) {
          currentResults[handName] = handEquity.equity();
        }
      }
    }

    const handsDelta = Number(data.hands_processed - previousHandsProcessed);
    const timeDeltaNs = Number(data.timestamp_ns - previousTimestampNs);
    const timeDeltaSeconds = timeDeltaNs / 1e9;
    const simulationsPerSecond =
      timeDeltaSeconds > 0 ? handsDelta / timeDeltaSeconds : 0;

    const statusMap: Record<number, JobStatus> = {
      0: "running",
      1: "completed",
      2: "failed",
    };
    const jobStatus = statusMap[data.status] || "running";

    const metrics: PerformanceMetrics = {
      mode: "base_python",
      duration_seconds: Number(data.timestamp_ns) / 1e9,
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
      metrics,
      timestamp: new Date(
        Number(data.timestamp_ns / BigInt(1e6))
      ).toISOString(),
    };

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
    const packet = Telemetry.TelemetryPacket.getRootAsTelemetryPacket(bb);
    return {
      handsProcessed: packet.handsProcessed(),
      timestampNs: packet.timestampNs(),
    };
  } catch (error) {
    console.error("Failed to extract packet data:", error);
    return null;
  }
}
