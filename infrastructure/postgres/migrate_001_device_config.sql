-- Migration 001: add last-pushed config columns to devices table
-- Run once against an existing DB (init.sql already handles new installs via TypeORM sync)

ALTER TABLE devices
  ADD COLUMN IF NOT EXISTS cfg_sit_minutes  INTEGER,
  ADD COLUMN IF NOT EXISTS cfg_co2_max_ppm  INTEGER,
  ADD COLUMN IF NOT EXISTS cfg_lux_min      REAL,
  ADD COLUMN IF NOT EXISTS cfg_lux_max      REAL;
