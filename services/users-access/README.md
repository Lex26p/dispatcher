# Users & Access

Users & Access is the core backend responsibility for stable user identity, access configuration, authentication/session state and authorization decisions.

## Current implementation stage

`CORE-005 / Step 1` established the domain/application boundary and standalone C++ service skeleton.

`CORE-005 / Step 2` established durable local users/access persistence, password-verifier storage and explicit first-administrator bootstrap.

`CORE-005 / Step 3` establishes the versioned `users-access.v1` authentication/session contract and server-side session engine.

There is still no authenticated Service Hub envelope/provider binding or Web login. Transport propagation begins at Step 4.

## Domain model

A user contains:

- stable opaque `id` independent of login/display properties;
- `login` as the local credential identity key;
- mutable human-readable `display_name`;
- `enabled` state.

The minimal independent capabilities remain:

- `view`;
- `control`;
- `edit`;
- `admin`.

There is no hidden hierarchy such as `admin => edit => control => view`.

A `PermissionSet` is a named assignable set of capabilities with its own stable opaque ID. An assignment links one user and permission set to either global scope or one project scope.

Effective permissions are the union of matching global/project assignments. Disabled users fail closed with no effective capabilities.

## Durable persistence

Step 2 selects SQLite specifically as the local Users & Access storage.

The choice is service-local:

- users/access configuration is small transactional metadata;
- credentials, assignments and bootstrap state must survive restart;
- SQLite adds no separate database process;
- the database file remains an internal Users & Access implementation detail;
- this does not select a common Dispatcher database for other services or future history.

Schema version is tracked with `PRAGMA user_version`.

Schema v2 stores:

- `users`;
- `permission_sets`;
- `access_assignments`;
- `credential_verifiers`;
- `security_audit`;
- `sessions`.

Step 3 migrates an existing schema v1 database in-place by adding only the `sessions` table/index and advancing `PRAGMA user_version` to `2`.

Foreign-key checking is enabled. A database with a schema version newer than the executable supports is rejected.

## Password verifier baseline

Step 2 uses OpenSSL `EVP_PBE_scrypt` rather than custom cryptography.

Current stored verifier parameters:

- algorithm: `scrypt`;
- `N = 2^17`;
- `r = 8`;
- `p = 1`;
- 16-byte cryptographically random salt;
- 32-byte derived digest.

The database stores only algorithm parameters, salt and derived verifier. Plaintext passwords are never persisted.

The verifier format is internal to Users & Access and can be evolved later.


## Authentication/session baseline

Step 3 uses server-side opaque sessions.

- the bearer token contains 256 bits of CSPRNG entropy and is represented as 64 lowercase hex characters;
- the token contains no user ID, permissions or timestamps;
- SQLite stores only SHA-256 token digest, never the raw bearer token;
- idle timeout is 30 minutes and absolute lifetime is 12 hours;
- validation and expiration are server-side;
- successful validation refreshes last activity;
- logout removes the session;
- sessions survive Users & Access restart until expiry/revocation;
- disabled users cannot authenticate and an already-issued session fails closed at the next validation;
- authentication success/failure, logout, expiry and disabled-session rejection are recorded in the local security audit without password/token material.

The external service address is:

    users-access.v1

Payload contract: `docs/architecture/users-access-contract.md`.
Machine-readable definitions: `services/users-access/protocol/dispatcher/users_access/v1/users_access.schema.json`.

Step 3 intentionally does not place the session token inside protected operation payloads. `login` is public; protected operations rely on the authenticated request context that Step 4 will add to the existing Service Hub boundary. Browser token storage is not selected yet.

## Secure first-administrator bootstrap

Bootstrap is explicit and separate from normal service startup:

    dispatcher-users-access --bootstrap-admin <login> <display-name> [database-path]

The bootstrap password and confirmation are read from standard input, not command-line arguments or environment variables. Interactive terminal input disables echo while the secret is read.

Bootstrap rules:

- storage must not already contain a user;
- login follows the existing Users & Access validation;
- display name is limited to the existing 256-byte domain limit;
- bootstrap password is 15..1024 bytes and has no composition rule;
- a new enabled user is created with stable opaque ID;
- a `Bootstrap administrators` permission set is created with explicit `view`, `control`, `edit`, `admin`;
- a global assignment links the user to that permission set;
- the scrypt verifier is stored;
- a `bootstrap_admin_created` security-audit record is stored;
- all bootstrap writes happen in one SQLite transaction;
- a second bootstrap is rejected.

Step 2 does not yet expose password login, reset/change-password, sessions or remote administration. Those belong to the Step 3 external contract and later Web work.

## Lifecycle

Normal service startup:

    dispatcher-users-access [database-path]

Default:

    database-path  dispatcher-users-access.db

On normal startup the executable opens/initializes SQLite before entering the SIGINT/SIGTERM lifecycle. Storage initialization failure prevents the service from starting.

No authenticated Service Hub provider binding exists yet. Step 4 owns transport integration.

## Dependencies

Ubuntu/WSL development dependencies added by Step 2:

    libsqlite3-dev
    libssl-dev

OpenSSL provides the established scrypt implementation and secure random salt generation. No standalone SQLite server is required.

## Build and test in WSL

From the repository root:

    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_users_access dispatcher_users_access_tests dispatcher_users_access_persistence_tests dispatcher_users_access_session_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^users-access\\."

Current CTest checks:

- `users-access.domain-and-application`;
- `users-access.persistence-and-credentials`;
- `users-access.bootstrap-cli`;
- `users-access.authentication-and-session`;
- `users-access.signal-term`;
- `users-access.signal-int`.

The tests use temporary database paths and do not write credential databases into the repository.
