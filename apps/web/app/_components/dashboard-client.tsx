"use client";

import Link from "next/link";
import { GearIcon, MonitorIcon } from "@phosphor-icons/react";
import { AppHeader } from "@/components/organisms/app-header";
import { SensorGrid } from "@/components/organisms/sensor-grid";
import { TelemetryChart } from "@/components/organisms/telemetry-chart";
import { PresenceBar } from "@/components/molecules/presence-bar";
import { SuggestionCard } from "@/components/molecules/suggestion-card";
import { DeviceInfoPanel } from "@/components/molecules/device-info-panel";
import { Button } from "@/components/atoms/button";
import { useTelemetry } from "@/hooks/use-socket";
import { useDevice } from "@/hooks/use-device";

const THRESHOLD_DEFAULTS = {
  tempMin: 18,
  tempMax: 28,
  humidMin: 30,
  humidMax: 70,
  co2Max: 1000,
  luxMin: 200,
  luxMax: 400,
  vocMin: 30000, // Ω
  sitMinutes: 45,
};

interface DashboardClientProps {
  nodeId: string;
}

function formatLastSeen(iso: string | null): string {
  if (!iso) return "unknown";
  const diff = Math.floor((Date.now() - new Date(iso).getTime()) / 1000);
  if (diff < 5) return "just now";
  if (diff < 60) return `${diff}s ago`;
  if (diff < 3600) return `${Math.floor(diff / 60)}m ago`;
  return `${Math.floor(diff / 3600)}h ago`;
}

export function DashboardClient({ nodeId }: DashboardClientProps) {
  const telemetry = useTelemetry(nodeId);
  const { device, loading: deviceLoading } = useDevice(nodeId);

  // Merge device's last-pushed config with defaults (null = never pushed)
  const thresholds = {
    ...THRESHOLD_DEFAULTS,
    co2Max: device?.cfg_co2_max_ppm ?? THRESHOLD_DEFAULTS.co2Max,
    luxMin: device?.cfg_lux_min ?? THRESHOLD_DEFAULTS.luxMin,
    luxMax: device?.cfg_lux_max ?? THRESHOLD_DEFAULTS.luxMax,
    sitMinutes: device?.cfg_sit_minutes ?? THRESHOLD_DEFAULTS.sitMinutes,
  };

  // Map nullable telemetry fields → sensor data with fallbacks
  const sensorData = telemetry
    ? {
        temperature: telemetry.temp_c ?? 0,
        humidity: telemetry.humidity ?? 0,
        co2: telemetry.co2_ppm ?? 0,
        lux: telemetry.lux ?? 0,
        vocResistance: telemetry.gas_ohm ?? 0,
        pressure: telemetry.pressure_hpa ?? 0,
      }
    : null;

  const present = telemetry?.presence === "PRESENT";
  const sittingMinutes = telemetry ? Math.floor(telemetry.sit_seconds / 60) : 0;

  const co2Elevated = sensorData !== null && sensorData.co2 > thresholds.co2Max;
  const co2Preheating = telemetry?.co2_preheating ?? false;

  const online = device?.status === "active";

  return (
    <div className="flex flex-col min-h-screen">
      <AppHeader nodeId={nodeId} online={online} />

      <main className="flex-1 p-5 pb-10">
        <div className="grid grid-cols-1 md:grid-cols-[1fr_260px] gap-5 max-w-6xl mx-auto">
          {/* Left column */}
          <div className="flex flex-col gap-4">
            {sensorData ? (
              <SensorGrid data={sensorData} thresholds={thresholds} />
            ) : (
              <div className="grid grid-cols-2 sm:grid-cols-3 gap-3">
                {Array.from({ length: 6 }).map((_, i) => (
                  <div
                    key={i}
                    className="h-28 bg-muted/40 animate-pulse rounded-none border border-border"
                  />
                ))}
              </div>
            )}

            <PresenceBar
              present={present}
              sittingMinutes={sittingMinutes}
              thresholdMinutes={thresholds.sitMinutes}
            />

            <TelemetryChart nodeId={nodeId} />
          </div>

          {/* Right column */}
          <div className="flex flex-col gap-3">
            {deviceLoading ? (
              <div className="h-32 bg-muted/40 animate-pulse border border-border" />
            ) : device ? (
              <DeviceInfoPanel
                name={device.name}
                firmware={device.fw_version ?? "—"}
                ip={device.ip ?? "—"}
                lastSeenLabel={formatLastSeen(device.last_seen_at)}
                online={online}
              />
            ) : null}

            {co2Preheating && (
              <SuggestionCard message="CO₂ sensor warming up — readings not yet reliable" />
            )}

            {!co2Preheating && co2Elevated && (
              <SuggestionCard message="Open a window — CO₂ is elevated" />
            )}

            {!telemetry && (
              <p className="text-xs text-muted-foreground text-center py-2">
                Waiting for telemetry…
              </p>
            )}

            <Button
              variant="outline"
              size="sm"
              className="justify-start gap-2"
              asChild
            >
              <Link href="/thresholds">
                <GearIcon size={14} />
                Thresholds
              </Link>
            </Button>
            <Button
              variant="outline"
              size="sm"
              className="justify-start gap-2"
              asChild
            >
              <Link href="/devices">
                <MonitorIcon size={14} />
                Devices
              </Link>
            </Button>
          </div>
        </div>
      </main>
    </div>
  );
}
