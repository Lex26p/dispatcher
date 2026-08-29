# Project Manager

Project Manager is the core service responsible for Dispatcher projects as stable, flat points of consolidation and context.

## Current implementation stage

`CORE-004 / Step 1` establishes the domain/application boundary and standalone C++ service skeleton.

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

The application layer depends on the abstract `ProjectRepository` storage port. Step 1 tests provide an in-memory implementation of that port.

Validation currently requires a non-whitespace name, limits the UTF-8 payload to 256 bytes for `name` and 4096 bytes for `description`, and reports application errors separately from repository failures.

## Intentionally not implemented in Step 1

- durable persistence technology;
- Service Hub provider/contract;
- authentication/authorization;
- project ownership of future resources;
- Dashboard relations;
- Web Project Manager UI.

Durable storage is selected and implemented in `CORE-004 / Step 2` from the requirements of this minimal domain model.

## Lifecycle

The executable is:

    dispatcher-project-manager

Step 1 does not listen on a network endpoint yet. It starts the service lifecycle skeleton, reports that Service Hub provider integration is not configured, waits for `SIGINT`/`SIGTERM`, and exits cleanly.

## Build and test in WSL

From the repository root:

    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_project_manager dispatcher_project_manager_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^project-manager\."

Current Step 1 CTest checks:

- `project-manager.domain-and-application`;
- `project-manager.signal-term`;
- `project-manager.signal-int`.
