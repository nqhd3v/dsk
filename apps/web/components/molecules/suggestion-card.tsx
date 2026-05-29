import * as React from "react";
import { SunHorizonIcon } from "@phosphor-icons/react";
import { cn } from "@/lib/utils";

interface SuggestionCardProps {
  message: string;
  className?: string;
}

export function SuggestionCard({ message, className }: SuggestionCardProps) {
  return (
    <div
      className={cn(
        "rounded-none border border-amber-500/30 bg-amber-500/10 px-3 py-2 flex items-center gap-2",
        className,
      )}
    >
      <SunHorizonIcon size={12} className="text-amber-400 shrink-0" />
      <p className="text-xs text-amber-300">{message}</p>
    </div>
  );
}
