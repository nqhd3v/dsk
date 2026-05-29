"use client";

import { useCallback, useState } from "react";
import type { ConfigCmd } from "@dsk/schemas";

const API_URL = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:4000";

// Fields we can push to the device — subset of ConfigCmd (ts added by server)
export type DeviceConfigPayload = Omit<ConfigCmd, "ts">;

interface UseDeviceConfigReturn {
  push: (deviceId: string, payload: DeviceConfigPayload) => Promise<void>;
  saving: boolean;
  error: string | null;
}

export function useDeviceConfig(): UseDeviceConfigReturn {
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const push = useCallback(
    async (deviceId: string, payload: DeviceConfigPayload) => {
      setSaving(true);
      setError(null);
      try {
        const res = await fetch(`${API_URL}/api/devices/${deviceId}/config`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });
        if (!res.ok) {
          const text = await res.text();
          throw new Error(`HTTP ${res.status}: ${text}`);
        }
      } catch (e) {
        setError(e instanceof Error ? e.message : "push failed");
        throw e;
      } finally {
        setSaving(false);
      }
    },
    [],
  );

  return { push, saving, error };
}
