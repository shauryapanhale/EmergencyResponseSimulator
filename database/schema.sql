-- Reference copy of the schema. The authoritative copy is embedded in src/repositories/Schema.cpp.
CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER NOT NULL
);
INSERT INTO schema_version (version)
    SELECT 1 WHERE NOT EXISTS (SELECT 1 FROM schema_version);

CREATE TABLE IF NOT EXISTS city_node (
    node_id   INTEGER PRIMARY KEY,
    name      TEXT NOT NULL,
    x         REAL NOT NULL,
    y         REAL NOT NULL,
    node_type TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS road (
    road_id          INTEGER PRIMARY KEY,
    from_node        INTEGER NOT NULL REFERENCES city_node(node_id),
    to_node          INTEGER NOT NULL REFERENCES city_node(node_id),
    distance_km      REAL NOT NULL CHECK (distance_km > 0),
    base_minutes     REAL NOT NULL CHECK (base_minutes > 0),
    traffic_factor   REAL NOT NULL DEFAULT 1.0 CHECK (traffic_factor > 0),
    is_blocked       INTEGER NOT NULL DEFAULT 0 CHECK (is_blocked IN (0, 1)),
    is_bidirectional INTEGER NOT NULL DEFAULT 1 CHECK (is_bidirectional IN (0, 1))
);

CREATE TABLE IF NOT EXISTS station (
    station_id   INTEGER PRIMARY KEY,
    name         TEXT NOT NULL,
    node_id      INTEGER NOT NULL REFERENCES city_node(node_id),
    station_type TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS vehicle (
    vehicle_id   INTEGER PRIMARY KEY,
    type         TEXT NOT NULL,
    station_id   INTEGER NOT NULL REFERENCES station(station_id),
    current_node INTEGER NOT NULL REFERENCES city_node(node_id),
    speed_kmph   REAL NOT NULL CHECK (speed_kmph > 0),
    status       TEXT NOT NULL,
    capabilities TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS simulation_run (
    run_id        INTEGER PRIMARY KEY AUTOINCREMENT,
    scenario_name TEXT NOT NULL,
    start_time    REAL NOT NULL DEFAULT 0,
    end_time      REAL,
    settings_text TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS incident (
    run_id      INTEGER NOT NULL REFERENCES simulation_run(run_id),
    incident_id INTEGER NOT NULL,
    type        TEXT NOT NULL,
    priority    TEXT NOT NULL,
    node_id     INTEGER NOT NULL REFERENCES city_node(node_id),
    description TEXT NOT NULL DEFAULT '',
    created_at  REAL NOT NULL,
    status      TEXT NOT NULL,
    PRIMARY KEY (run_id, incident_id)
);

CREATE TABLE IF NOT EXISTS dispatch (
    run_id        INTEGER NOT NULL,
    dispatch_id   INTEGER NOT NULL,
    incident_id   INTEGER NOT NULL,
    vehicle_id    INTEGER NOT NULL REFERENCES vehicle(vehicle_id),
    route_text    TEXT NOT NULL DEFAULT '',
    dispatched_at REAL NOT NULL,
    arrived_at    REAL,
    completed_at  REAL,
    eta_minutes   REAL NOT NULL CHECK (eta_minutes >= 0),
    outcome       TEXT NOT NULL DEFAULT '',
    PRIMARY KEY (run_id, dispatch_id),
    FOREIGN KEY (run_id, incident_id) REFERENCES incident(run_id, incident_id)
);

CREATE TABLE IF NOT EXISTS event_log (
    event_id    INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id      INTEGER NOT NULL REFERENCES simulation_run(run_id),
    sim_time    REAL NOT NULL,
    entity_type TEXT NOT NULL,
    entity_id   INTEGER,
    event_type  TEXT NOT NULL,
    details     TEXT NOT NULL DEFAULT ''
);