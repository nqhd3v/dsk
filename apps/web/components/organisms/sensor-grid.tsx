import {
  SensorTile,
  type SensorStatus,
} from "@/components/molecules/sensor-tile";
import {
  CloudIcon,
  DropIcon,
  GaugeIcon,
  SunIcon,
  ThermometerIcon,
  WindIcon,
} from "@phosphor-icons/react";

interface SensorData {
  temperature: number;
  humidity: number;
  co2: number;
  lux: number;
  vocResistance: number; // kΩ — higher = cleaner air
  pressure: number;
}

interface Thresholds {
  tempMin: number;
  tempMax: number;
  humidMin: number;
  humidMax: number;
  co2Max: number;
  luxMin: number;
  luxMax: number;
  vocMin: number; // kΩ below = poor air
}

function co2Status(v: number, max: number): SensorStatus {
  if (v > max) return "warning";
  return "normal";
}

function rangeStatus(v: number, min: number, max: number): SensorStatus {
  if (v < min || v > max) return "warning";
  return "normal";
}

function vocStatus(kOhm: number, min: number): SensorStatus {
  return kOhm < min ? "warning" : "good";
}

interface SensorGridProps {
  data: SensorData;
  thresholds: Thresholds;
}

export function SensorGrid({ data, thresholds }: SensorGridProps) {
  const tempSt = rangeStatus(
    data.temperature,
    thresholds.tempMin,
    thresholds.tempMax,
  );
  const humidSt = rangeStatus(
    data.humidity,
    thresholds.humidMin,
    thresholds.humidMax,
  );
  const co2St = co2Status(data.co2, thresholds.co2Max);
  const luxSt = rangeStatus(data.lux, thresholds.luxMin, thresholds.luxMax);
  const vocSt = vocStatus(data.vocResistance, thresholds.vocMin);
  const vocLabel = vocSt === "good" ? "good" : "poor air";

  return (
    <div className="grid grid-cols-3 gap-3">
      <SensorTile
        icon={<ThermometerIcon size={14} />}
        label="Temperature"
        value={data.temperature.toFixed(1)}
        unit="°C"
        subtext={`${thresholds.tempMin}–${thresholds.tempMax} normal`}
        status={tempSt}
      />
      <SensorTile
        icon={<DropIcon size={14} />}
        label="Humidity"
        value={data.humidity.toFixed(0)}
        unit="%"
        subtext={`${thresholds.humidMin}–${thresholds.humidMax} normal`}
        status={humidSt}
      />
      <SensorTile
        icon={<CloudIcon size={14} />}
        label="CO₂"
        value={data.co2.toFixed(0)}
        unit=" ppm"
        status={co2St}
        statusLabel={co2St === "warning" ? "⚠ ventilate" : undefined}
        subtext={co2St === "normal" ? `< ${thresholds.co2Max} ok` : undefined}
      />
      <SensorTile
        icon={<SunIcon size={14} />}
        label="Light"
        value={data.lux.toFixed(0)}
        unit=" lx"
        subtext={`${thresholds.luxMin}–${thresholds.luxMax} ideal`}
        status={luxSt}
      />
      <SensorTile
        icon={<WindIcon size={14} />}
        label="Air quality"
        value={`${(data.vocResistance / 1000).toFixed(0)}k`}
        unit=" Ω"
        statusLabel={vocLabel}
        status={vocSt}
      />
      <SensorTile
        icon={<GaugeIcon size={14} />}
        label="Pressure"
        value={data.pressure.toFixed(0)}
        unit=" hPa"
        subtext="standard"
        status="normal"
      />
    </div>
  );
}
