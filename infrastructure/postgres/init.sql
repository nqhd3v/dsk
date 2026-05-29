-- dsk-guard — Postgres init (runs once on first container start)
-- Requires: timescale/timescaledb image (TimescaleDB extension pre-installed)

-- Enable TimescaleDB
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- ── Device registry ──────────────────────────────────────────────

CREATE TABLE IF NOT EXISTS devices (
    id              UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    node_id         VARCHAR(64)  NOT NULL UNIQUE,
    name            VARCHAR(64)  NOT NULL DEFAULT '',
    status          VARCHAR(16)  NOT NULL DEFAULT 'pending'
                    CHECK (status IN ('pending', 'active', 'offline', 'removed')),
    fw_version      VARCHAR(32),
    mac             VARCHAR(17),
    ip              VARCHAR(45),
    last_seen_at    TIMESTAMPTZ,
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_devices_status ON devices (status);

-- ── Telemetry hypertable ─────────────────────────────────────────
-- One row per sensor reading per node. TimescaleDB chunks by time.

CREATE TABLE IF NOT EXISTS telemetry (
    time            TIMESTAMPTZ  NOT NULL,
    node_id         VARCHAR(64)  NOT NULL,

    -- BH1750
    lux             REAL,

    -- BME680
    temp_c          REAL,
    humidity        REAL,
    pressure_hpa    REAL,
    gas_ohm         INTEGER,

    -- ACD1200
    co2_ppm         INTEGER,
    co2_preheating  BOOLEAN      NOT NULL DEFAULT FALSE,

    -- LD2410S
    presence        VARCHAR(8)   NOT NULL DEFAULT 'ABSENT'
                    CHECK (presence IN ('PRESENT', 'ABSENT')),
    distance_cm     INTEGER,
    sit_seconds     INTEGER      NOT NULL DEFAULT 0,

    -- HCHO (future)
    hcho_ppb        INTEGER,

    -- Metadata
    fw_version      VARCHAR(32)
);

-- Convert to TimescaleDB hypertable (chunk interval = 1 day)
SELECT create_hypertable('telemetry', 'time',
    chunk_time_interval => INTERVAL '1 day',
    if_not_exists => TRUE
);

-- Index for querying by node
CREATE INDEX IF NOT EXISTS idx_telemetry_node_time ON telemetry (node_id, time DESC);

-- ── Retention policy (optional) ──────────────────────────────────
-- Keep 90 days of telemetry, drop older chunks automatically.
-- Uncomment when ready:
-- SELECT add_retention_policy('telemetry', INTERVAL '90 days', if_not_exists => TRUE);

-- ── Continuous aggregate (optional) ──────────────────────────────
-- 5-minute rollups for dashboard charts. Uncomment when needed:
--
-- CREATE MATERIALIZED VIEW telemetry_5m
-- WITH (timescaledb.continuous) AS
-- SELECT
--     time_bucket('5 minutes', time) AS bucket,
--     node_id,
--     AVG(lux) AS avg_lux,
--     AVG(temp_c) AS avg_temp_c,
--     AVG(humidity) AS avg_humidity,
--     AVG(co2_ppm) AS avg_co2_ppm,
--     MAX(sit_seconds) AS max_sit_seconds
-- FROM telemetry
-- GROUP BY bucket, node_id
-- WITH NO DATA;
--
-- SELECT add_continuous_aggregate_policy('telemetry_5m',
--     start_offset => INTERVAL '1 hour',
--     end_offset   => INTERVAL '5 minutes',
--     schedule_interval => INTERVAL '5 minutes'
-- );
