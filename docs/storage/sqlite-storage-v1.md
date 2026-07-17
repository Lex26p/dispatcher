# SQLite storage v1

## Summary

SQLite storage v1 introduces the first persistent storage backend for Dispatcher.

The storage layer is implemented as a separate backend library:

```text id="elx91o"
backend/libs/dispatcher-storage-sqlite
```

The library provides a small SQLite wrapper, a migration runner, repositories for core persisted data, and an integration context that opens one database and exposes all repositories.

This version does not yet connect SQLite storage to the live runtime pipeline. It provides the persistent storage foundation that future stages can connect to runtime, alarm, history, configuration, audit, and frontend APIs.

## Library

Main library:

```text id="u36rj3"
dispatcher-storage-sqlite
```

Location:

```text id="q5iv6q"
backend/libs/dispatcher-storage-sqlite
```

Main umbrella header:

```text id="a2wpv4"
#include <dispatcher/storage/sqlite/sqlite_storage.hpp>
```

The library currently includes:

```text id="zxn4ul"
SqliteDatabase
SqliteError
SqliteMigration
SqliteMigrationRunner
SqliteHistorySample
SqliteHistoryStorage
SqliteAlarmEvent
SqliteAlarmEventStorage
SqliteConfigurationSnapshotRecord
SqliteConfigurationSnapshotStorage
SqliteStorageContext
```

## Dependency

SQLite is provided through vcpkg:

```text id="sv2iki"
sqlite3
```

The CMake target used by the library is:

```text id="ucpecl"
unofficial::sqlite3::sqlite3
```

## SqliteDatabase

`SqliteDatabase` is a small move-only RAII wrapper over `sqlite3*`.

Header:

```text id="bjhzxo"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_database.hpp
```

Implementation:

```text id="lesjny"
backend/libs/dispatcher-storage-sqlite/src/sqlite_database.cpp
```

Supported operations:

```text id="qa5b13"
open_in_memory()
open_file(...)
is_open()
path()
native_handle()
execute(...)
query_int64(...)
close()
```

The wrapper owns the SQLite handle and closes it in the destructor.

`SqliteDatabase` is intentionally non-copyable and movable.

## SqliteError

SQLite-specific failures are represented by:

```text id="wjvbl4"
dispatcher::storage::sqlite::SqliteError
```

Header:

```text id="ghcjqg"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_error.hpp
```

It derives from:

```text id="jl1uzj"
std::runtime_error
```

## Migration foundation

SQLite schema changes are represented by:

```text id="oecq32"
SqliteMigration
```

Header:

```text id="w3vh1y"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_migration.hpp
```

Migration fields:

```text id="o6edn6"
version
name
sql
```

Migrations are applied by:

```text id="j256zq"
SqliteMigrationRunner
```

Header:

```text id="ebja8z"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_migration_runner.hpp
```

Implementation:

```text id="jc1o9p"
backend/libs/dispatcher-storage-sqlite/src/sqlite_migration_runner.cpp
```

The migration runner creates and maintains:

```sql id="ga7rqi"
CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY NOT NULL,
    name TEXT NOT NULL,
    applied_at_utc TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);
```

Supported behavior:

```text id="q5aq9q"
schema_migrations table is created automatically
migration version must be positive
migration name must not be empty
migration SQL must not be empty
migrations are applied in version order
already applied migrations are skipped
each migration is wrapped in a transaction
failed migration is rolled back
migration names are escaped before insert
```

## Current migrations

SQLite storage v1 currently defines three repository migrations:

```text id="u0gk6p"
100 — create_history_samples
200 — create_alarm_events
300 — create_configuration_snapshots
```

When using `SqliteStorageContext::apply_schema()`, all three migrations are applied together.

## History samples

History samples are stored in:

```text id="z7owh6"
history_samples
```

Migration version:

```text id="ut46ck"
100
```

Repository:

```text id="g3e4xk"
SqliteHistoryStorage
```

Headers:

```text id="h88c9e"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_history_sample.hpp
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_history_storage.hpp
```

Implementation:

```text id="t1dxyg"
backend/libs/dispatcher-storage-sqlite/src/sqlite_history_storage.cpp
```

Schema:

```sql id="o54m29"
CREATE TABLE IF NOT EXISTS history_samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    tag_id TEXT NOT NULL,
    timestamp_utc TEXT NOT NULL,
    value_type TEXT NOT NULL,
    numeric_value REAL NULL,
    text_value TEXT NULL,
    bool_value INTEGER NULL,
    quality TEXT NOT NULL,
    source TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS ix_history_samples_tag_time
ON history_samples (tag_id, timestamp_utc DESC, id DESC);
```

Supported value types:

```text id="h8qtfq"
numeric
text
boolean
```

Supported operations:

