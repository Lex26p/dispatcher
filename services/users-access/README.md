# Users & Access

Users & Access is the core backend responsibility for stable user identity, access configuration, authentication/session state and authorization decisions.

## Current implementation stage

`CORE-005 / Step 1` establishes only the domain/application boundary and standalone C++ service skeleton.

There is intentionally no production credential storage, authentication/session contract, Service Hub provider or Web login in this step. Those are added by later CORE-005 steps after their exact security decisions are made.

## Step 1 domain model

### User

A user currently contains:

- stable opaque `id` independent of login/display properties;
- `login` as the local credential identity key;
- mutable human-readable `display_name` data;
- `enabled` state.

No password, verifier, session or token field exists in the Step 1 User model.

### Capabilities

The minimal machine-readable capabilities are:

- `view`;
- `control`;
- `edit`;
- `admin`.

They are independent capabilities. Step 1 does not introduce a hidden hierarchy such as `admin => edit => control => view`; callers must request the capability or accepted capability combination required by their own policy.

### Permission sets and assignments

A `PermissionSet` is a named assignable set of capabilities with its own stable opaque ID.

An assignment links:

- one user;
- one permission set;
- one explicit scope.

Supported Step 1 scopes are:

- global;
- one project identified by opaque project ID.

There are no explicit denies, nested roles/groups, tenant hierarchy, arbitrary ABAC expressions or future Device/Dashboard scopes.

## Effective permission semantics

Evaluation is deterministic and server-side oriented:

1. a missing user returns a user-not-found evaluation error;
2. a disabled user is denied with no effective capabilities;
3. global assignments apply in global and project evaluation;
4. project assignments apply only to that exact project;
5. matching assignments are merged by union of capabilities;
6. duplicate capabilities do not change the result;
7. a project assignment never grants a global capability;
8. invalid/inconsistent repository data fails with a storage evaluation error rather than granting access.

These semantics are sufficient for the first Project Manager enforcement planned later in CORE-005 without creating a universal ACL engine.

## Repository boundary

`UsersAccessRepository` is an internal service storage port for:

- users;
- permission sets;
- access assignments.

Unit tests provide an in-memory implementation. Step 1 does not select production persistence technology.

## Lifecycle

The standalone executable is:

    dispatcher-users-access

It currently starts only the domain skeleton and waits for SIGINT/SIGTERM using the same Linux synchronous signal lifecycle pattern as existing Dispatcher backend services.

No network endpoint or Service Hub registration is created in Step 1.

## Build and test in WSL

From the repository root:

    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_users_access dispatcher_users_access_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^users-access\."

Current CTest checks:

- `users-access.domain-and-application`;
- `users-access.signal-term`;
- `users-access.signal-int`.
