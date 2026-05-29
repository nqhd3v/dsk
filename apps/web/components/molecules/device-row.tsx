import * as React from "react";
import { CpuIcon, CaretRightIcon } from "@phosphor-icons/react";
import { cn } from "@/lib/utils";
import { Button } from "@/components/atoms/button";

export type DeviceStatus = "active" | "offline" | "pending";

interface DeviceRowProps {
  nodeId: string;
  name: string;
  status: DeviceStatus;
  ip: string;
  firmware: string;
  lastSeenLabel: string;
  temperature?: number;
  present?: boolean;
  onApprove?: () => void;
  onReject?: () => void;
  onClick?: () => void;
  className?: string;
}

const statusStyle: Record<
  DeviceStatus,
  { dot: string; badge: string; label: string }
> = {
  active: {
    dot: "bg-emerald-400",
    badge: "bg-emerald-500/15 text-emerald-400 border-emerald-500/30",
    label: "active",
  },
  offline: {
    dot: "bg-muted-foreground",
    badge: "bg-muted text-muted-foreground border-border",
    label: "offline",
  },
  pending: {
    dot: "bg-amber-400",
    badge: "bg-amber-500/15 text-amber-400 border-amber-500/30",
    label: "pending",
  },
};

export function DeviceRow({
  nodeId,
  name,
  status,
  ip,
  firmware,
  lastSeenLabel,
  temperature,
  present,
  onApprove,
  onReject,
  onClick,
  className,
}: DeviceRowProps) {
  const s = statusStyle[status];

  return (
    <div
      className={cn(
        "flex items-center gap-3 p-4 border-b border-border last:border-0 hover:bg-muted/30 transition-colors cursor-pointer",
        className,
      )}
      onClick={onClick}
    >
      {/* Icon */}
      <div
        className={cn(
          "flex items-center justify-center size-10 rounded-none border border-border bg-muted/50",
          status === "pending" && "border-amber-500/40 bg-amber-500/10",
        )}
      >
        <CpuIcon
          size={20}
          className={
            status === "active"
              ? "text-emerald-400"
              : status === "pending"
                ? "text-amber-400"
                : "text-muted-foreground"
          }
        />
      </div>

      {/* Info */}
      <div className="flex-1 min-w-0">
        <div className="flex items-center gap-2">
          <span className="text-sm font-medium">{name}</span>
          {status !== "active" && (
            <span
              className={cn(
                "text-[10px] px-1.5 py-0.5 border rounded-none",
                s.badge,
              )}
            >
              {s.label}
            </span>
          )}
        </div>
        <p className="text-xs text-muted-foreground truncate">
          {nodeId} · {ip} · fw {firmware} · seen {lastSeenLabel}
        </p>
      </div>

      {/* Right side */}
      {status === "pending" ? (
        <div className="flex gap-2" onClick={(e) => e.stopPropagation()}>
          <Button
            size="sm"
            onClick={onApprove}
            className="bg-emerald-600 hover:bg-emerald-500 text-white border-0"
          >
            Approve
          </Button>
          <Button size="sm" variant="destructive" onClick={onReject}>
            Reject
          </Button>
        </div>
      ) : (
        <div className="flex items-center gap-3 text-xs text-muted-foreground">
          {temperature !== undefined && (
            <span className="font-medium text-foreground">{temperature}°C</span>
          )}
          {present !== undefined && (
            <span
              className={present ? "text-emerald-400" : "text-muted-foreground"}
            >
              {present ? "Present" : "Away"}
            </span>
          )}
          <CaretRightIcon size={14} />
        </div>
      )}
    </div>
  );
}
