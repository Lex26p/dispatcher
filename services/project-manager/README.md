# Project Manager

Project Manager is the core service responsible for Dispatcher projects as stable, flat points of consolidation and context.

## Current implementation stage

`CORE-004 / Step 1` established the domain/application boundary and standalone C++ service skeleton.

`CORE-004 / Step 2` established the first production durable storage adapter using local SQLite.

`CORE-004 / Step 3` established the versioned Service Hub provider contract at `project-manager.v1`.

`CORE-004 / Steps 4–6` established the `/projects` Web UI, shared browser project context, and real browser → Service Hub → Project Manager → SQLite restart-recovery integration.

`CORE-004 — Project Manager` is complete.

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

    dispatcher-project-manager [database-path] [service-hub-address]

Defaults:

    database-path        dispatcher-project-manager.db
    service-hub-address  127.0.0.1:50052

On startup the executable opens/initializes SQLite storage before entering the Linux signal lifecycle. If storage cannot be opened or initialized, startup fails instead of running with volatile data.

Project Manager does not open its own server endpoint. It connects as a provider to Service Hub `/v1/ws`, requests subprotocol `dispatcher.service-hub.v1`, and registers service `project-manager.v1`. If the Hub connection is lost, the provider retries and registers again after reconnect.

## Service Hub contract

Project Manager v1 uses service address:

    project-manager.v1

Operations:

- `create-project`;
- `list-projects`;
- `get-project`;
- `update-project`.

The full payload/error contract is documented in `docs/architecture/project-manager-contract.md`. Project-specific fields stay inside Service Hub `operation`/`payload`; the common Service Hub envelope is unchanged.


## Web integration

The Web Shell uses the existing shared `ServiceHubClient` and service `project-manager.v1`; it does not open a second transport. `/projects` provides list/create/edit behavior, while `ProjectContextProvider` keeps either a real Project v1 snapshot or explicit global mode for the current browser session.

The real Windows + WSL acceptance command is:

    npx.cmd --yes npm@11.19.0 run test:e2e:project-manager

It starts real C++ Service Hub and Project Manager processes, uses a temporary SQLite database, exercises the production Web UI, restarts Project Manager on the same database, waits for provider re-registration, and verifies stable ID/data/context recovery.

## Intentionally not implemented yet

- authentication/authorization;
- project ownership of future resources;
- Dashboard relations;
- project deletion lifecycle;
- user-specific persistence of selected project context.

## Dependencies

On Ubuntu/WSL the Project Manager dependencies are already aligned with existing core services:

    libsqlite3-dev
    libboost-dev
    libjson-c-dev

SQLite remains local persistence; Boost.Beast + json-c implement the Service Hub adapter. No standalone SQLite server is required.

## Build and test in WSL

From the repository root:

    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_project_manager dispatcher_project_manager_tests dispatcher_project_manager_persistence_tests dispatcher_project_manager_service_hub_test_client
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^project-manager\."

Current CTest checks:

- `project-manager.domain-and-application`;
- `project-manager.persistence`;
- `project-manager.service-hub-integration`;
- `project-manager.signal-term`;
- `project-manager.signal-int`.
