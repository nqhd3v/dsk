"use client";

import * as React from "react";
import { ArrowLeftIcon, CheckIcon, WarningIcon } from "@phosphor-icons/react";
import Link from "next/link";
import { AppHeader } from "@/components/organisms/app-header";
import {
  ThresholdsForm,
  type ThresholdsFormHandle,
  THRESHOLD_DEFAULTS,
} from "@/components/organisms/thresholds-form";
import { Button } from "@/components/atoms/button";
import { useDevice } from "@/hooks/use-device";
import { useDeviceConfig } from "@/hooks/use-device-config";
import { cn } from "@/lib/utils";

const NODE_ID = "node1";

export default function ThresholdsPage() {
  const { device, loading: deviceLoading } = useDevice(NODE_ID);
  const { push, saving, error } = useDeviceConfig();

  const formRef = React.useRef<ThresholdsFormHandle>(null);
  const [dirty, setDirty] = React.useState(false);
  const [saved, setSaved] = React.useState(false);

  async function handleSave() {
    if (!device || !formRef.current) return;
    const vals = formRef.current.getValues();
    await push(device.id, {
      sit_minutes: vals.sitMinutes,
      co2_max_ppm: vals.co2Max,
      lux_min: vals.luxMin,
      lux_max: vals.luxMax,
    });
    setDirty(false);
    setSaved(true);
    setTimeout(() => setSaved(false), 3000);
  }

  const online = device?.status === "active";
  const canSave = dirty && !saving && !!device;
  console.log(device);

  return (
    <div className="flex flex-col min-h-screen">
      <AppHeader
        nodeId={device ? `${device.node_id} — ${device.name}` : NODE_ID}
        online={online}
      />

      <main className="flex-1 p-5 pb-10 max-w-3xl mx-auto w-full">
        <div className="flex items-center justify-between mb-5">
          <div className="flex items-center gap-2">
            <Button variant="ghost" size="icon-sm" asChild>
              <Link href="/">
                <ArrowLeftIcon size={16} />
              </Link>
            </Button>
            <h1 className="text-base font-semibold">Thresholds</h1>
          </div>

          <div className="flex items-center gap-2">
            {error && (
              <span className="flex items-center gap-1 text-xs text-destructive">
                <WarningIcon size={12} />
                {error}
              </span>
            )}
            {saved && (
              <span className="flex items-center gap-1 text-xs text-emerald-400">
                <CheckIcon size={12} />
                Pushed to device
              </span>
            )}
            <Button
              size="sm"
              onClick={handleSave}
              disabled={!canSave}
              className={cn(saving && "opacity-60 cursor-not-allowed")}
            >
              {saving ? "Saving…" : "Save"}
            </Button>
          </div>
        </div>

        {deviceLoading ? (
          <div className="flex flex-col gap-3">
            {Array.from({ length: 4 }).map((_, i) => (
              <div
                key={i}
                className="h-20 bg-muted/40 animate-pulse border border-border"
              />
            ))}
          </div>
        ) : !device ? (
          <p className="text-sm text-muted-foreground">Device not found.</p>
        ) : (
          <ThresholdsForm
            initial={{
              ...THRESHOLD_DEFAULTS,
              ...(device.cfg_sit_minutes !== null && { sitMinutes: device.cfg_sit_minutes }),
              ...(device.cfg_co2_max_ppm !== null && { co2Max: device.cfg_co2_max_ppm }),
              ...(device.cfg_lux_min !== null && { luxMin: device.cfg_lux_min }),
              ...(device.cfg_lux_max !== null && { luxMax: device.cfg_lux_max }),
            }}
            formRef={formRef}
            onChange={() => {
              setDirty(true);
              setSaved(false);
            }}
          />
        )}
      </main>
    </div>
  );
}
