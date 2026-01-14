import { useEffect, useRef, useState, useCallback } from "react";
import { TelemetryUpdate, WebSocketMessage } from "../api/contract";
import { apiClient } from "../api/client";

export function useWebSocket(jobId: string | null, telemetryWsUrl: string | null) {
  const [data, setData] = useState<TelemetryUpdate | null>(null);
  const [connected, setConnected] = useState(false);
  const [telemetryConnected, setTelemetryConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const wsRef = useRef<WebSocket | null>(null);
  const telemetryWsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);
  const telemetryReconnectTimeoutRef = useRef<number | null>(null);

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

  const connectTelemetry = useCallback(() => {
    if (!telemetryWsUrl) return;

    try {
      const ws = new WebSocket(telemetryWsUrl);
      telemetryWsRef.current = ws;

      ws.onopen = () => {
        setTelemetryConnected(true);
        if (telemetryReconnectTimeoutRef.current) {
          clearTimeout(telemetryReconnectTimeoutRef.current);
          telemetryReconnectTimeoutRef.current = null;
        }
      };

      ws.onmessage = (event) => {
        if (event.data instanceof ArrayBuffer || event.data instanceof Blob) {
          console.log("Received binary telemetry data:", event.data.byteLength || (event.data as Blob).size, "bytes");
        }
      };

      ws.onerror = (err) => {
        console.warn("Telemetry WebSocket error (non-fatal):", err);
        setTelemetryConnected(false);
      };

      ws.onclose = () => {
        setTelemetryConnected(false);
        if (telemetryWsUrl) {
          telemetryReconnectTimeoutRef.current = window.setTimeout(() => {
            connectTelemetry();
          }, 2000);
        }
      };
    } catch (err) {
      console.warn("Failed to create telemetry WebSocket (non-fatal):", err);
    }
  }, [telemetryWsUrl]);

  useEffect(() => {
    connect();
    if (telemetryWsUrl) {
      connectTelemetry();
    }
    return () => {
      if (wsRef.current) {
        wsRef.current.close();
      }
      if (telemetryWsRef.current) {
        telemetryWsRef.current.close();
      }
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      if (telemetryReconnectTimeoutRef.current) {
        clearTimeout(telemetryReconnectTimeoutRef.current);
      }
    };
  }, [connect, connectTelemetry]);

  return { data, connected, telemetryConnected, error };
}
