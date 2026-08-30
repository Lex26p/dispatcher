# Users & Access

Users & Access is the core backend responsibility for stable user identity, access configuration, authentication/session state and authoritative access decisions.

## Current implementation stage

- `CORE-005 / Step 1`: domain/application boundary and standalone C++ service skeleton.
- `Step 2`: durable local SQLite storage, scrypt credential verifier and explicit first-admin bootstrap.
- `Step 3`: versioned `users-access.v1` authentication/session contract and server-side session engine.
- `Step 4`: authenticated Service Hub request boundary and production session-core provider.
- `Step 5`: Project Manager uses Users & Access as authoritative authorization dependency.
- `Step 6A` completed in `04e83879c73e298d1eac61acbd8e861f0ba5988d`: all reserved administration operations are connected to the real backend/Service Hub path.
- `Step 6B` completed in `ccde3a262d92ace53069d6e7740108b84f14aad9`: Web owns the browser session lifecycle, login/logout/current user and administration UI over that API.
- `Step 6C` completed in `382e4be446dbc3a4cf8b76cc4a88a67eaff6ba59`: administration mutations are coupled atomically with durable local security audit.
- `Step 7A` completed in `f25aef1d3ff721f86487662289661409f72d3e57`: authoritative project-scoped control mode backend/contract.
- current `Step 8`: backend acceptance and documentation closure. The uncommitted Step 7B Web control-mode/security work was discarded and deferred to `CORE-014 — Web Integration & Core Operations UI`.

## Domain and access model

A user has stable opaque `id`, `login`, mutable `display_name` and `enabled` state.

Independent capabilities are `view`, `control`, `edit`, `admin`; there is no implicit hierarchy. Permission sets are named capability collections. Assignments link a user and permission set to global scope or one project scope. Effective capabilities are the union of matching assignments. Disabled users fail closed.

## Durable persistence

SQLite is service-local Users & Access storage, not a platform-wide database choice.

Schema v2 stores:

- `users`;
- `permission_sets`;
- `access_assignments`;
- `credential_verifiers`;
- `security_audit`;
- `sessions`.

Foreign keys are enabled. Unsupported newer schema versions are rejected.

Step 6A adds `SqliteUsersAccessAdministrationStore`, which opens the same already-initialized schema v2 through a separate FULLMUTEX connection. It does not add a schema migration or a second persistence technology. User + initial credential creation is one SQLite transaction.

## Password verifier baseline

OpenSSL `EVP_PBE_scrypt` parameters remain:

- `N = 2^17`;
- `r = 8`;
- `p = 1`;
- 16-byte random salt;
- 32-byte digest.

Plaintext passwords are never persisted. Ordinary administrator create/reset follows the same local baseline as first-admin bootstrap: 15..1024 bytes, no composition rule.

## Authentication/session baseline

- opaque 256-bit bearer token, 64 lowercase hex on wire;
- SQLite stores SHA-256 token digest only;
- 30-minute idle timeout;
- 12-hour absolute lifetime;
- server-side validation and activity refresh;
- logout removes the session;
- durable sessions survive restart until expiry/revocation;
- disabled users fail closed;
- raw password/session token material is excluded from normal diagnostics/audit.

Service address: `users-access.v1`.

Payload contract: `docs/architecture/users-access-contract.md`.
Machine-readable definitions: `services/users-access/protocol/dispatcher/users_access/v1/users_access.schema.json`.

`login` is public. Every other operation uses Service Hub transport `auth` separately from business payload.

## Administration backend — Step 6A

All previously reserved v1 operations are implemented on the real provider path:

- `list-users`;
- `create-user`;
- `set-user-enabled`;
- `set-user-password`;
- `list-permission-sets`;
- `create-permission-set`;
- `list-access-assignments`;
- `assign-access`;
- `remove-access-assignment`.

Every administration request requires an authenticated session with authoritative **global `admin`** capability. A non-admin request returns `access.forbidden`; missing/invalid session fails before administration work.

The backend keeps existing independent capability and global/project scope semantics. No Device/Dashboard-specific ACL, fixed role hierarchy or arbitrary policy language is added.

Step 6A tests cover the application/storage path and a real Service Hub path including unauthenticated denial, non-admin denial, user creation, password reset, enable/disable, permission sets and assignments.

Step 6C extends the same local `security_audit` table with `user_created`, `user_enabled`, `user_disabled`, `user_password_reset`, `permission_set_created`, `access_assignment_added` and `access_assignment_removed`. The actor is the authoritative authenticated global-admin user; user/assignment events store the target user as subject, while permission-set creation leaves the user-specific subject empty.

Each administration mutation and its audit row share one SQLite transaction. If audit insertion fails, the mutation rolls back and the operation fails closed. No plaintext password/raw bearer is recorded, schema remains v2, and no fake Event Hub/audit mechanism is introduced.


## Control mode — Step 7A

Users & Access now owns an ephemeral session-scoped accidental-write guard through protected `users-access.v1` operations:

- `enable-control-mode` with a project ID;
- `disable-control-mode`;
- `current-control-mode`.

Enable requires authoritative effective `control` for the target project. The mode has a fixed 10-minute absolute lifetime, status reads do not extend it, access revocation resets it, and logout/invalid session makes it unusable. The mode is intentionally in-memory: durable sessions survive service restart, while control mode resets to `inactive`. Only the session-token digest is used as the in-memory key; raw bearer material is not stored.

Control mode is not authorization by itself. Future write-capable services must still evaluate their normal capability/subject policy.

## Browser session integration — Step 6B

The backend contract remains unchanged. Web keeps only the opaque bearer for the current browser session in `sessionStorage`, restores identity through authoritative `current-session`, and sends protected operations through the same Service Hub WebSocket. Invalid/expired session errors clear the browser bearer; browser user/permission presentation never replaces backend authorization.

The administration UI uses the Step 6A API only for authenticated global administrators.

Further Web feature work is frozen during the backend-first phase through `CORE-013`. Future Users & Access Web integration requirements, including control mode and browser security acceptance, are maintained in `docs/development/WEB_IMPLEMENTATION.md`; React + TypeScript remains the selected Web stack.

## Secure first-administrator bootstrap

```text
dispatcher-users-access --bootstrap-admin <login> <display-name> [database-path]
```

Password + confirmation come from stdin; terminal echo is disabled for interactive input. Bootstrap requires empty users storage and atomically creates enabled admin user, `Bootstrap administrators` permission set with all four explicit capabilities, global assignment, scrypt verifier and bootstrap audit record.

## Lifecycle

Normal startup:

```text
dispatcher-users-access [database-path] [service-hub-address]
```

Defaults:

```text
database-path       dispatcher-users-access.db
service-hub-address 127.0.0.1:50052
```

Storage/session/administration adapters initialize before the reconnecting `users-access.v1` provider enters the SIGINT/SIGTERM lifecycle. Initialization failure prevents startup.

## Dependencies

Ubuntu/WSL development dependencies:

```text
libsqlite3-dev
libssl-dev
libboost-dev
libjson-c-dev
```

## Build and test in WSL

From repository root:

```text
cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_users_access dispatcher_users_access_administration_tests dispatcher_users_access_control_mode_tests dispatcher_users_access_control_mode_integration_client dispatcher_users_access_administration_integration_client
ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^users-access\\."
```

Current Users & Access CTests include domain, persistence/credentials, bootstrap, authentication/session, control mode, administration, lifecycle and Service Hub integration. Tests use temporary database paths and do not write credential databases into the repository.
