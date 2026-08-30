# Project Manager

Project Manager is the core service responsible for Dispatcher projects as stable, flat points of consolidation and context.

## Current implementation stage

`CORE-004` established the Project Manager domain/application boundary, durable SQLite storage, `project-manager.v1` Service Hub provider, `/projects` Web UI, shared browser project context and real browser restart-recovery integration.

`CORE-005 / Step 5` adds backend-authoritative authorization to the existing Project Manager provider without changing Project v1 business payloads.

Project Manager now requires session authentication for all v1 operations and evaluates current access through `users-access.v1/evaluate-access`.

The current Project model still contains only:

- stable opaque `id`;
- human-readable `name`;
- optional `description`.

The identifier is independent of mutable display properties. Projects are not nested and do not contain Dashboard, Device, ACL or other future-service records.

## Application boundary

`ProjectManager` supports:

- create;
- list;
- get by project ID;
- update name/description.

The application layer still depends only on the abstract `ProjectRepository` storage port. It does **not** depend on Users & Access, Service Hub, session tokens or permission types.

Domain/application tests use an in-memory repository; production startup uses `SqliteProjectRepository`.

Validation requires a non-whitespace name, limits the UTF-8 payload to 256 bytes for `name` and 4096 bytes for `description`, and reports application errors separately from repository failures.

## Authorization boundary

Authorization is applied around the Service Hub provider before protected business operations are executed.

Policy:

- `create-project` requires global `admin`;
- `list-projects` exposes only projects where `view` is allowed;
- `get-project` requires project-scoped `view`;
- `update-project` requires project-scoped `edit` or `admin`.

Capabilities remain independent. Project Manager does not assume `admin => edit => view`.

The incoming bearer is read only from the Service Hub request-level `auth` context. `user_id`, roles or permissions inside Project Manager payload are never trusted and remain invalid/unknown business fields.

Project Manager does not link against Users & Access C++ implementation and does not read its SQLite database. A separate internal Service Hub client connection calls:

    users-access.v1 / evaluate-access

with the forwarded session credential and requested global/project scope.

Effective access is checked on every protected Project Manager request. Assignments are not cached locally, so revocation, disabled users and session expiry take effect on the next authoritative evaluation.

If Users & Access is unavailable, returns an internal storage/crypto failure, or cannot provide a valid evaluation response, Project Manager fails closed with `project.authorization_unavailable`.

Detailed policy/error semantics are documented in `docs/architecture/project-manager-contract.md`.

## Durable persistence

Project Manager uses local SQLite storage.

Reasons for the choice:

- Project Manager owns a small local metadata model;
- SQLite is embedded and transactional, so no additional database service is introduced;
- the database file remains private to Project Manager instead of becoming a cross-service contract;
- create/read/list/update and restart/reopen behavior can be tested directly;
- the choice does not prescribe persistence technology for other Dispatcher services or future history storage.

The internal schema is versioned through SQLite `PRAGMA user_version`. Schema version remains `1` and stores only `id`, `name` and `description`.

`CORE-005 / Step 5` does not add user/access columns or share persistence with Users & Access.

## Lifecycle

The executable is:

    dispatcher-project-manager [database-path] [service-hub-address]

Defaults:

    database-path        dispatcher-project-manager.db
    service-hub-address  127.0.0.1:50052

On startup the executable opens/initializes SQLite before entering the Linux signal lifecycle.

Project Manager does not open its own server endpoint. It connects as provider to Service Hub `/v1/ws`, requests subprotocol `dispatcher.service-hub.v1`, and registers `project-manager.v1`.

The provider remains registered even if Users & Access is temporarily unavailable. In that state protected operations fail closed rather than allowing access.

If Service Hub itself is lost, the provider reconnects and registers again. The internal authorization client also recreates its client-role connection when necessary.

## Service Hub contract

Project Manager v1 uses service address:

    project-manager.v1

Operations:

- `create-project`;
- `list-projects`;
- `get-project`;
- `update-project`.

Project payload definitions are unchanged from CORE-004.

After `CORE-005 / Step 4`, callers may carry a session credential in the shared Service Hub request `auth` field. Step 5 makes that context mandatory for Project Manager business operations and validates its effective access through Users & Access.

No Project Manager payload contains `auth`, `user_id`, role or permissions.

## Web integration during CORE-005 Step 5

The Web Shell already has transport support for optional per-request session `auth`, but shared login/current-user/session ownership is intentionally Step 6.

Therefore Step 5 protects the backend **before** Web login UX exists. The current unauthenticated `/projects` UI receives `auth.invalid_session` from the real Project Manager boundary.

The dedicated real browser Project Manager integration is temporarily focused on confirming this fail-closed boundary. Full authenticated browser CRUD returns in `CORE-005 / Step 6–7` when the Web Shell owns a real user session.

This staging is intentional: Web presentation is not used as a security boundary.

## Step 5 integration coverage

`project-manager.service-hub-integration` now uses real processes:

    Service Hub
        ├── Project Manager
        └── Users & Access

The scenario bootstraps a real admin, creates two projects, seeds project-scoped test users through a test-only Users & Access fixture, and verifies:

- unauthenticated Project Manager access is denied;
- global admin can create projects;
- global view can see the complete list;
- a project-scoped user sees only an allowed project;
- allowed/denied `get-project`;
- create denial without global admin;
- project-scoped edit success and inaccessible update denial;
- project-scoped `admin` can update without implicit `view`, confirming capability independence;
- durable session use after Users & Access restart;
- access revocation reflected on the next request;
- disabled user invalidates an existing session;
- expired session returns `auth.session_expired`;
- Users & Access unavailable => `project.authorization_unavailable`;
- authorization resumes after Users & Access returns;
- Project Manager and Users & Access re-register after Service Hub restart;
- authenticated authorization still works after Hub reconnect;
- service shutdown does not print credential material.

The fixture is test-only. It does not add production administration operations before Step 6.

## Current authorization limitation

When the caller does not have global `view`, `list-projects` evaluates `view` separately for each stored project through `users-access.v1/evaluate-access`. This keeps Step 5 on the existing Users & Access v1 contract and avoids inventing a batch ACL API prematurely.

The baseline is correct and fail-closed, but very large project sets may require a future contract-level batch/visibility optimization after real scale requirements are known. That optimization must preserve authoritative filtering and must not move access state into Project Manager storage.

## Intentionally not implemented yet

- Web login/logout/current-user context — Step 6;
- production Users & Access administration API/UI — Step 6;
- control mode — Step 7;
- project ownership of future resources;
- Dashboard relations;
- project deletion lifecycle;
- user-specific durable persistence of selected project context.

## Dependencies

On Ubuntu/WSL:

    libsqlite3-dev
    libboost-dev
    libjson-c-dev

Project Manager itself still uses SQLite locally plus Boost.Beast/json-c for Service Hub transport.

Real Step 5 integration also requires the existing Users & Access dependencies because the test runs that production service:

    libssl-dev

No direct production library dependency Project Manager → Users & Access is introduced.

## Build and test in WSL

From the repository root:

    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" -j
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^project-manager\."

Current CTest checks:

- `project-manager.domain-and-application`;
- `project-manager.persistence`;
- `project-manager.service-hub-integration`;
- `project-manager.signal-term`;
- `project-manager.signal-int`.

The full build is required before `project-manager.service-hub-integration`, because the scenario also launches the Users & Access production executable and its test-only fixture from the sibling build directory.
