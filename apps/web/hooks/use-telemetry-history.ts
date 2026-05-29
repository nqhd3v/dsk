"use client";

import { useEffect, useState } from "react";
import type { EnvTelemetry } from "@dsk/schemas";

const API_URL = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:4000";

/**
 * Fetches recent telemetry history for a node.
 * Returns rows ordered oldest→newest (server returns latest-first, we reverse).
 */
export function useTelemetryHistory(
  nodeId: string,
  limit = 360,
): { rows: EnvTelemetry[]; loading: boolean } {
  const [rows, setRows] = useState<EnvTelemetry[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let cancelled = false;
    setLoading(true);

    fetch(`${API_URL}/api/telemetry/${nodeId}/recent?limit=${limit}`)
      .then((r) => (r.ok ? r.json() : Promise.reject(r.status)))
      .then((data: EnvTelemetry[]) => {
        if (!cancelled) {
          // Server returns newest-first; reverse for chronological order
          setRows([...data].reverse());
        }
      })
      .catch(() => {
        if (!cancelled) setRows([]);
      })
      .finally(() => {
        if (!cancelled) setLoading(false);
      });

    return () => {
      cancelled = true;
    };
  }, [nodeId, limit]);

  return { rows, loading };
}
