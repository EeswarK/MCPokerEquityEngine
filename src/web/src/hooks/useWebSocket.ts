import { useEffect, useRef, useState, useCallback } from "react";
import { TelemetryUpdate, WebSocketMessage } from "../api/contract";
import { apiClient } from "../api/client";
import {
  parseTelemetryPacket,
  extractPacketData,
} from "../utils/telemetryParser";

export function useWebSocket(
  jobId: string | null,
  telemetryWsUrl: string | null,
) {
  const [data, setData] = useState<TelemetryUpdate | null>(null);
  const [connected, setConnected] = useState(false);
  const [telemetryConnected, setTelemetryConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const wsRef = useRef<WebSocket | null>(null);
  const telemetryWsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);
  const telemetryReconnectTimeoutRef = useRef<number | null>(null);
  const previousHandsProcessedRef = useRef<bigint>(BigInt(0));
  const previousTimestampNsRef = useRef<bigint>(BigInt(0));

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
            // Merge with existing data to preserve metrics from C++ telemetry WebSocket
            setData((prevData) => ({
              ...message.data,
              metrics: prevData?.metrics || message.data.metrics,
            }));
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
        previousHandsProcessedRef.current = BigInt(0);
        previousTimestampNsRef.current = BigInt(0);
        if (telemetryReconnectTimeoutRef.current) {
          clearTimeout(telemetryReconnectTimeoutRef.current);
          telemetryReconnectTimeoutRef.current = null;
        }
      };

      ws.onmessage = async (event) => {
        if (event.data instanceof ArrayBuffer) {
          const parsed = parseTelemetryPacket(
            event.data,
            jobId || "",
            previousHandsProcessedRef.current,
            previousTimestampNsRef.current,
          );

          if (parsed) {
            // Merge with existing data to preserve current_results from Python WebSocket
            setData((prevData) => ({
              ...parsed,
              current_results: prevData?.current_results || parsed.current_results,
            }));
            const packetData = extractPacketData(event.data);
            if (packetData) {
              previousHandsProcessedRef.current = packetData.handsProcessed;
              previousTimestampNsRef.current = packetData.timestampNs;
            }
          } else {
            console.warn("Failed to parse telemetry packet");
          }
        } else if (event.data instanceof Blob) {
          const arrayBuffer = await event.data.arrayBuffer();
          const parsed = parseTelemetryPacket(
            arrayBuffer,
            jobId || "",
            previousHandsProcessedRef.current,
            previousTimestampNsRef.current,
          );

          if (parsed) {
            // Merge with existing data to preserve current_results from Python WebSocket
            setData((prevData) => ({
              ...parsed,
              current_results: prevData?.current_results || parsed.current_results,
            }));
            const packetData = extractPacketData(arrayBuffer);
            if (packetData) {
              previousHandsProcessedRef.current = packetData.handsProcessed;
              previousTimestampNsRef.current = packetData.timestampNs;
            }
          } else {
            console.warn("Failed to parse telemetry packet");
          }
        } else {
          console.warn(
            "Received non-binary telemetry data:",
            typeof event.data,
          );
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
