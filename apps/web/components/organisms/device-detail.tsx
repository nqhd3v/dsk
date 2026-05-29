import * as React from "react";
import {
  ArrowLeftIcon,
  GearIcon,
  ChartBarIcon,
  ArrowsClockwiseIcon,
  TrashIcon,
} from "@phosphor-icons/react";
import { Card } from "@/components/atoms/card";
import { Button } from "@/components/atoms/button";
import { Input } from "@/components/atoms/input";
import { Separator } from "@/components/atoms/separator";
import { cn } from "@/lib/utils";

export interface DeviceDetailData {
  nodeId: string;
  name: string;
  status: "active" | "offline" | "pending";
  ip: string;
  mac: string;
  firmware: string;
  lastSeenLabel: string;
  registeredLabel: string;
  latestReading?: {
    temperature: number;
    humidity: number;
    co2: number;
    lux: number;
    vocKOhm: number;
    pressure: number;
  };
}

interface DeviceDetailProps {
  device: DeviceDetailData;
  onBack?: () => void;
  onThresholds?: () => void;
  onDashboard?: () => void;
  onRestart?: () => void;
  onRemove?: () => void;
  onRename?: (name: string) => void;
}

const statusStyle = {
  active: "bg-emerald-500/15 text-emerald-400 border-emerald-500/30",
  offline: "bg-muted text-muted-foreground border-border",
  pending: "bg-amber-500/15 text-amber-400 border-amber-500/30",
};

export function DeviceDetail({
  device,
  onBack,
  onThresholds,
  onDashboard,
  onRestart,
  onRemove,
  onRename,
}: DeviceDetailProps) {
  const [nameVal, setNameVal] = React.useState(device.name);

  return (
    <div className="flex flex-col gap-4">
      {/* Header */}
      <div className="flex items-center gap-3">
        <Button variant="ghost" size="icon-sm" onClick={onBack}>
          <ArrowLeftIcon size={16} />
        </Button>
        <span className="font-medium">{device.name}</span>
        <span
          className={cn(
            "text-[10px] px-1.5 py-0.5 border rounded-none",
            statusStyle[device.status],
          )}
        >
          {device.status}
        </span>
      </div>

      <div className="grid grid-cols-3 gap-3">
        {/* Left: info grid */}
        <div className="col-span-2 grid grid-cols-2 gap-3">
          <InfoCard label="Node ID" value={device.nodeId} />
          <InfoCard label="Firmware" value={device.firmware} />
          <InfoCard label="IP address" value={device.ip} />
          <InfoCard label="MAC address" value={device.mac} />
          <InfoCard label="Last seen" value={device.lastSeenLabel} />
          <InfoCard label="Registered" value={device.registeredLabel} />
        </div>

        {/* Right: actions */}
        <div className="flex flex-col gap-2">
          <Button
            variant="outline"
            size="sm"
            className="justify-start gap-2"
            onClick={onThresholds}
          >
            <GearIcon size={14} />
            Thresholds
          </Button>
          <Button
            variant="outline"
            size="sm"
            className="justify-start gap-2"
            onClick={onDashboard}
          >
            <ChartBarIcon size={14} />
            View dashboard
          </Button>
          <Button
            variant="outline"
            size="sm"
            className="justify-start gap-2"
            onClick={onRestart}
          >
            <ArrowsClockwiseIcon size={14} />
            Restart device
          </Button>

          <div className="mt-2">
            <p className="text-xs text-destructive font-medium mb-1">
              Danger zone
            </p>
            <Button
              variant="destructive"
              size="sm"
              className="w-full justify-start gap-2"
              onClick={onRemove}
            >
              <TrashIcon size={14} />
              Remove device
            </Button>
            <p className="text-[10px] text-muted-foreground mt-1.5 leading-snug">
              Revokes MQTT creds, wipes NVS, keeps telemetry history.
            </p>
          </div>
        </div>
      </div>

      {/* Rename */}
      <Card className="p-4 flex flex-col gap-2">
        <span className="text-xs text-muted-foreground">Display name</span>
        <div className="flex gap-2">
          <Input
            value={nameVal}
            onChange={(e) => setNameVal(e.target.value)}
            className="flex-1"
          />
          <Button size="sm" onClick={() => onRename?.(nameVal)}>
            Rename
          </Button>
        </div>
      </Card>

      {/* Latest reading */}
      {device.latestReading && (
        <Card className="p-4 flex flex-col gap-3">
          <div className="flex items-center gap-1.5 text-xs text-muted-foreground">
            <ChartBarIcon size={14} />
            <span>Latest reading</span>
          </div>
          <Separator />
          <div className="grid grid-cols-3 gap-x-6 gap-y-1 text-xs">
            <ReadingRow
              label="Temp"
              value={`${device.latestReading.temperature}°C`}
            />
            <ReadingRow
              label="Humid"
              value={`${device.latestReading.humidity}%`}
            />
            <ReadingRow label="CO₂" value={`${device.latestReading.co2} ppm`} />
            <ReadingRow
              label="Light"
              value={`${device.latestReading.lux} lx`}
            />
            <ReadingRow
              label="Air"
              value={`${device.latestReading.vocKOhm}k Ω`}
            />
            <ReadingRow
              label="Press"
              value={`${device.latestReading.pressure} hPa`}
            />
          </div>
        </Card>
      )}
    </div>
  );
}

function InfoCard({ label, value }: { label: string; value: string }) {
  return (
    <Card className="p-3 flex flex-col gap-1">
      <span className="text-[10px] text-muted-foreground">{label}</span>
      <span className="text-sm font-medium">{value}</span>
    </Card>
  );
}

function ReadingRow({ label, value }: { label: string; value: string }) {
  return (
    <div className="flex gap-1.5">
      <span className="text-muted-foreground">{label}</span>
      <span className="font-medium">{value}</span>
    </div>
  );
}
