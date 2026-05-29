import { Card } from "@/components/atoms/card";
import {
  DeviceRow,
  type DeviceStatus,
} from "@/components/molecules/device-row";

export interface DeviceEntry {
  nodeId: string;
  name: string;
  status: DeviceStatus;
  ip: string;
  firmware: string;
  lastSeenLabel: string;
  temperature?: number;
  present?: boolean;
}

interface DeviceListProps {
  devices: DeviceEntry[];
  total?: number;
  onApprove?: (nodeId: string) => void;
  onReject?: (nodeId: string) => void;
  onSelect?: (nodeId: string) => void;
}

export function DeviceList({
  devices,
  total,
  onApprove,
  onReject,
  onSelect,
}: DeviceListProps) {
  return (
    <Card className="overflow-hidden p-0">
      <div className="flex items-center gap-2 p-4 border-b border-border">
        <span className="text-sm font-medium">Devices</span>
        {total !== undefined && (
          <span className="text-xs text-muted-foreground">
            {total} registered
          </span>
        )}
      </div>
      {devices.map((d) => (
        <DeviceRow
          key={d.nodeId}
          {...d}
          onApprove={() => onApprove?.(d.nodeId)}
          onReject={() => onReject?.(d.nodeId)}
          onClick={() => onSelect?.(d.nodeId)}
        />
      ))}
    </Card>
  );
}