```text id="fhjtw3"
apply_schema()
append(...)
count()
count_for_tag(...)
latest_for_tag(...)
read_recent_for_tag(...)
```

Validation rules:

```text id="dgko8o"
tag_id must not be empty
timestamp_utc must not be empty
quality must not be empty
source must not be empty
numeric sample requires numeric_value
text sample requires text_value
boolean sample requires bool_value
```

Example:

```cpp id="c189w0"
auto database =
    dispatcher::storage::sqlite::SqliteDatabase::open_file(
        "dispatcher.sqlite"
    );

dispatcher::storage::sqlite::SqliteHistoryStorage history{
    database
};

history.apply_schema();

const auto id =
    history.append(
        dispatcher::storage::sqlite::SqliteHistorySample::numeric(
            "pump.pressure",
            "2026-07-02T10:00:00.000Z",
            12.5
        )
    );
```

## Alarm events

Alarm events are stored in:

```text id="fg9tkx"
alarm_events
```

Migration version:

```text id="wijvdr"
200
```

Repository:

```text id="r3k0h7"
SqliteAlarmEventStorage
```

Headers:

```text id="nc96lu"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_alarm_event.hpp
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_alarm_event_storage.hpp
```

Implementation:

```text id="arxovw"
backend/libs/dispatcher-storage-sqlite/src/sqlite_alarm_event_storage.cpp
```

Schema:

```sql id="t9rxdx"
CREATE TABLE IF NOT EXISTS alarm_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    alarm_id TEXT NOT NULL,
    tag_id TEXT NOT NULL,
    event_type TEXT NOT NULL,
    severity TEXT NOT NULL,
    state TEXT NOT NULL,
    message TEXT NOT NULL,
    timestamp_utc TEXT NOT NULL,
    source TEXT NOT NULL,
    operator_id TEXT NULL,
    comment TEXT NULL
);

CREATE INDEX IF NOT EXISTS ix_alarm_events_alarm_time
ON alarm_events (alarm_id, timestamp_utc DESC, id DESC);

CREATE INDEX IF NOT EXISTS ix_alarm_events_time
ON alarm_events (timestamp_utc DESC, id DESC);

CREATE INDEX IF NOT EXISTS ix_alarm_events_tag_time
ON alarm_events (tag_id, timestamp_utc DESC, id DESC);
```

Supported event factory helpers:

```text id="v3myhf"
SqliteAlarmEvent::raised(...)
SqliteAlarmEvent::cleared(...)
SqliteAlarmEvent::acknowledged(...)
```

Supported operations:

```text id="kvp3gm"
apply_schema()
append(...)
count()
count_for_alarm(...)
latest_for_alarm(...)
read_recent_for_alarm(...)
read_recent(...)
```

Validation rules:

```text id="qpdp5n"
alarm_id must not be empty
tag_id must not be empty
event_type must not be empty
severity must not be empty
state must not be empty
message must not be empty
timestamp_utc must not be empty
source must not be empty
```

Example:

```cpp id="bwot7u"
auto database =
    dispatcher::storage::sqlite::SqliteDatabase::open_file(
        "dispatcher.sqlite"
    );

dispatcher::storage::sqlite::SqliteAlarmEventStorage alarm_events{
    database
};

alarm_events.apply_schema();

const auto id =
    alarm_events.append(
        dispatcher::storage::sqlite::SqliteAlarmEvent::raised(
            "alarm-001",
            "pump.pressure",
            "critical",
            "Pressure is above limit",
            "2026-07-02T10:01:00.000Z"
        )
    );
```

## Configuration snapshots

Configuration snapshots are stored in:

```text id="fu4e4m"
configuration_snapshots
```

Migration version:

```text id="ivpqf1"
300
```

Repository:

```text id="nq7hw9"
SqliteConfigurationSnapshotStorage
```

Headers:

```text id="tb0jjw"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_configuration_snapshot.hpp
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_configuration_snapshot_storage.hpp
```

Implementation:

```text id="g9qtet"
backend/libs/dispatcher-storage-sqlite/src/sqlite_configuration_snapshot_storage.cpp
```

Schema:

```sql id="vwmn1x"
CREATE TABLE IF NOT EXISTS configuration_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    snapshot_name TEXT NOT NULL,
    schema_version TEXT NOT NULL,
    created_at_utc TEXT NOT NULL,
    source TEXT NOT NULL,
    payload_json TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS ix_configuration_snapshots_created
ON configuration_snapshots (created_at_utc DESC, id DESC);
```

Supported operations:

```text id="oh3fh5"
apply_schema()
save(...)
count()
find_by_id(...)
latest()
read_latest(...)
```

Validation rules:

```text id="bzlgu1"
snapshot_name must not be empty
schema_version must not be empty
created_at_utc must not be empty
source must not be empty
payload_json must not be empty
```

Example:

