# Внешний контракт Users & Access

## Статус

Versioned предметная boundary Users & Access зафиксирована в `CORE-005 / Step 3` и развивается совместимо в рамках `users-access.v1`.

Она используется для:

- локальной аутентификации пользователя;
- server-side session lifecycle;
- получения текущего authenticated user;
- backend-authoritative access evaluation;
- минимального администрирования users/permission sets/assignments;
- authenticated Service Hub request path без второго transport.

Service address:

`users-access.v1`

Machine-readable payload definitions:

`services/users-access/protocol/dispatcher/users_access/v1/users_access.schema.json`

Контракт language-independent. C++ типы Users & Access являются реализацией сервиса, а не межсервисным API.

## Service Hub transport binding

`CORE-005 / Step 4` подключил контракт к существующему Service Hub v1 без изменения endpoint/subprotocol.

Protected request передаёт bearer отдельно от business payload:

```json
"auth": {
  "type": "session",
  "token": "64-lowercase-hex-characters"
}
```

Только `login` является public operation и отправляется без `auth`. Остальные операции protected.

Service Hub проверяет только transport shape. Он не вычисляет user identity/permissions и не является authorization engine. Production Users & Access provider authoritative валидирует token внутри `AuthenticationSessionService`.

Project Manager и другие providers не должны принимать `user_id`, roles или permissions из business payload как доказательство идентичности. `CORE-005 / Step 5` уже применил эту boundary к реальному Project Manager authorization.

Raw bearer token не должен попадать в diagnostics/audit и не хранится Service Hub как durable state. `CORE-005 / Step 6B` хранит bearer только в browser `sessionStorage` текущей вкладки/session под ключом `dispatcher.user-session.v1`. Web не сохраняет user/permissions как security authority: reload выполняет authoritative `current-session`, а `auth.invalid_session` / `auth.session_expired` очищают bearer. Temporary transport failure допускает retry restoration и сам по себе не аннулирует durable server-side session.

## User

```json
{
  "id": "opaque-user-id",
  "login": "operator",
  "display_name": "Operator",
  "enabled": true
}
```

`id` — stable opaque identifier, независимый от mutable login/display properties. Password, verifier, session digest и другие secrets никогда не входят в User response.

## Capabilities

Минимальные capability names v1:

- `view`;
- `control`;
- `edit`;
- `admin`.

Capabilities независимы. Скрытой иерархии `admin => edit => control => view` нет.

## Scope

Global:

```json
{ "kind": "global" }
```

Project:

```json
{
  "kind": "project",
  "project_id": "opaque-project-id"
}
```

В первом CORE-005 реально поддерживаются только global и project scope.

## Permission set

```json
{
  "id": "opaque-permission-set-id",
  "name": "Project operators",
  "capabilities": ["view", "control"]
}
```

Effective permissions — union всех matching global/project assignments согласно domain semantics Step 1.

## Session model

Session token — opaque bearer secret:

- 256 bit CSPRNG entropy;
- wire representation — 64 lowercase hex characters;
- token не содержит user ID, roles, permissions или timestamps;
- SQLite хранит только SHA-256 digest;
- idle timeout — `1800000 ms` (30 минут);
- absolute lifetime — `43200000 ms` (12 часов);
- validation/expiration выполняются server-side;
- successful validation обновляет last activity;
- logout удаляет session;
- sessions durable переживают restart до expiry/revocation;
- disabled user не получает новую session, а existing session fail-closed при следующей validation.

Raw session token нельзя писать в ordinary diagnostics, audit records или test output.

## Session summary

Protected responses используют summary без bearer:

```json
{
  "user": {
    "id": "opaque-user-id",
    "login": "operator",
    "display_name": "Operator",
    "enabled": true
  },
  "issued_at_unix_ms": 1788060000000,
  "absolute_expires_at_unix_ms": 1788103200000,
  "idle_timeout_ms": 1800000
}
```

Permissions не кэшируются в session summary/token: authoritative access вычисляется из актуальных assignments.

## Session-core operations

### `login` — public

Request:

```json
{
  "login": "operator",
  "password": "secret input"
}
```

Success:

```json
{
  "session_token": "64-lowercase-hex-characters",
  "session": {
    "user": {
      "id": "opaque-user-id",
      "login": "operator",
      "display_name": "Operator",
      "enabled": true
    },
    "issued_at_unix_ms": 1788060000000,
    "absolute_expires_at_unix_ms": 1788103200000,
    "idle_timeout_ms": 1800000
  }
}
```

Unknown login, wrong password, missing credential и disabled user возвращают generic `auth.invalid_credentials` без user-enumeration detail.

### `logout` — protected

