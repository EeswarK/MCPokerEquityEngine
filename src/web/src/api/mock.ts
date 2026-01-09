import {
  CreateJobRequest,
  CreateJobResponse,
  JobStatusResponse,
  WebSocketMessage,
} from "./contract";
import { EngineMode } from "../types";

const MOCK_API_DELAY = 100; // ms

export class MockAPIClient {
  private jobs: Map<
    string,
    {
      request: CreateJobRequest;
      status: "pending" | "running" | "completed" | "failed";
      progress: number;
      results: Record<string, number>;
      startTime: number;
    }
  > = new Map();

  async createJob(request: CreateJobRequest): Promise<CreateJobResponse> {
    await new Promise((resolve) => setTimeout(resolve, MOCK_API_DELAY));

    const jobId = `job_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;

    this.jobs.set(jobId, {
      request,
      status: "pending",
      progress: 0,
      results: {},
      startTime: Date.now(),
    });

    // Simulate job starting
    setTimeout(() => {
      const job = this.jobs.get(jobId);
      if (job) {
        job.status = "running";
        this.simulateJobProgress(jobId);
      }
    }, 200);

    return {
      job_id: jobId,
      status: "pending",
      created_at: new Date().toISOString(),
    };
  }

  async getJobStatus(jobId: string): Promise<JobStatusResponse> {
    await new Promise((resolve) => setTimeout(resolve, MOCK_API_DELAY));

    const job = this.jobs.get(jobId);
    if (!job) {
      throw new Error("Job not found");
    }

    return {
      job_id: jobId,
      status: job.status,
      progress: job.progress,
      created_at: new Date(job.startTime).toISOString(),
      completed_at:
        job.status === "completed" ? new Date().toISOString() : undefined,
    };
  }

  private simulateJobProgress(jobId: string) {
    const job = this.jobs.get(jobId);
    if (!job || job.status !== "running") return;

    const handNames = Object.keys(job.request.range_spec);
    const totalHands = handNames.length;

    let completedHands = 0;
    const interval = setInterval(() => {
      if (completedHands < totalHands) {
        const handName = handNames[completedHands];
        // Simulate equity calculation (random for now)
        job.results[handName] = 0.5 + Math.random() * 0.4; // 0.5 to 0.9

        completedHands++;
        job.progress = completedHands / totalHands;
      } else {
        job.status = "completed";
        job.progress = 1.0;
        clearInterval(interval);
      }
    }, 500); // Update every 500ms
  }

  createMockWebSocket(jobId: string): WebSocket {
    // Create a mock WebSocket that simulates telemetry updates
    let readyStateValue: number = WebSocket.CONNECTING;
    const mockWs = {
      get readyState() {
        return readyStateValue;
      },
      onopen: null as ((event: Event) => void) | null,
      onmessage: null as ((event: MessageEvent) => void) | null,
      onerror: null as ((event: Event) => void) | null,
      onclose: null as ((event: CloseEvent) => void) | null,
      send: () => {},
      close: () => {
        readyStateValue = WebSocket.CLOSED;
        if (mockWs.onclose) {
          mockWs.onclose(new CloseEvent("close"));
        }
      },
    } as unknown as WebSocket;

    // Simulate connection
    setTimeout(() => {
      readyStateValue = WebSocket.OPEN;
      if (mockWs.onopen) {
        mockWs.onopen(new Event("open"));
      }

      const job = this.jobs.get(jobId);
      if (!job) return;

      const interval = setInterval(() => {
        if (job.status === "completed" || job.status === "failed") {
          clearInterval(interval);
          readyStateValue = WebSocket.CLOSED;
          if (mockWs.onclose) {
            mockWs.onclose(new CloseEvent("close"));
          }
          return;
        }

        const message: WebSocketMessage = {
          type: "telemetry",
          data: {
            job_id: jobId,
            status: job.status,
            progress: job.progress,
            current_results: { ...job.results },
            metrics: this.generateMockMetrics(job.request.mode),
            timestamp: new Date().toISOString(),
          },
        };

        if (mockWs.onmessage) {
          mockWs.onmessage(
            new MessageEvent("message", {
              data: JSON.stringify(message),
            }),
          );
        }
      }, 200); // Send update every 200ms
    }, 100);

    return mockWs;
  }

  private generateMockMetrics(mode: EngineMode) {
    const baseSimsPerSec = 10000;
    const multipliers: Record<EngineMode, number> = {
      base_python: 1.0,
      numpy: 3.0,
      multiprocessing: 2.5,
      cpp_base: 5.0,
      cpp_simd: 15.0,
      cpp_threaded: 8.0,
    };

    return {
      mode,
      duration_seconds: 0,
      simulations_per_second: baseSimsPerSec * (multipliers[mode] || 1.0),
      cpu_percent: 80 + Math.random() * 15,
      memory_mb: 200 + Math.random() * 100,
    };
  }
}

export const mockClient = new MockAPIClient();
