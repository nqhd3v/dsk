"use client";

import { useEffect, useRef, useState } from "react";
import { getSocket } from "@/lib/socket";
import type { EnvTelemetry } from "@dsk/schemas";

/**
 * Subscribe to live telemetry for a specific node.
 * Returns latest EnvTelemetry or null if no data yet.
 *
 * Usage:
 *   const telemetry = useTelemetry("node1");
 *   if (telemetry) console.log(telemetry.co2_ppm);
 */
export function useTelemetry(nodeId: string): EnvTelemetry | null {
  const [data, setData] = useState<EnvTelemetry | null>(null);
  const nodeIdRef = useRef(nodeId);

  useEffect(() => {
    nodeIdRef.current = nodeId;
    const socket = getSocket();

    // Connect if not already
    if (!socket.connected) {
      socket.connect();
    }

    // Listen for telemetry events, filter by nodeId
    const onTelemetry = (payload: { nodeId: string; data: EnvTelemetry }) => {
      if (payload.nodeId === nodeIdRef.current) {
        setData(payload.data);
      }
    };

    socket.on("telemetry", onTelemetry);

    return () => {
      socket.off("telemetry", onTelemetry);
    };
  }, [nodeId]);

  return data;
}

/**
 * Subscribe to ALL nodes.
 * Returns a map: nodeId → latest EnvTelemetry.
 *
 * Usage:
 *   const allTelemetry = useAllTelemetry();
 *   const node1Data = allTelemetry["node1"];
 */
export function useAllTelemetry(): Record<string, EnvTelemetry> {
  const [dataMap, setDataMap] = useState<Record<string, EnvTelemetry>>({});

  useEffect(() => {
    const socket = getSocket();

    if (!socket.connected) {
      socket.connect();
    }

    const onTelemetry = (payload: { nodeId: string; data: EnvTelemetry }) => {
      setDataMap((prev) => ({
        ...prev,
        [payload.nodeId]: payload.data,
      }));
    };

    socket.on("telemetry", onTelemetry);

    return () => {
      socket.off("telemetry", onTelemetry);
    };
  }, []);

  return dataMap;
}
