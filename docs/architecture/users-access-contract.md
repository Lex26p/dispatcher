# Внешний контракт Users & Access

## Статус

Контракт фиксируется в `CORE-005 / Step 3`.

Он задаёт versioned предметную boundary Users & Access, необходимую для:

- локальной аутентификации пользователя;
- server-side session lifecycle;
- получения текущего authenticated user;
- backend-authoritative access evaluation;
- минимального администрирования users/permission sets/assignments;
- последующей authenticated Service Hub boundary в Step 4.

Service address:

`users-access.v1`

Machine-readable payload definitions:

`services/users-access/protocol/dispatcher/users_access/v1/users_access.schema.json`

Контракт является language-independent. C++ типы Users & Access являются реализацией сервиса, а не межсервисным API.

## Граница Step 3 / Step 4

Step 3 **не изменяет Service Hub v1 envelope** и ещё не регистрирует production Users & Access provider.

Он фиксирует предметные operations и реализует session/application storage semantics. В Step 4 существующий Service Hub получит согласованный authenticated request context, после чего `users-access.v1` будет подключён к transport boundary.

Только `login` является публичной operation.

Остальные operations требуют authenticated request context. Session token не должен дублироваться внутри их business payload. Точный способ передачи bearer token через Service Hub определяется Step 4 и не является частью этого payload-контракта.

Project Manager и другие providers не должны принимать `user_id`, roles или permissions из собственного business payload как доказательство идентичности.

## User

Wire model пользователя:

```json
{
  "id": "opaque-user-id",
  "login": "operator",
  "display_name": "Operator",
  "enabled": true
}
```

`id` является стабильным opaque identifier и не зависит от mutable login/display properties.

Password, credential verifier, session token digest и другие secrets никогда не входят в User response.

## Capabilities

Минимальные capability names v1:

- `view`;
- `control`;
- `edit`;
- `admin`.

Они независимы. Контракт не задаёт скрытую иерархию `admin => edit => control => view`.

## Scope

Global scope:

```json
{
  "kind": "global"
}
```

Project scope:

```json
{
  "kind": "project",
  "project_id": "opaque-project-id"
}
```

В CORE-005 v1 реально поддерживаются только global и project scope. Device/Dashboard/tenant/field scopes не создаются заранее.

## Permission set

```json
{
  "id": "opaque-permission-set-id",
  "name": "Project operators",
  "capabilities": ["view", "control"]
}
```

Effective permissions являются union всех matching global/project assignments согласно domain semantics Step 1.

## Session model

Session token является opaque bearer secret.

Step 3 фиксирует:

- token генерируется только server-side CSPRNG;
- raw entropy — 256 bit;
- wire representation — 64 lowercase hexadecimal characters;
- token не содержит user ID, roles, permissions, timestamps или других данных;
- SQLite хранит SHA-256 digest token, а не raw bearer token;
- session state хранится server-side;
- idle timeout — `1800000 ms` (30 минут);
- absolute lifetime — `43200000 ms` (12 часов);
- timeout enforcement выполняется server-side;
- successful validation обновляет last-activity timestamp;
- logout удаляет session;
- expired session удаляется при authoritative validation;
- disabled user не может получить новую session, а существующая session становится недействительной при следующей validation;
- sessions сохраняются в durable Users & Access storage и могут пережить restart, пока не истекли или не отозваны.

Browser storage mechanism для bearer token Step 3 не выбирает. Он фиксируется вместе с реальным Web/Service Hub security boundary, а production TLS/origin/deployment policy остаётся отдельной задачей.

Raw session token нельзя писать в обычные diagnostics, audit records или test output.

## Session summary

Protected responses используют session summary без bearer secret:

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

Effective permissions намеренно не кэшируются внутри session token/summary: access должен вычисляться из актуальных assignments.

## Operations

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

Unknown login, wrong password, missing credential и disabled user не должны давать caller различимые credential-specific ответы. Они возвращают generic `auth.invalid_credentials`.

Password никогда не возвращается и не записывается в audit.

### `logout` — protected

Request payload:

```json
{}
```

Success payload:

```json
{}
```

Logout относится к session из authenticated request context. Business payload не содержит token/user ID.

### `current-session` — protected

