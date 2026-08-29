# Project Manager

Project Manager is the core service responsible for Dispatcher projects as stable, flat points of consolidation and context.

## Current implementation stage

`CORE-004 / Step 1` established the domain/application boundary and standalone C++ service skeleton.

`CORE-004 / Step 2` adds the first production durable storage adapter using local SQLite.

The current Project model contains only:

- stable opaque `id`;
- human-readable `name`;
- optional `description`.

The identifier is independent of mutable display properties. Projects are not nested and do not contain Dashboard, Device or other future-service records.

## Application boundary

`ProjectManager` currently supports:

- create;
- list;
- get by project ID;
- update name/description.

The application layer still depends only on the abstract `ProjectRepository` storage port. Domain/application tests use an in-memory implementation; production startup uses `SqliteProjectRepository`.

Validation requires a non-whitespace name, limits the UTF-8 payload to 256 bytes for `name` and 4096 bytes for `description`, and reports application errors separately from repository failures.

## Durable persistence

Step 2 selects SQLite specifically for Project Manager storage.

Reasons for the choice:

- Project Manager currently owns a small local metadata model;
- SQLite is embedded and transactional, so no additional database service is introduced;
- one database file remains private to Project Manager instead of becoming a cross-service contract;
- create/read/list/update and restart/reopen behavior can be tested directly;
- the choice does not prescribe persistence technology for other Dispatcher services or future history storage.

The internal schema is versioned through SQLite `PRAGMA user_version`. Step 2 schema version is `1` and stores only `id`, `name` and `description`.

A new database path is initialized automatically. A database with a schema version newer than the executable supports is rejected rather than silently modified.

## Lifecycle

The executable is:

    dispatcher-project-manager [database-path]

Default database path:

    dispatcher-project-manager.db

On startup the executable opens/initializes SQLite storage before entering the Linux signal lifecycle. If storage cannot be opened or initialized, startup fails instead of running with volatile data.

Step 2 still does not listen on a network endpoint. Service Hub provider integration is added in `CORE-004 / Step 3`.

## Intentionally not implemented yet

- Service Hub provider/Project Manager external contract;
- authentication/authorization;
- project ownership of future resources;
- Dashboard relations;
- Web Project Manager UI.

## Dependencies

On Ubuntu/WSL the Step 2 development dependency is:

    libsqlite3-dev

No standalone SQLite server is required.

## Build and test in WSL

From the repository root:

    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_project_manager dispatcher_project_manager_tests dispatcher_project_manager_persistence_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^project-manager\."

Current CTest checks:

- `project-manager.domain-and-application`;
- `project-manager.persistence`;
- `project-manager.signal-term`;
- `project-manager.signal-int`.
