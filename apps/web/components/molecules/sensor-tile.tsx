import * as React from "react"
import { cn } from "@/lib/utils"
import { Card } from "@/components/atoms/card"

export type SensorStatus = "normal" | "warning" | "alert" | "good"

interface SensorTileProps {
  icon: React.ReactNode
  label: string
  value: string | number
  unit?: string
  subtext?: string
  status?: SensorStatus
  statusLabel?: string
  className?: string
}

const statusColor: Record<SensorStatus, string> = {
  normal: "text-foreground",
  warning: "text-amber-400",
  alert: "text-destructive",
  good: "text-emerald-400",
}

const statusLabelColor: Record<SensorStatus, string> = {
  normal: "text-muted-foreground",
  warning: "text-amber-400",
  alert: "text-destructive",
  good: "text-emerald-400",
}

export function SensorTile({
  icon,
  label,
  value,
  unit,
  subtext,
  status = "normal",
  statusLabel,
  className,
}: SensorTileProps) {
  return (
    <Card className={cn("flex flex-col gap-1 p-4", className)}>
      <div className="flex items-center gap-1.5 text-xs text-muted-foreground">
        {icon}
        <span>{label}</span>
      </div>
      <div className={cn("text-3xl font-bold tracking-tight", statusColor[status])}>
        {value}
        {unit && <span className="text-lg font-normal ml-0.5">{unit}</span>}
      </div>
      {statusLabel ? (
        <span className={cn("text-xs", statusLabelColor[status])}>
          {statusLabel}
        </span>
      ) : subtext ? (
        <span className="text-xs text-muted-foreground">{subtext}</span>
      ) : null}
    </Card>
  )
}
