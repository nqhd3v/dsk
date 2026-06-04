import * as React from "react";
import { UserIcon } from "@phosphor-icons/react";
import { cn } from "@/lib/utils";
import { Card } from "@/components/atoms/card";
import { Badge } from "@/components/atoms/badge";

interface PresenceBarProps {
  present: boolean;
  sittingMinutes: number;
  thresholdMinutes: number;
  /** Live distance to nearest target (cm), null when absent */
  distanceCm?: number | null;
  /** Configured presence range (cm) — beyond this = absent */
  rangeCm?: number;
  className?: string;
}

export function PresenceBar({
  present,
  sittingMinutes,
  thresholdMinutes,
  distanceCm,
  rangeCm,
  className,
}: PresenceBarProps) {
  const pct = Math.min((sittingMinutes / thresholdMinutes) * 100, 100);
  const remaining = thresholdMinutes - sittingMinutes;

  return (
    <Card className={cn("p-4 flex flex-col gap-3", className)}>
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-1.5 text-xs text-muted-foreground">
          <UserIcon size={14} />
          <span>Presence</span>
        </div>
        <Badge
          className={cn(
            present
              ? "bg-emerald-500/20 text-emerald-400 border-emerald-500/30"
              : "bg-muted text-muted-foreground border-border",
          )}
        >
          {present ? "Present" : "Away"}
        </Badge>
      </div>

      <div className="flex items-end justify-between gap-2">
        <div className="flex items-end gap-2">
          <span className="text-3xl font-bold">{sittingMinutes}</span>
          <span className="text-muted-foreground text-sm mb-1">min sitting</span>
        </div>
        {/* Live radar distance to nearest target */}
        <div className="flex flex-col items-end mb-1">
          <span
            className={cn(
              "text-lg font-semibold tabular-nums",
              !present && "text-muted-foreground",
            )}
          >
            {distanceCm != null ? `${distanceCm} cm` : "—"}
          </span>
          <span className="text-[10px] text-muted-foreground">
            distance{rangeCm != null ? ` (range ${rangeCm})` : ""}
          </span>
        </div>
      </div>

      {/* Progress bar */}
      <div className="flex items-center gap-3">
        <div className="flex-1 h-1.5 bg-muted rounded-none overflow-hidden">
          <div
            className={cn(
              "h-full transition-all",
              pct >= 100
                ? "bg-destructive"
                : pct >= 80
                  ? "bg-amber-400"
                  : "bg-amber-500",
            )}
            style={{ width: `${pct}%` }}
          />
        </div>
        <span className="text-xs text-muted-foreground shrink-0">
          {thresholdMinutes} min
        </span>
      </div>

      {remaining > 0 && present && (
        <p className="text-xs text-amber-400">
          {remaining} min until stand-up reminder
        </p>
      )}
      {pct >= 100 && (
        <p className="text-xs text-destructive font-medium">
          Time to stand up!
        </p>
      )}
    </Card>
  );
}
