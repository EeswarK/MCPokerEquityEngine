import { useEffect, useRef, useState, useCallback } from "react";
import { TelemetryUpdate, WebSocketMessage } from "../api/contract";
import { apiClient } from "../api/client";

export function useWebSocket(jobId: string | null) {
  const [data, setData] = useState<TelemetryUpdate | null>(null);
  const [connected, setConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);

  const connect = useCallback(() => {
    if (!jobId) return;

    try {
      const ws = apiClient.createWebSocket(jobId);
      wsRef.current = ws;

      ws.onopen = () => {
        setConnected(true);
        setError(null);
        if (reconnectTimeoutRef.current) {
          clearTimeout(reconnectTimeoutRef.current);
          reconnectTimeoutRef.current = null;
        }
      };

      ws.onmessage = (event) => {
        try {
          const message: WebSocketMessage = JSON.parse(event.data);
          if (message.type === "telemetry") {
            setData(message.data);
          } else if (message.type === "error") {
            setError(message.data.error);
          }
        } catch (err) {
          console.error("Failed to parse WebSocket message:", err);
        }
      };

      ws.onerror = (err) => {
        console.error("WebSocket error:", err);
        setError("WebSocket connection error");
        setConnected(false);
      };

      ws.onclose = () => {
        setConnected(false);
        // Attempt reconnect after 2 seconds if job is still active
        if (jobId) {
          reconnectTimeoutRef.current = window.setTimeout(() => {
            connect();
          }, 2000);
        }
      };
    } catch (err) {
      console.error("Failed to create WebSocket:", err);
      setError("Failed to create WebSocket connection");
    }
  }, [jobId]);

  useEffect(() => {
    connect();
    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
    };
  }, [connect]);

  return { data, connected, error };
}
