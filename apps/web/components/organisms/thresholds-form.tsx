"use client";

import * as React from "react";
import { useForm, Controller } from "react-hook-form";
import {
  PersonSimpleWalkIcon,
  CloudIcon,
  ThermometerIcon,
  DropIcon,
  SunIcon,
  WindIcon,
  ArrowCounterClockwiseIcon,
  DeviceMobileIcon,
  DesktopIcon,
} from "@phosphor-icons/react";
import {
  ThresholdSlider,
  ThresholdRangeSlider,
} from "@/components/molecules/threshold-slider";
import { cn } from "@/lib/utils";

export interface ThresholdValues {
  // Pushed to device via MQTT cmd/config
  sitMinutes: number;
  co2Max: number;
  luxMin: number;
  luxMax: number;
  // Web-only display thresholds (not in ConfigCmd)
  tempMin: number;
  tempMax: number;
  humidMin: number;
  humidMax: number;
  vocMin: number; // kΩ — below = poor air
}

export const THRESHOLD_DEFAULTS: ThresholdValues = {
  sitMinutes: 45,
  co2Max: 1000,
  luxMin: 200,
  luxMax: 400,
  tempMin: 18,
  tempMax: 28,
  humidMin: 30,
  humidMax: 70,
  vocMin: 30,
};

export interface ThresholdsFormHandle {
  getValues: () => ThresholdValues;
  reset: () => void;
}

interface ThresholdsFormProps {
  initial?: Partial<ThresholdValues>;
  /** Notified on every field change — parent can use for dirty tracking */
  onChange?: (values: ThresholdValues) => void;
  formRef?: React.Ref<ThresholdsFormHandle>;
}

function SectionLabel({
  icon,
  label,
  device,
}: {
  icon: React.ReactNode;
  label: string;
  device: boolean;
}) {
  return (
    <div className="flex items-center justify-between mb-1">
      <div className="flex items-center gap-1.5 text-xs text-muted-foreground font-medium uppercase tracking-wide">
        {icon}
        {label}
      </div>
      <span
        className={cn(
          "text-[10px] px-1.5 py-0.5 border rounded-none flex items-center gap-1",
          device
            ? "border-emerald-500/30 bg-emerald-500/10 text-emerald-400"
            : "border-border bg-muted/30 text-muted-foreground",
        )}
      >
        {device ? (
          <>
            <DeviceMobileIcon size={10} /> pushed to device
          </>
        ) : (
          <>
            <DesktopIcon size={10} /> dashboard only
          </>
        )}
      </span>
    </div>
  );
}

