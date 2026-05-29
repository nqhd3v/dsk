"use client";

import * as React from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";
import { ChartLineIcon } from "@phosphor-icons/react";
import { Card } from "@/components/atoms/card";
import { useTelemetryHistory } from "@/hooks/use-telemetry-history";
import { useTelemetry } from "@/hooks/use-socket";
import type { EnvTelemetry } from "@dsk/schemas";

const MAX_POINTS = 360; // 1h at 10s interval

interface ChartPoint {
  ts: number;
  time: string;
  temp: number | null;
  humidity: number | null;
  co2: number | null;
}

function toPoint(row: EnvTelemetry): ChartPoint {
  const d = new Date(row.ts * 1000);
  const time = d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
  return {
    ts: row.ts,
    time,
    temp: row.temp_c,
    humidity: row.humidity,
    co2: row.co2_ppm,
  };
}

interface TelemetryChartProps {
  nodeId: string;
}

export function TelemetryChart({ nodeId }: TelemetryChartProps) {
  const { rows: history, loading } = useTelemetryHistory(nodeId, MAX_POINTS);
  const live = useTelemetry(nodeId);

  // Seed points from history; append live updates
  const [points, setPoints] = React.useState<ChartPoint[]>([]);

  React.useEffect(() => {
    if (!loading) {
      // eslint-disable-next-line react-hooks/set-state-in-effect
      setPoints(history.map(toPoint));
    }
  }, [history, loading]);

  React.useEffect(() => {
    if (!live) return;
    // eslint-disable-next-line react-hooks/set-state-in-effect
    setPoints((prev) => {
      // Deduplicate by ts
      if (prev.length > 0 && prev[prev.length - 1].ts === live.ts) return prev;
      const next = [...prev, toPoint(live)];
      return next.length > MAX_POINTS ? next.slice(-MAX_POINTS) : next;
    });
  }, [live]);

  // Pick tick every ~10 min (60 points) so X axis isn't crowded
  const tickPoints = React.useMemo(() => {
    if (points.length === 0) return new Set<number>();
    const interval = Math.max(1, Math.floor(points.length / 6));
    return new Set(
      points.filter((_, i) => i % interval === 0).map((p) => p.ts),
    );
  }, [points]);

  return (
    <Card className="p-4 flex flex-col gap-3">
      <div className="flex items-center gap-1.5 text-xs text-muted-foreground">
        <ChartLineIcon size={14} />
        <span>Telemetry history (1 h)</span>
      </div>

      {loading ? (
        <div className="h-44 bg-muted/40 animate-pulse border border-border border-dashed" />
      ) : points.length === 0 ? (
        <div className="flex items-center justify-center h-44 border border-border border-dashed text-xs text-muted-foreground">
          No data yet
        </div>
      ) : (
        <ResponsiveContainer width="100%" height={176}>
          <LineChart
            data={points}
            margin={{ top: 4, right: 8, left: -16, bottom: 0 }}
          >
            <CartesianGrid strokeDasharray="3 3" stroke="var(--border)" />
            <XAxis
              dataKey="ts"
              type="number"
              domain={["dataMin", "dataMax"]}
              tickFormatter={(ts: number) => {
                const d = new Date(ts * 1000);
                return d.toLocaleTimeString([], {
                  hour: "2-digit",
                  minute: "2-digit",
                });
              }}
              tick={{ fontSize: 10 }}
              ticks={[...tickPoints]}
              stroke="var(--muted-foreground)"
            />
            {/* Left Y: temp + humidity */}
            <YAxis
              yAxisId="env"
              domain={[0, 100]}
              tick={{ fontSize: 10 }}
              stroke="var(--muted-foreground)"
            />
            {/* Right Y: CO2 ppm */}
            <YAxis
              yAxisId="co2"
              orientation="right"
              domain={[300, "auto"]}
              tick={{ fontSize: 10 }}
              stroke="var(--muted-foreground)"
            />
            <Tooltip
              contentStyle={{
                fontSize: 11,
                background: "var(--card)",
                border: "1px solid var(--border)",
                borderRadius: 0,
              }}
              labelFormatter={(ts: number) =>
                new Date(ts * 1000).toLocaleTimeString()
              }
              formatter={(value: number, name: string) => {
                if (name === "temp") return [`${value} °C`, "Temp"];
                if (name === "humidity") return [`${value} %`, "Humidity"];
                if (name === "co2") return [`${value} ppm`, "CO₂"];
                return [value, name];
              }}
            />
            <Legend
              iconType="plainline"
              wrapperStyle={{ fontSize: 11 }}
              formatter={(v) =>
                v === "temp"
                  ? "Temp (°C)"
                  : v === "humidity"
                    ? "Humidity (%)"
                    : "CO₂ (ppm)"
              }
            />
            <Line
              yAxisId="env"
              type="monotone"
              dataKey="temp"
              stroke="#f97316"
              dot={false}
              strokeWidth={1.5}
              connectNulls
            />
            <Line
              yAxisId="env"
              type="monotone"
              dataKey="humidity"
              stroke="#3b82f6"
              dot={false}
              strokeWidth={1.5}
              connectNulls
            />
            <Line
              yAxisId="co2"
              type="monotone"
              dataKey="co2"
              stroke="#a855f7"
              dot={false}
              strokeWidth={1.5}
              connectNulls
            />
          </LineChart>
        </ResponsiveContainer>
      )}
    </Card>
  );
}
