import { Entity, Column, PrimaryColumn } from 'typeorm';

@Entity('telemetry')
export class TelemetryEntity {
  @PrimaryColumn({ type: 'timestamptz' })
  time!: Date;

  @Column({ type: 'varchar', length: 64 })
  node_id!: string;

  // BH1750
  @Column({ type: 'real', nullable: true })
  lux!: number | null;

  // BME680
  @Column({ type: 'real', nullable: true })
  temp_c!: number | null;

  @Column({ type: 'real', nullable: true })
  humidity!: number | null;

  @Column({ type: 'real', nullable: true })
  pressure_hpa!: number | null;

  @Column({ type: 'int', nullable: true })
  gas_ohm!: number | null;

  // ACD1200
  @Column({ type: 'int', nullable: true })
  co2_ppm!: number | null;

  @Column({ type: 'boolean', default: false })
  co2_preheating!: boolean;

  // LD2410S
  @Column({ type: 'varchar', length: 8, default: 'ABSENT' })
  presence!: 'PRESENT' | 'ABSENT';

  @Column({ type: 'int', nullable: true })
  distance_cm!: number | null;

  // Raw nearest-target distance — always recorded, ignores presence range
  @Column({ type: 'int', nullable: true })
  radar_nearest_cm!: number | null;

  @Column({ type: 'int', default: 0 })
  sit_seconds!: number;

  // HCHO (future)
  @Column({ type: 'int', nullable: true })
  hcho_ppb!: number | null;

  // Metadata
  @Column({ type: 'varchar', length: 32, nullable: true })
  fw_version!: string | null;
}