export function ThresholdsForm({
  initial,
  onChange,
  formRef,
}: ThresholdsFormProps) {
  const defaults = { ...THRESHOLD_DEFAULTS, ...initial };

  const { control, reset, getValues, watch } = useForm<ThresholdValues>({
    defaultValues: defaults,
  });

  // Sync if initial prop changes (e.g. after async device fetch)
  React.useEffect(() => {
    if (initial) reset({ ...THRESHOLD_DEFAULTS, ...initial });
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [initial]);

  // Notify parent on every change
  React.useEffect(() => {
    const { unsubscribe } = watch((vals) => {
      onChange?.(vals as ThresholdValues);
    });
    return unsubscribe;
  }, [watch, onChange]);

  // Expose getValues + reset to parent via ref
  React.useImperativeHandle(formRef, () => ({
    getValues,
    reset: () => reset(THRESHOLD_DEFAULTS),
  }));

  // Read luxMin/luxMax live for cross-field slider bounds
  const luxMin = watch("luxMin");
  const luxMax = watch("luxMax");

  return (
    <div className="flex flex-col gap-5">
      {/* ── Device thresholds (pushed via MQTT) ───────────────────── */}
      <div className="flex flex-col gap-3">
        <SectionLabel
          icon={<PersonSimpleWalkIcon size={12} />}
          label="Behavior"
          device
        />

        <Controller
          control={control}
          name="sitMinutes"
          render={({ field }) => (
            <ThresholdSlider
              icon={<PersonSimpleWalkIcon size={16} />}
              label="Stand-up reminder"
              value={field.value}
              min={5}
              max={45}
              step={5}
              minLabel="5 min"
              maxLabel="45 min"
              formatValue={(v) => `${v} min`}
              onChange={field.onChange}
            />
          )}
        />

        <Controller
          control={control}
          name="co2Max"
          render={({ field }) => (
            <ThresholdSlider
              icon={<CloudIcon size={16} />}
              label="CO₂ alert"
              value={field.value}
              min={600}
              max={2000}
              step={50}
              minLabel="600 ppm"
              maxLabel="2000 ppm"
              formatValue={(v) => `${v} ppm`}
              onChange={field.onChange}
            />
          )}
        />

        <Controller
          control={control}
          name="luxMin"
          render={({ field }) => (
            <ThresholdSlider
              icon={<SunIcon size={16} />}
              label="Light — low"
              value={field.value}
              min={0}
              max={luxMax - 10}
              step={10}
              formatValue={(v) => `${v} lx`}
              onChange={field.onChange}
            />
          )}
        />

        <Controller
          control={control}
          name="luxMax"
          render={({ field }) => (
            <ThresholdSlider
              icon={<SunIcon size={16} />}
              label="Light — high"
              value={field.value}
              min={luxMin + 10}
              max={1000}
              step={10}
              formatValue={(v) => `${v} lx`}
              onChange={field.onChange}
            />
          )}
        />
      </div>

      {/* ── Dashboard-only display ranges ─────────────────────────── */}
      <div className="flex flex-col gap-3">
        <SectionLabel
          icon={<DesktopIcon size={12} />}
          label="Display ranges"
          device={false}
        />

        <div className="grid grid-cols-2 gap-3">
          {/* Temp range — two fields, one slider */}
          <Controller
            control={control}
            name="tempMin"
            render={({ field: fieldLow }) => (
              <Controller
                control={control}
                name="tempMax"
                render={({ field: fieldHigh }) => (
                  <ThresholdRangeSlider
                    icon={<ThermometerIcon size={16} />}
                    label="Temp range"
                    low={fieldLow.value}
                    high={fieldHigh.value}
                    min={10}
                    max={40}
                    formatValue={(v) => `${v}°C`}
                    onChange={(l, h) => {
                      fieldLow.onChange(l);
                      fieldHigh.onChange(h);
                    }}
                  />
                )}
              />
            )}
          />

          <Controller
            control={control}
            name="humidMin"
            render={({ field: fieldLow }) => (
              <Controller
                control={control}
                name="humidMax"
                render={({ field: fieldHigh }) => (
                  <ThresholdRangeSlider
                    icon={<DropIcon size={16} />}
                    label="Humidity range"
                    low={fieldLow.value}
                    high={fieldHigh.value}
                    min={10}
                    max={100}
                    formatValue={(v) => `${v}%`}
                    onChange={(l, h) => {
                      fieldLow.onChange(l);
                      fieldHigh.onChange(h);
                    }}
                  />
                )}
              />
            )}
          />

          <Controller
            control={control}
            name="vocMin"
            render={({ field }) => (
              <ThresholdSlider
                icon={<WindIcon size={16} />}
                label="VOC threshold"
                value={field.value}
                min={10}
                max={100}
                step={5}
                minLabel="10k Ω"
                maxLabel="100k Ω"
                hint="Below = poor air"
                formatValue={(v) => `${v}k Ω`}
                onChange={field.onChange}
              />
            )}
          />
        </div>
      </div>

      {/* Reset */}
      <div className="flex justify-end">
        <button
          type="button"
          onClick={() => reset(THRESHOLD_DEFAULTS)}
          className="flex items-center gap-1.5 text-xs text-muted-foreground hover:text-foreground transition-colors"
        >
          <ArrowCounterClockwiseIcon size={12} />
          Reset to defaults
        </button>
      </div>
    </div>
  );
}
