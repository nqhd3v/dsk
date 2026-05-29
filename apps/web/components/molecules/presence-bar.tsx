import * as React from "react";
import { UserIcon } from "@phosphor-icons/react";
import { cn } from "@/lib/utils";
import { Card } from "@/components/atoms/card";
import { Badge } from "@/components/atoms/badge";

interface PresenceBarProps {
  present: boolean;
  sittingMinutes: number;
  thresholdMinutes: number;
  className?: string;
}

export function PresenceBar({
  present,
  sittingMinutes,
  thresholdMinutes,
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

      <div className="flex items-end gap-2">
        <span className="text-3xl font-bold">{sittingMinutes}</span>
        <span className="text-muted-foreground text-sm mb-1">min sitting</span>
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
