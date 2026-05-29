"use client";

import { Slider } from "@/components/atoms/slider";
import { cn } from "@/lib/utils";
import * as React from "react";

interface ThresholdSliderProps {
  icon: React.ReactNode;
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  formatValue?: (v: number) => string;
  minLabel?: string;
  maxLabel?: string;
  hint?: string;
  onChange?: (value: number) => void;
  className?: string;
}

export function ThresholdSlider({
  icon,
  label,
  value,
  min,
  max,
  step = 1,
  formatValue,
  minLabel,
  maxLabel,
  hint,
  onChange,
  className,
}: ThresholdSliderProps) {
  const fmt = formatValue ?? ((v: number) => String(v));

  return (
    <div
      className={cn(
        "flex flex-col gap-3 p-4 border border-border rounded-none",
        className,
      )}
    >
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-1.5 text-sm">
          {icon}
          <span>{label}</span>
        </div>
        <span className="text-sm font-medium tabular-nums">{fmt(value)}</span>
      </div>
      <Slider
        min={min}
        max={max}
        step={step}
        value={[value]}
        onValueChange={([v]) => onChange?.(v)}
      />
      <div className="flex justify-between text-xs text-muted-foreground">
        <span>{minLabel ?? fmt(min)}</span>
        {hint && <span>{hint}</span>}
        <span>{maxLabel ?? fmt(max)}</span>
      </div>
    </div>
  );
}

interface ThresholdRangeSliderProps {
  icon: React.ReactNode;
  label: string;
  low: number;
  high: number;
  min: number;
  max: number;
  step?: number;
  formatValue?: (v: number) => string;
  onChange?: (low: number, high: number) => void;
  className?: string;
}

export function ThresholdRangeSlider({
  icon,
  label,
  low,
  high,
  min,
  max,
  step = 1,
  formatValue,
  onChange,
  className,
}: ThresholdRangeSliderProps) {
  const fmt = formatValue ?? ((v: number) => String(v));

  return (
    <div
      className={cn(
        "flex flex-col gap-3 p-4 border border-border rounded-none",
        className,
      )}
    >
      <div className="flex items-center gap-1.5 text-sm mb-1">
        {icon}
        <span>{label}</span>
      </div>
      <div className="flex items-center justify-between text-xs text-muted-foreground">
        <span>Low</span>
        <span className="font-medium text-foreground">{fmt(low)}</span>
      </div>
      <Slider
        min={min}
        max={max}
        step={step}
        value={[low, high]}
        onValueChange={([l, h]) => onChange?.(l, h)}
      />
      <div className="flex items-center justify-between text-xs text-muted-foreground">
        <span>High</span>
        <span className="font-medium text-foreground">{fmt(high)}</span>
      </div>
    </div>
  );
}
