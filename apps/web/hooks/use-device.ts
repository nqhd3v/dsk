"use client";

import { useCallback, useEffect, useState } from "react";
import type { Device } from "@dsk/schemas";

const API_URL = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:4000";

interface UseDeviceReturn {
  device: Device | null;
  loading: boolean;
  error: string | null;
  refetch: () => void;
}

export function useDevice(nodeId: string): UseDeviceReturn {
  const [device, setDevice] = useState<Device | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchData = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await fetch(`${API_URL}/api/devices`);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const json: Device[] = await res.json();
      const deviceData = json.find((d) => d.node_id === nodeId);
      if (!deviceData) throw new Error(`Device not found`);
      setDevice(deviceData);
    } catch (e) {
      setError(e instanceof Error ? e.message : "fetch failed");
    } finally {
      setLoading(false);
    }
  }, [nodeId]);

  useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect
    fetchData();
  }, [fetchData]);

  return { device, loading, error, refetch: fetchData };
}
