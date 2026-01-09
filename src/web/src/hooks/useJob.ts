import { useState, useCallback } from "react";
import { CreateJobRequest, CreateJobResponse } from "../api/contract";
import { apiClient } from "../api/client";
import { useWebSocket } from "./useWebSocket";

export function useJob() {
  const [jobId, setJobId] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const { data: telemetry, connected } = useWebSocket(jobId);

  const submitJob = useCallback(async (request: CreateJobRequest) => {
    setLoading(true);
    setError(null);

    try {
      const response: CreateJobResponse = await apiClient.createJob(request);
      setJobId(response.job_id);
    } catch (err) {
      const errorMessage =
        err instanceof Error ? err.message : "Failed to create job";
      setError(errorMessage);
      setJobId(null);
    } finally {
      setLoading(false);
    }
  }, []);

  const resetJob = useCallback(() => {
    setJobId(null);
    setError(null);
  }, []);

  return {
    jobId,
    submitJob,
    resetJob,
    loading,
    error,
    telemetry,
    connected,
  };
}
