import { cn } from "@/lib/utils";
import { CpuIcon, ShieldCheckIcon } from "@phosphor-icons/react";
import Link from "next/link";

interface AppHeaderProps {
  nodeId?: string;
  online?: boolean;
}

export function AppHeader({ nodeId, online }: AppHeaderProps) {
  return (
    <header className="flex items-center justify-between px-5 py-3 border-b border-border">
      <Link href="/" className="flex items-center gap-2 text-sm font-semibold">
        <ShieldCheckIcon size={18} className="text-primary" />
        <span>Desk Guardian</span>
      </Link>
      {nodeId && (
        <div className="flex items-center gap-2 text-xs text-muted-foreground">
          <CpuIcon size={14} />
          <span>{nodeId}</span>
          <span
            className={cn(
              "size-2 rounded-full",
              online ? "bg-emerald-400" : "bg-muted-foreground",
            )}
          />
        </div>
      )}
    </header>
  );
}