```cpp id="ntn4wk"
auto database =
    dispatcher::storage::sqlite::SqliteDatabase::open_file(
        "dispatcher.sqlite"
    );

dispatcher::storage::sqlite::SqliteConfigurationSnapshotStorage snapshots{
    database
};

snapshots.apply_schema();

const auto id =
    snapshots.save(
        dispatcher::storage::sqlite::SqliteConfigurationSnapshotRecord::create(
            "initial",
            "1",
            "2026-07-02T10:02:00.000Z",
            "{\"devices\":[],\"tags\":[]}"
        )
    );
```

## SqliteStorageContext

`SqliteStorageContext` is the integration entry point for SQLite storage v1.

Header:

```text id="cjeiwq"
backend/libs/dispatcher-storage-sqlite/include/dispatcher/storage/sqlite/sqlite_storage_context.hpp
```

Implementation:

```text id="hj6ss9"
backend/libs/dispatcher-storage-sqlite/src/sqlite_storage_context.cpp
```

Supported operations:

```text id="e888qf"
open_in_memory()
open_file(...)
migrations()
apply_schema()
is_open()
database()
history_storage()
alarm_event_storage()
configuration_snapshot_storage()
```

`SqliteStorageContext::apply_schema()` applies all current SQLite repository migrations:

```text id="kjyjhi"
100 — history_samples
200 — alarm_events
300 — configuration_snapshots
```

Example:

```cpp id="fe9cc5"
auto context =
    dispatcher::storage::sqlite::SqliteStorageContext::open_file(
        "dispatcher.sqlite"
    );

context.apply_schema();

auto history =
    context.history_storage();

auto alarm_events =
    context.alarm_event_storage();

auto configuration_snapshots =
    context.configuration_snapshot_storage();
```

The repositories returned by the context share the same underlying SQLite database.

## Test coverage

SQLite storage tests are part of:

```text id="txlfwq"
dispatcher-storage-sqlite-tests
```

Current test files:

```text id="vt77ps"
tests/unit/sqlite_database_tests.cpp
tests/unit/sqlite_migration_runner_tests.cpp
tests/unit/sqlite_history_storage_tests.cpp
tests/unit/sqlite_alarm_event_storage_tests.cpp
tests/unit/sqlite_configuration_snapshot_storage_tests.cpp
tests/unit/sqlite_storage_context_tests.cpp
```

The test executable covers:

```text id="ryqv2g"
database open and close
in-memory database
file database persistence
SQL execute
scalar query
move construction
move assignment
migration table creation
migration ordering
migration idempotency
migration rollback
history sample persistence
alarm event persistence
configuration snapshot persistence
shared database context integration
```

## Verification commands

Configure:

```powershell id="gzywmu"
cmake --preset windows-vs-debug
```

Build:

```powershell id="jmcjst"
cmake --build --preset windows-vs-debug
```

Run backend tests:

```powershell id="m71bbh"
ctest --preset windows-vs-debug
```

Expected result:

```text id="s390i1"
100% tests passed, 0 tests failed out of 13
```

Frontend build check:

```powershell id="lkb5sw"
dotnet build frontend\Dispatcher.Frontend.slnx
```

E2E smoke:

```powershell id="srgoq9"
.\out\build\windows-vs-debug\backend\apps\dispatcher-e2e-smoke\Debug\dispatcher-e2e-smoke.exe 10000
```

## Known limitations

SQLite storage v1 is a storage foundation. It does not yet provide full production persistence wiring.

Accepted limitations:

```text id="hm9h22"
SQLite repositories are not yet connected to DispatcherRuntime.
Telemetry ingest does not yet automatically write to history_samples.
Alarm engine does not yet automatically write to alarm_events.
Configuration import/export does not yet automatically write configuration_snapshots.
HTTP API does not yet expose SQLite history reads.
HTTP API does not yet expose SQLite alarm event reads.
HTTP API does not yet expose configuration snapshot reads.
No retention policy exists yet.
No database backup command exists yet.
No WAL/bootstrap pragmas are configured yet.
No connection pool exists.
No prepared statement reuse/cache exists.
No schema downgrade mechanism exists.
No OpenAPI contract exists for SQLite-backed endpoints.
```

These limitations are accepted for SQLite storage v1 and should be addressed in later integration and deployment stages.

## Future integration targets

Likely future work:

```text id="n7z4ov"
connect telemetry ingest pipeline to SqliteHistoryStorage
connect alarm transitions to SqliteAlarmEventStorage
connect configuration import/export to SqliteConfigurationSnapshotStorage
add HTTP endpoints for recent history samples
add HTTP endpoints for alarm event timeline
add HTTP endpoints for configuration snapshot list/latest
add runtime configuration for SQLite database path
add startup schema migration through SqliteStorageContext
add operational docs for database location, backup, and retention
```
