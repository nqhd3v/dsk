-- Migration 003: add raw nearest-target distance column to telemetry table
-- Always-recorded radar distance (ignores presence range gate).
-- Run once against an existing DB.

ALTER TABLE telemetry
  ADD COLUMN IF NOT EXISTS radar_nearest_cm INTEGER;