Request `{}`. Success `{}`. Subject/token берутся только из authenticated request context.

### `current-session` — protected

Request `{}`. Success содержит `{ "session": <SessionSummary> }`.

### `evaluate-access` — protected

Request:

```json
{
  "scope": {
    "kind": "project",
    "project_id": "project-42"
  },
  "capability": "edit"
}
```

Success:

```json
{
  "allowed": true,
  "effective_capabilities": ["view", "edit"]
}
```

Subject берётся только из authenticated request context. `evaluate-access` не заменяет service-specific policy.

## Administration operations

`CORE-005 / Step 6A` подключает все ранее зарезервированные v1 administration operations к реальному application/Service Hub path.

Все они:

- protected session auth;
- требуют authoritative global `admin`;
- не доверяют user identity из business payload;
- используют существующий service-local SQLite schema v2;
- не вводят новый transport или отдельную общую БД.

### `list-users`

Request `{}`.

Success:

```json
{ "users": [] }
```

### `create-user`

```json
{
  "login": "engineer",
  "display_name": "Engineer",
  "enabled": true,
  "password": "initial secret"
}
```

Success возвращает `{ "user": ... }`. User + credential создаются одной SQLite transaction; plaintext password не сохраняется и не возвращается.

### `set-user-enabled`

```json
{
  "user_id": "opaque-user-id",
  "enabled": false
}
```

Success возвращает `{ "user": ... }`.

### `set-user-password`

```json
{
  "user_id": "opaque-user-id",
  "password": "new secret"
}
```

Success `{}`. Verifier остаётся внутренним Users & Access representation.

### `list-permission-sets`

Request `{}`. Success `{ "permission_sets": [] }`.

### `create-permission-set`

```json
{
  "name": "Project editor",
  "capabilities": ["view", "edit"]
}
```

Success возвращает `{ "permission_set": ... }`.

### `list-access-assignments`

Request может быть `{}` либо:

```json
{ "user_id": "opaque-user-id" }
```

Success `{ "assignments": [] }`.

### `assign-access`

```json
{
  "user_id": "opaque-user-id",
  "permission_set_id": "opaque-permission-set-id",
  "scope": {
    "kind": "project",
    "project_id": "project-42"
  }
}
```

Success возвращает `{ "assignment": ... }`.

### `remove-access-assignment`

Payload имеет ту же composite identity (`user_id`, `permission_set_id`, `scope`). Success `{}`.

Explicit deny, nested groups и arbitrary ACL expressions в v1 не добавляются.

## Validation limits

Domain byte limits authoritative:

- login: непустой по смыслу, до 256 UTF-8 bytes;
- display name: до 256 UTF-8 bytes;
- permission-set name: непустой по смыслу, до 256 UTF-8 bytes;
- ordinary admin create/reset password: 15..1024 bytes, без composition rule;
- project ID и stable IDs: non-empty opaque strings.

15-byte minimum согласован с first-admin bootstrap baseline. Plaintext secret никогда не хранится.

## Error codes

Service-specific v1 codes:

- `access.invalid_request`;
- `access.unknown_operation`;
- `auth.invalid_credentials`;
- `auth.invalid_session`;
- `auth.session_expired`;
- `access.forbidden`;
- `access.user_not_found`;
- `access.permission_set_not_found`;
- `access.conflict`;
- `access.storage_error`;
- `auth.crypto_error`;
- `access.internal_error`.

`hub.*` остаётся зарезервированным за Service Hub.

Disabled user при login получает `auth.invalid_credentials`; ранее выданная session при следующей validation — `auth.invalid_session`; expired session — `auth.session_expired`.

## Security audit

Текущий durable session/security audit уже включает bootstrap, authentication, logout, expiry и disabled-session rejection без password/raw bearer material.

План CORE-005 также требует audit значимых administration mutations (user changes, password reset, permission-set/assignment changes). `Step 6A` не изобретает частично атомарный или ложный audit поверх отдельного SQLite connection; `Step 6C` является явным completion item Step 6 для завершения administration audit taxonomy/semantics до перехода к Step 7.

Публикация security audit в будущий Event Hub не входит в CORE-005.

## Control mode

Control mode не входит в Step 6A/6B session schema. Его реальный session baseline остаётся `CORE-005 / Step 7`.

## Versioning

`users-access.v1` сохраняет уже зафиксированные operation names/payload shapes.

Step 4 совместимо добавил Service Hub `auth`, Step 5 применил его к Project Manager, Step 6A активировал уже зарезервированные administration operations, а Step 6B добавляет browser session ownership/restoration поверх того же transport без изменения wire contract. Несовместимое изменение требует нового service version.
