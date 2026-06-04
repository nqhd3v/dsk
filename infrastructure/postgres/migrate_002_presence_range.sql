-- Migration 002: add radar presence-range config column to devices table
-- Run once against an existing DB.

ALTER TABLE devices
  ADD COLUMN IF NOT EXISTS cfg_presence_range_cm INTEGER;
