# Device Manager

Device Manager is the core backend responsibility for durable device and metric metadata. Runtime metric values remain the responsibility of Data Hub.

## Current implementation stage

`CORE-006 / Steps 1–2` establish the C++20 domain model, standalone process skeleton and documented project/resource authorization semantics. Step 3 adds service-local durable storage without introducing the external wire API yet.

Implemented domain baseline:

- stable opaque device and metric IDs represented as bounded non-empty strings;
- Device metadata: `name`, `description`, `location`;
- Metric metadata: optional `device_id`, `name`, `description`, semantic value type, engineering `unit`, `writable`, `working/state` kind and state-metric link;
- standalone metrics are represented by the absence of `device_id`;
- value types map semantically to the existing Data Hub baseline: `bool`, `int64`, `uint64`, `double`, `string`, `bytes`;
- every working metric requires a state-metric link;
- a state metric is read-only and cannot have another state link;
- Device project associations are explicit; attached metrics inherit them;
- standalone metrics have their own project associations and standalone working/state pairs must use identical project sets;
- the exact runtime state enum/value encoding is intentionally not selected here.

Step 2 defines the future authorization model: Device can participate in multiple projects; shared/global metadata mutations require global edit/admin and association changes require global admin. `control` is not a metadata capability.

## Durable storage

Step 3 uses SQLite as **service-local Device Manager persistence**, not as a platform-wide database decision.

SQLite schema v1 stores:

- devices;
- metrics;
- working → state metric links;
- Device → project associations;
- standalone Metric → project associations.

`SqliteMetadataRepository` validates the complete `DeviceCatalog` before mutation and replaces it in one SQLite transaction. Invalid metadata is rejected without partially changing the previous catalog. Loaded metadata is validated again before it is returned. A database with a newer unsupported `PRAGMA user_version` is rejected explicitly.

Runtime current values, subscriptions and write execution are not stored here; they remain Data Hub/Driver Runtime responsibilities.

## Build and test

From the repository root in Linux/WSL:

```text
cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_device_manager dispatcher_device_manager_domain_tests dispatcher_device_manager_persistence_tests
ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^device-manager\\."
```

The executable accepts an optional SQLite database path:

```text
dispatcher-device-manager [database-path]
```

Without an argument it uses `dispatcher-device-manager.db` in the current working directory. Step 3 still does not implement Service Hub operations, authorization client code, Data Hub calls, driver configuration or Web UI. External service/provider behavior belongs to later CORE-006 steps.
