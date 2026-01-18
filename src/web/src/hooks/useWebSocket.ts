import { useEffect, useRef, useState, useCallback } from "react";
import { TelemetryUpdate } from "../api/contract";
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
  const telemetryWsRef = useRef<WebSocket | null>(null);
  const telemetryReconnectTimeoutRef = useRef<number | null>(null);
  const previousHandsProcessedRef = useRef<bigint>(BigInt(0));
  const previousTimestampNsRef = useRef<bigint>(BigInt(0));

  const connectTelemetry = useCallback(() => {
    if (!telemetryWsUrl) return;

    try {
      const ws = new WebSocket(telemetryWsUrl);
      telemetryWsRef.current = ws;

      ws.onopen = () => {
        setConnected(true);
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
            // C++ telemetry now contains everything (metrics + equity results)
            setData(parsed);
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
            // C++ telemetry now contains everything (metrics + equity results)
            setData(parsed);
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
        setConnected(false);
      };

      ws.onclose = () => {
        setConnected(false);
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
    if (telemetryWsUrl) {
      connectTelemetry();
    }
    return () => {
      if (telemetryWsRef.current) {
        telemetryWsRef.current.close();
      }
      if (telemetryReconnectTimeoutRef.current) {
        clearTimeout(telemetryReconnectTimeoutRef.current);
      }
    };
  }, [connectTelemetry]);

  return { data, connected };
}
