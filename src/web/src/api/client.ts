import axios from "axios";
import {
  CreateJobRequest,
  CreateJobResponse,
  JobStatusResponse,
} from "./contract";
import { mockClient } from "./mock";

const API_BASE_URL = import.meta.env.VITE_API_URL || "http://localhost:8000";
const USE_MOCK = import.meta.env.VITE_USE_MOCK === "true" || !API_BASE_URL;

export class APIClient {
  async createJob(request: CreateJobRequest): Promise<CreateJobResponse> {
    if (USE_MOCK) {
      return mockClient.createJob(request);
    }

    const response = await axios.post(`${API_BASE_URL}/api/jobs`, request);
    return response.data;
  }

  async getJobStatus(jobId: string): Promise<JobStatusResponse> {
    if (USE_MOCK) {
      return mockClient.getJobStatus(jobId);
    }

    const response = await axios.get(
      `${API_BASE_URL}/api/jobs/${jobId}/status`,
    );
    return response.data;
  }

  createWebSocket(jobId: string): WebSocket {
    if (USE_MOCK) {
      return mockClient.createMockWebSocket(jobId);
    }

    const wsUrl = import.meta.env.VITE_WS_URL || "ws://localhost:8000";
    return new WebSocket(`${wsUrl}/ws/${jobId}`);
  }
}

export const apiClient = new APIClient();