Request payload:

```json
{}
```

Success:

```json
{
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

Subject user берётся только из trusted authenticated request context. `user_id` в этом request отсутствует.

`evaluate-access` не заменяет service-specific policy. Например, Project Manager Step 5 сам определяет, какая capability требуется для create/update, а Users & Access authoritative вычисляет effective access.

## Administration operations

Эти operations являются protected и требуют global `admin` policy после появления authenticated Service Hub boundary.

### `list-users`

Request: `{}`.

Success:

```json
{
  "users": []
}
```

### `create-user`

Request:

```json
{
  "login": "engineer",
  "display_name": "Engineer",
  "enabled": true,
  "password": "initial secret"
}
```

User + credential должны создаваться fail-closed; password не возвращается.

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

Success payload: `{}`.

Password verifier остаётся внутренним Users & Access storage representation.

### `list-permission-sets`

Request: `{}`.

Success:

```json
{
  "permission_sets": []
}
```

### `create-permission-set`

```json
{
  "name": "Project editor",
  "capabilities": ["view", "edit"]
}
```

Success возвращает `{ "permission_set": ... }`.

### `list-access-assignments`

Request может быть пустым либо ограничивать список конкретным user:

```json
{
  "user_id": "opaque-user-id"
}
```

Success:

```json
{
  "assignments": []
}
```

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

Payload имеет ту же composite identity assignment (`user_id`, `permission_set_id`, `scope`).

Success payload: `{}`.

Explicit deny, nested groups и arbitrary ACL expressions в v1 не добавляются.

## Validation limits

Domain byte limits остаются authoritative и не подменяются JSON Schema character-count limits.

Текущий baseline:

- login: непустой по смыслу, до 256 UTF-8 bytes;
- display name: до 256 UTF-8 bytes;
- permission set name: непустой по смыслу, до 256 UTF-8 bytes;
- password verifier implementation принимает до 1024 bytes;
- first-admin bootstrap требует минимум 15 bytes;
- project ID и stable IDs должны быть non-empty opaque strings.

Password policy для обычного admin create/reset должна быть согласована с Step 2 local credential baseline; plaintext secret никогда не хранится.

## Error codes

Service-specific codes v1:

- `access.invalid_request` — malformed/unknown payload shape;
- `access.unknown_operation` — unknown `users-access.v1` operation;
- `auth.invalid_credentials` — login rejected без user-enumeration detail;
- `auth.invalid_session` — bearer session отсутствует/отозвана/некорректна;
- `auth.session_expired` — authoritative session timeout;
- `access.forbidden` — authenticated user не имеет required capability;
- `access.user_not_found` — target admin user не существует;
- `access.permission_set_not_found` — target permission set не существует;
- `access.conflict` — duplicate login/assignment или другой deterministic conflict;
- `access.storage_error` — durable Users & Access operation failed;
- `auth.crypto_error` — required cryptographic operation failed;
- `access.internal_error` — unexpected Users & Access application failure.

`hub.*` остаётся зарезервированным за Service Hub и не переименовывается.

Disabled user при `login` получает `auth.invalid_credentials`. При использовании ранее выданной session authenticated boundary считает её недействительной; Step 4 фиксирует точное transport mapping так, чтобы disabled state не становился способом подделать identity.

## Security audit

Step 3 добавляет локальные durable event types:

- `authentication_succeeded`;
- `authentication_failed`;
- `session_logged_out`;
- `session_expired`;
- `session_rejected_disabled_user`.

В audit не записываются password, raw session token или credential verifier.

Публикация этих событий в будущий Event Hub не входит в CORE-005 Step 3.

## Control mode

Control mode **не добавляется в session schema Step 3**.

План CORE-005 разрешает определить его session representation позже. Реальный control-mode baseline фиксируется Step 7 после того, как authenticated Service Hub и Web session context уже проверены. Это позволяет не придумывать преждевременную Device/write semantics.

## Versioning

`users-access.v1` фиксирует предметные operation names и payload shapes.

Step 4 может совместимо расширить Service Hub transport envelope authenticated context, но не должен переносить user identity в business payload Users & Access/Project Manager.

Несовместимое изменение Users & Access payload semantics требует нового service version, а не скрытого изменения v1.
