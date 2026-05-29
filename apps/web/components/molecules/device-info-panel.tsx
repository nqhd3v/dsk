import { Card } from "@/components/atoms/card";
import { cn } from "@/lib/utils";

interface DeviceInfoPanelProps {
  name: string;
  firmware: string;
  ip: string;
  lastSeenLabel: string;
  online: boolean;
  className?: string;
}

export function DeviceInfoPanel({
  name,
  firmware,
  ip,
  lastSeenLabel,
  online,
  className,
}: DeviceInfoPanelProps) {
  return (
    <Card className={cn("p-4 flex flex-col gap-1.5", className)}>
      <div className="flex items-center justify-between mb-1">
        <span className="text-xs text-muted-foreground">Device</span>
        <span
          className={cn(
            "size-2.5 rounded-full",
            online ? "bg-emerald-400" : "bg-muted-foreground",
          )}
        />
      </div>
      <Row label="Name" value={name} />
      <Row label="FW" value={firmware} />
      <Row label="IP" value={ip} />
      <Row label="Last seen" value={lastSeenLabel} />
    </Card>
  );
}

function Row({ label, value }: { label: string; value: string }) {
  return (
    <div className="flex justify-between text-xs">
      <span className="text-muted-foreground">{label}</span>
      <span className="font-medium">{value}</span>
    </div>
  );
}
