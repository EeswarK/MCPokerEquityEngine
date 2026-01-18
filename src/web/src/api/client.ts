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
  // Track which backend was used for each job to route status requests correctly
  private jobBackends: Map<string, string> = new Map();

  private getBaseUrl(mode?: string): string {
    if (mode?.startsWith("cpp_")) {
      // For C++ modes, we use the /cpp/ prefix which Nginx routes to port 8002
      // If API_BASE_URL is http://localhost:8000, we might need to handle local dev too
      if (API_BASE_URL.includes("localhost") || API_BASE_URL.includes("127.0.0.1")) {
        return API_BASE_URL.replace(":8000", ":8002");
      }
      return `${API_BASE_URL}/cpp`;
    }
    return API_BASE_URL;
  }

  async createJob(request: CreateJobRequest): Promise<CreateJobResponse> {
    if (USE_MOCK) {
      return mockClient.createJob(request);
    }

    const baseUrl = this.getBaseUrl(request.mode);
    const response = await axios.post(`${baseUrl}/api/jobs`, request);
    
    const jobResponse = response.data as CreateJobResponse;
    this.jobBackends.set(jobResponse.job_id, baseUrl);
    
    return jobResponse;
  }

  async getJobStatus(jobId: string): Promise<JobStatusResponse> {
    if (USE_MOCK) {
      return mockClient.getJobStatus(jobId);
    }

    const baseUrl = this.jobBackends.get(jobId) || API_BASE_URL;
    const response = await axios.get(
      `${baseUrl}/api/jobs/${jobId}/status`,
    );
    return response.data;
  }
}

export const apiClient = new APIClient();
