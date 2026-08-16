# Дорожная карта Dispatcher v2

Эта дорожная карта продолжает завершённый базовый цикл `S00–S12`.

Первая дорожная карта дала законченный вертикальный срез:

```text
Modbus / SNMP
      ↓
logical Tag
      ↓
Monitoring
      ↓
Device Editor
      ↓
Mimic Editor / Runtime
```

Roadmap v2 развивает Dispatcher в сторону долговременного хранения данных, событий и тревог, управления доступом, повторного использования конфигурации и контролируемой автоматизации.

## Обозначения

- `[x]` — шаг подготовлен/реализован в репозитории.
- `[ ]` — шаг ещё не реализован.
- Ошибка проверки означает продолжение текущего шага.
- Следующий шаг начинается только после локальной проверки и нового Git SHA.
- Шаг можно разделить, если фактический объём окажется слишком большим.

## Цели Roadmap v2

В scope входят:

```text
Historian
Alarms / Events
Users / Roles
Templates
Scripting
```

Не входят автоматически:

```text
distributed execution
redundancy
cluster
external message broker
enterprise identity providers
arbitrary plugin framework
```

Эти направления рассматриваются отдельно после появления реальной необходимости.

## Архитектурные принципы v2

### Logical Tag остаётся главной runtime-границей

Новые подсистемы работают через:

```text
TagId
```

Historian, alarm rules, templates и scripts не должны хранить Modbus address или SNMP OID вместо logical binding.

### Configuration и operational data разделяются

Низкочастотная configuration:

```text
devices
tags
mimics
historian policies
alarm definitions
templates
scripts
security configuration
```

не должна смешиваться с высокочастотными operational records:

```text
history samples
events
alarm transitions
audit records
script executions
```

На первом этапе operational data может оставаться в SQLite, но в отдельной operational database.

Это сохраняет возможность позже заменить storage historian/events без миграции всей configuration database.

### Не блокировать protocol polling записью истории

Historian ingest не должен выполнять тяжёлый disk I/O внутри `TagService.Changed`/protocol polling path.

Между runtime changes и persistence должен быть ограниченный asynchronous ingestion boundary.

Конкретный механизм выбирается на реализации V2-S01 без создания отдельного message broker.

### Security раньше полноценного scripting

Scripting появляется только после:

```text
authentication
permissions
audit
```

Скрипт не получает прямой доступ к filesystem/process/network/DI container по умолчанию.

### Templates начинаются с конкретных use cases

Не создаётся generic template framework до появления хотя бы двух реальных use cases.

Первые два use cases:

```text
Mimic templates
Device/tag templates
```

Только после их реализации общая часть выносится в общий template catalog, если это действительно уменьшает дублирование.

---

# Phase 5 — Historian

## [x] V2-S01 — Historian storage и ingestion foundation

Реализовано:

- отдельная operational SQLite database;
- independent operational schema version `1`;
- `history_samples`;
- typed history record:
  - `TagId`;
  - UTC timestamp;
  - `HistoryValueType`;
  - canonical `ValueText`;
- `HistorianService`;
- bounded `Channel<HistorySample>`;
- `TryWrite` в synchronous `TagService.Changed` callback;
- asynchronous batch writer;
- retry текущего batch при persistence error;
- `DroppedSampleCount`;
- `RejectedSampleCount`;
- Historian hosted service стартует до Modbus/SNMP polling;
- startup/storage/overflow/integration tests без реального оборудования.

Baseline overflow behavior:

```text
buffer full
    ↓
do not block TagService
    ↓
drop newest incoming sample
    ↓
increment diagnostics + warning
```

Первый scope не требует внешней time-series database.

### Результат

```text
TagService
   ↓ change
TryWrite bounded channel
   ↓ async batch writer
Operational SQLite
```

---

## [x] V2-S02 — Historian policies и retention

Реализовано:

- persistent `historian_policies` в configuration SQLite schema v4;
- policy:
  - `TagId`;
  - `Enabled`;
  - `Mode`;
  - `PeriodMilliseconds`;
  - `RetentionDays`;
- modes:
  - `OnChange`;
  - `Periodic`;
- минимальный Periodic interval `100 ms`;
- live policy catalog без restart protocol polling;
- configuration API GET/PUT/DELETE;
- отсутствие policy означает «не архивировать»;
- `Enabled=false` выключает sampling, но не retention;
- stale policy сохраняется после удаления/переименования TagId;
- stale policy возвращается API с `TagExists=false`;
- stale policy не выполняет sampling;
- periodic sampling использует current TagService value и timestamp времени sample;
- retention cleanup hosted service;
- retention применяется per TagId;
- disabled/stale policy продолжает удалять слишком старую историю;
- удаление policy не удаляет существующую history автоматически;
- cleanup diagnostics:
  - `CleanupRunCount`;
  - `DeletedSampleCount`;
- baseline buffer overflow diagnostics V2-S01 сохраняются.

Deadband не добавлен: два требуемых sampling modes закрывают текущий use case без дополнительной numeric-only semantics.

### Результат

Инженер явно выбирает, какие logical tags архивируются, в каком режиме и сколько дней хранится история.

---

## [x] V2-S03 — History query API

Реализовано:

- `GET /api/history`;
- repeated `tagId`;
- обязательные `from` / `to`;
- inclusive time range;
- `order=asc|desc`;
- default `limit=1000`;
- maximum `limit=2000` points per series;
- maximum `16` tags per request;
- duplicate `tagId` rejected;
- deterministic ordering by:
  - `timestamp`;
  - internal `sample_id` tie-breaker;
- response grouped as `Series[]`;
- one series returned for every requested `TagId`, including empty series;
- `Series` preserves requested tag order;
- `Truncated` per series via `limit + 1` query;
- public sample:
  - `Timestamp`;
  - `ValueType`;
  - lossless canonical `ValueText`;
- no protocol-specific fields;
- query does not depend on current configuration/policy, so retained history of deleted/stale tags remains readable;
- existing operational index is reused;
- operational schema remains version `1`;
- excessive response size is bounded to at most `16 × 2000 = 32000` returned samples.

### Результат

Историю одного или нескольких logical tags можно запросить независимо от Web UI через стабильный REST contract.

---

## [x] V2-S04 — Historian Web / Trends

Реализован новый Web service:

```text
/history
```

Компоновка:

```text
слева  → configured/manual TagId selection
центр  → SVG trend + history table
сверху → presets / from / to / order / point limit / query
справа → selected series properties
```

Реализовано:

- global navigation `История / Тренды`;
- выбор одного/нескольких logical TagId;
- Web limit `8` concurrent series;
- configured Modbus/SNMP tags;
- ручной TagId для retained history deleted/stale tags;
- локальный tag filter;
- time presets:
  - 15 min;
  - 1 h;
  - 6 h;
  - 24 h;
- произвольный local `from/to`;
- ASC/DESC;
- selectable API point limit:
  - 100;
  - 500;
  - 1000;
  - 2000;
- SVG trend без внешней chart library;
- numeric/boolean series rendering;
- `String/Json/Null` в dense table;
- client display cap:
  - 1000 SVG points per series;
  - 2000 total table rows;
- API `Truncated` state отображается явно;
- no-data states;
- selected series statistics/properties;
- таблица сохраняет lossless `ValueType + ValueText`;
- Web использует только существующий V2-S03 `/api/history`;
- Server/SQLite schema не изменяются;
- saved trend selections не добавлены;
- realtime historian stream не добавлен.

### Результат Phase 5

Dispatcher хранит, ограниченно запрашивает и показывает долговременную историю logical tags через Trend/Web UI.

---

# Phase 6 — Events foundation

## [x] V2-S05 — Event Journal

Реализован единый immutable operational journal до AlarmService/Audit.

Event record:

```text
EventId
Timestamp
Category
Type
Severity
Source
Message
DataJson
```

Categories:

```text
System
Device
Command
Configuration
```

Severity:

```text
Information
Warning
Error
```

Operational SQLite:

- schema `v1 → v2`;
- existing `history_samples` сохраняется;
- новая append-only table `events`;
- indexes:
  - time;
  - category/time;
  - severity/time;
  - source/time.

Ingestion:

- bounded `Channel<EventRecord>`;
- background batch writer;
- retry текущего batch при persistence error;
- producer не блокирует SQLite;
- `DroppedEventCount`;
- `RejectedEventCount`.

Initial producers:

```text
SystemStarted / SystemStopping
DeviceOnline / DeviceOffline
TagWriteSucceeded / TagWriteFailed
RuntimeConfigurationApplied
```

Device events:

- first observed Online/Offline фиксируется;
- одинаковый status на каждом poll-cycle не создаёт повторные events;
- event создаётся только при status transition.

Tag write events покрывают success и все текущие rejection/error branches.

Configuration event создаётся после успешного Modbus/SNMP runtime apply.

Event Journal не является AlarmService и не имеет update/delete API.

### Результат

Появляется единая долговременная временная лента важных operational событий, готовая к query/Web boundary V2-S06.

---

## [x] V2-S06 — Events API и Web

Реализовано:

### REST query

```text
GET /api/events
```

Filters:

```text
from
to
category?
severity?
source?
text?
page
limit
```

Semantics:

- `from/to` required и inclusive;
- newest-first;
- stable tie-breaker `event_id DESC`;
- `source` — exact case-sensitive match;
- `text` — case-sensitive Unicode substring в:
  - Type;
  - Source;
  - Message;
  - DataJson;
- default page `1`;
- default limit `200`;
- maximum limit `500`;
- `HasMore` через `limit + 1`;
- без `COUNT(*)`;
- journal остаётся read-only/immutable.

### Realtime

Существующий RuntimeHub расширен:

```text
EventAdded
```

Event отправляется только после successful SQLite persistence и имеет реальный `EventId`.

Historical replay через SignalR не добавлен.

### Web

Новый service:

```text
/events
```

Компоновка:

```text
слева  → category / severity / source / text filters
центр  → time range / dense table / paging
справа → selected event details / DataJson
```

Web:

- presets:
  - 15 min;
  - 1 h;
  - 6 h;
  - 24 h;
- local `from/to`;
- limits:
  - 100;
  - 200;
  - 500;
- server-side paging;
- severity state visible in table;
- realtime connection state;
- Live mode;
- new persisted events merge into page 1 when they match filters;
- on historical pages new matching events are counted as `Новые`;
- `DataJson` pretty-printed in details;
- global navigation `События`.

Operational SQLite schema остаётся version `2`.

### Результат Phase 6

Инженер может диагностировать исторические и новые operational события системы до появления Alarm rules.

---

# Phase 7 — Users / Roles / Audit

## [x] V2-S07 — Authentication foundation

Цель: ввести идентичность пользователя без самодельной криптографии.

Первый scope:

- local users;
- secure platform password hashing;
- login;
- logout;
- current user;
- authenticated session;
- initial administrator bootstrap;
- disabled users.

Не добавлять OAuth/OIDC/LDAP/AD до отдельного требования.

### [x] V2-S07A — Local users storage, password hashing и bootstrap

Реализовано:

- `local_users` в configuration SQLite;
- configuration schema `v4 → v5` без изменения operational schema;
- local user:
  - `UserId`;
  - `UserName`;
  - `NormalizedUserName`;
  - `DisplayName`;
  - `Enabled`;
  - `PasswordHash`;
- case-insensitive logical username через unique normalized value;
- password хранится только как hash;
- hashing выполняет штатный ASP.NET Core Identity `PasswordHasher<TUser>`;
- bootstrap первого local user выполняется только если таблица пользователей пуста и явно задан bootstrap password;
- скрытого/default password нет;
- bootstrap password не сохраняется в SQLite;
- disabled state является persistent security configuration;
- роли, permissions и отдельный administrator flag не добавлены преждевременно.

Bootstrap configuration:

```text
Authentication:BootstrapAdministrator:UserName
Authentication:BootstrapAdministrator:DisplayName
Authentication:BootstrapAdministrator:Password
```

`Password` по умолчанию пуст. Для первого запуска его следует передавать через secret/environment configuration, например `Authentication__BootstrapAdministrator__Password`.

### [x] V2-S07B — Server authentication session

Реализовано:

- `POST /api/auth/login`;
- `POST /api/auth/logout`;
- `GET /api/auth/current`;
- password verification через тот же ASP.NET Core Identity `PasswordHasher<TUser>`;
- case-insensitive username lookup через existing `NormalizedUserName`;
- generic `401` для unknown user / неверного password / disabled user;
- disabled user не может создать новую authenticated session;
- ASP.NET Core cookie authentication без собственного session token format;
- non-persistent HttpOnly cookie `Dispatcher.Auth`;
- `SameSite=Strict`;
- session ticket lifetime `8 hours` со sliding expiration;
- claims содержат только identity:
  - `UserId`;
  - `UserName`;
  - `DisplayName`;
- `GET /api/auth/current` явно различает anonymous/authenticated state;
- `POST /api/auth/logout` завершает cookie session;
- configuration/operational SQLite schema не меняются;
- существующие runtime/configuration endpoints пока не получают authorization requirements.

Roles, permissions, audit login events и Web login по-прежнему не входят в этот подшаг.

### [x] V2-S07C — Web authentication integration

Реализовано:

- scoped `AuthenticationClient` в Blazor WebAssembly;
- startup/current-session check через `GET /api/auth/current`;
- compact login screen для anonymous user;
- anonymous state не показывает service drawer и service workspace;
- login использует existing `POST /api/auth/login`;
- successful login сохраняет текущий browser route;
- authenticated global header показывает `DisplayName` и `UserName`;
- logout action использует existing `POST /api/auth/logout`;
- logout возвращает Web в anonymous login state, сохраняя текущий route для следующего login;
- invalid credentials показывают generic login error;
- client не хранит отдельный auth token/localStorage state и не читает HttpOnly cookie напрямую;
- roles/permissions не добавлены;
- Web visibility явно не считается security boundary: Server permission enforcement остаётся V2-S08.

### Результат V2-S07

Dispatcher имеет полный минимальный local authentication vertical slice: durable local user, platform password hashing, cookie session, Server current-user identity и Web login/current-user/logout flow. Anonymous/authenticated UX различается, но permission-based Server authorization начинается только в V2-S08.

---

## [x] V2-S08 — Permissions и Roles

Авторизация строится по permissions, а не по scattered role-name checks.

Начальный набор permissions:

```text
Runtime.Read
Tags.Write

Devices.Edit
Mimics.Edit
Historian.Configure
Alarms.Configure
Alarms.Acknowledge

Users.Manage
Roles.Manage

Templates.Edit

Scripts.Edit
Scripts.Execute
```

Начальные built-in roles:

```text
Viewer
Operator
Engineer
Administrator
```

Server проверяет effective permission, а не role name.

Обязательный порядок:

```text
Server authorization
      ↓
Web visibility/enabled state
```

Скрытая Web-кнопка не считается защитой.

### [x] V2-S08A — Permission/role configuration foundation

Реализовано:

- configuration SQLite schema `v5 → v6`;
- durable tables:
  - `security_roles`;
  - `security_role_permissions`;
  - `security_user_roles`;
- permission identifiers определены централизованно в `Dispatcher.Contracts.Authorization.PermissionNames`;
- built-in role definitions idempotently поддерживаются Server startup:
  - `Viewer` → `Runtime.Read`;
  - `Operator` → `Runtime.Read`, `Tags.Write`, `Alarms.Acknowledge`;
  - `Engineer` → runtime/write/device/mimic/historian/alarm/template/script engineering permissions, без user/role administration;
  - `Administrator` → все declared permissions;
- built-in role mappings являются system-managed;
- bootstrap user получает `Administrator`;
- при first migration existing V2-S07 database с одним local user и без assignments этот user получает `Administrator` как one-time compatibility bridge;
- если existing users несколько, Server не угадывает initial administrator;
- delayed bootstrap после startup без password также получает `Administrator`;
- singleton `SecurityCatalog` вычисляет effective permissions как union assigned role permissions;
- disabled user получает empty effective permissions;
- cookie остаётся identity-only, roles/permissions в claims не копируются;
- REST/SignalR endpoint enforcement в этом подшаге ещё не включается.

### [x] V2-S08B — Server permission enforcement

Реализовано:

- ASP.NET Core authorization policies/requirements поверх current `SecurityCatalog`;
- permission handler получает `UserId` из authenticated principal и проверяет effective permission, а не role name;
- protected read boundaries → `Runtime.Read`:
  - `/api/tags`;
  - `/api/devices`;
  - Modbus/SNMP configuration reads;
  - historian policy read;
  - `/api/history`;
  - `/api/events`;
  - mimic runtime reads;
- RuntimeHub `/hubs/runtime` → `Runtime.Read`;
- tag writes → `Tags.Write`;
- Modbus/SNMP configuration mutations → `Devices.Edit`;
- mimic configuration mutations → `Mimics.Edit`;
- historian policy mutations → `Historian.Configure`;
- `/health` и authentication API остаются public;
- anonymous protected request получает `401`;
- authenticated user без effective permission получает `403`;
- cookie challenge/forbid для API не делает redirect в SPA;
- unknown non-read `/api` mutation fail-closed до появления явной permission mapping;
- integration tests покрывают anonymous, Viewer, Operator, Engineer и изменение current `SecurityCatalog` после выдачи cookie;
- role-name checks в request authorization отсутствуют.

### [x] V2-S08C — Web permission visibility/enabled state

Реализовано после Server enforcement:

- authenticated login/current-user response содержит current `EffectivePermissions[]` из `SecurityCatalog`;
- cookie остаётся identity-only и не получает role/permission claims;
- `AuthenticationClient` хранит Server-projected permissions и предоставляет `HasPermission` / `HasAllPermissions`;
- Monitoring, Mimics runtime, History и Events требуют `Runtime.Read` в Web route/navigation projection;
- Device Editor требует `Runtime.Read + Devices.Edit`;
- Mimic Editor требует `Runtime.Read + Mimics.Edit`;
- direct route к недоступному editor не рендерит editor и показывает явное insufficient-permission state;
- Monitoring tag write controls показываются только при `Tags.Write`;
- Mimic Button command становится disabled без `Tags.Write`;
- editor navigation/action visibility отражает соответствующий permission;
- History/Events остаются read-only services; текущего Historian configuration Web control ещё нет;
- client checks остаются UX и не заменяют Server authorization V2-S08B.

### Результат V2-S08

Операторские и инженерные действия имеют server-side permission boundary, а Web отражает уже существующую Server authority через effective-permission projection. Authentication cookie остаётся identity-only; изменение Web visibility не является механизмом защиты.

---

## [x] V2-S09 — Users/Roles Web + Audit

Web admin service:

```text
Users / Roles
```

Функции:

- создать/отключить user;
- изменить display name;
- reset/change password по безопасному flow;
- назначить roles;
- посмотреть effective permissions.

Security-sensitive actions пишутся в audit/events:

```text
login success/failure where appropriate
user create/disable
role changes
tag writes
alarm acknowledgement
configuration changes
script execution
```

Audit record должен содержать actor identity.

### [x] V2-S09A — Users/Roles management API foundation

Реализовано:

- Server contracts для user/role management без password hash exposure;
- `/api/security/users`:
  - list;
  - create;
  - update `DisplayName` / `Enabled`;
  - password reset;
  - replace role assignments;
- `/api/security/roles`:
  - list;
  - create custom role;
  - update custom role;
  - delete unassigned custom role;
- `UserName` immutable после создания;
- `/api/auth/current` читает актуальные user metadata из durable `local_users` по identity `UserId`, поэтому изменённый `DisplayName` не застывает в cookie projection;
- password reset использует тот же platform `PasswordHasher<LocalUserConfiguration>` и limits `12..256`;
- built-in roles system-managed и не изменяются management API;
- role permissions принимают только declared `PermissionNames`;
- user profile/create endpoints требуют `Users.Manage`;
- role CRUD/assignments требуют `Roles.Manage`;
- credential reset требует одновременно `Users.Manage + Roles.Manage`;
- после security mutation current `SecurityCatalog` перечитывается из durable configuration;
- mutation, уменьшающая authority, отклоняется если после неё не останется enabled user с обоими `Users.Manage` и `Roles.Manage`;
- проверка administrative survivability использует effective permissions, а не role names;
- configuration schema остаётся `6`;
- Web admin UI и audit events в S09A не добавляются.

### [x] V2-S09B — Users/Roles Web admin service

Реализовано поверх S09A API без изменения Server/storage schema:

- Web route `/security`;
- global navigation `Пользователи / Роли` доступна при `Users.Manage OR Roles.Manage`;
- direct route использует ту же OR capability projection и не требует `Runtime.Read`;
- local navigation разделяет `Пользователи` и `Роли` согласно current permissions;
- users center — плотная таблица login/display-name/Enabled/roles/effective-permission count;
- selected user properties справа:
  - immutable `UserName`;
  - `DisplayName`;
  - `Enabled`;
  - role assignments;
  - effective permissions;
  - password reset;
- create user с initial password и Enabled state;
- roles center — плотная таблица role type/assigned users/permission count;
- built-in roles отображаются read-only через DTO `BuiltIn`, а не role-name checks;
- custom role create/update/delete и declared permission picker;
- assigned custom role delete action disabled до снятия assignments; Server остаётся окончательной проверкой;
- `Users.Manage` в одиночку не вызывает roles API и даёт только user-management UI;
- `Roles.Manage` в одиночку не вызывает users API и даёт только role-management UI;
- role assignments и password reset показываются только при `Users.Manage + Roles.Manage`;
- Server `ProblemDetails.detail`, включая lockout `409`, показывается как operation error;
- после successful security mutation Web refresh-ит `GET /api/auth/current` через `AuthenticationClient`, чтобы current actor metadata/permissions/navigation обновились сразу;
- client visibility/enabled state остаётся UX only и не заменяет S09A Server permission policies/lockout guard;
- actor-aware audit events ещё не добавлены.

### [x] V2-S09C — Actor-aware security audit wiring

Реализовано:

- operational SQLite schema `v2 → v3`; existing history/events сохраняются;
- `EventRecord` / `EventRecordDto` получили nullable `ActorUserId` / `ActorUserName`;
- старые/system/device events остаются actor-less;
- login success сохраняет verified actor identity;
- login failure сохраняет `ActorUserId/ActorUserName = null` и только bounded attempted username в `DataJson`; password не журналируется;
- user create/update, role assignment, password reset и role create/update/delete создают actor-aware security events;
- password reset audit не содержит plaintext password или hash;
- существующие `TagWriteSucceeded` / `TagWriteFailed` получают actor identity после Server authorization;
- user-driven Modbus/SNMP, Mimic и Historian policy mutations создают actor-aware `ConfigurationChanged`;
- existing `RuntimeConfigurationApplied` остаётся отдельным operational event и не подменяется audit record;
- Event Journal сохраняет append-only/read-only модель и существующий bounded asynchronous ingestion contract;
- audit actor fields доступны через Events REST/SignalR contract без отдельной mutable audit configuration.

Не добавляются отдельный compliance audit store, IP/device fingerprinting, session-token storage или audit update/delete API.

### Результат Phase 7

Dispatcher имеет базовую локальную модель безопасности, permission-based Users/Roles administration и actor-aware immutable trace security-sensitive действий пользователя.

---

# Phase 8 — Alarms

## [x] V2-S10 — Alarm definitions и Alarm Editor

Alarm binding:

```text
TagId
```

Первый набор типов без expression language:

```text
Digital true/false
High
Low
```

Definition минимум:

```text
AlarmId
Name
Enabled
TagId
Condition
Threshold
Severity
Message
Delay
Hysteresis
```

`Threshold/Hysteresis` используются только там, где применимы.

Alarm definitions хранятся как configuration, а не в history table.

Editor использует стандартную схему:

```text
слева  → alarms
центр  → rules/list
справа → properties
сверху → add/save/delete
```

### [x] V2-S10A — Alarm definitions + Server configuration foundation

Реализовано:

- configuration SQLite schema `v6 → v7`;
- durable `alarm_definitions`;
- stable immutable `AlarmId`;
- protocol-neutral binding только по `TagId`;
- conditions `DigitalTrue`, `DigitalFalse`, `High`, `Low`;
- alarm-specific severity type с первым набором `Information`, `Warning`, `Error`, выровненным с текущей Event Journal taxonomy;
- digital conditions не имеют `Threshold/Hysteresis`;
- `High/Low` требуют decimal threshold и non-negative decimal hysteresis;
- decimal threshold/hysteresis сохраняются как invariant text без binary floating conversion;
- non-negative `DelayMilliseconds`;
- Server CRUD `/api/configuration/alarms/definitions`;
- reads требуют `Runtime.Read`; mutations требуют `Alarms.Configure`;
- create/update проверяют current logical `TagId`; definition не получает FK/cascade к protocol tag table и может стать stale после последующего удаления tag;
- successful create/update/delete пишут actor-aware `ConfigurationChanged`;
- Alarm runtime state, transitions, ACK и realtime в S10A не добавляются.

### [x] V2-S10B — Alarm Editor Web

Реализовано поверх S10A API без изменения Server/storage schema:

- Web route `/alarms`;
- global navigation `Тревоги`;
- route/navigation требуют `Runtime.Read + Alarms.Configure`;
- слева — плотный список definitions с Enabled и severity state;
- центр — compact add/save/delete/refresh toolbar и dense rules table;
- справа — properties выбранного definition;
- client-side draft + explicit Save;
- persisted `AlarmId` immutable/read-only;
- logical `TagId` picker объединяет Modbus/SNMP configuration;
- stale persisted TagId остаётся видимым и помечается как отсутствующий; Save требует current configured TagId;
- `DigitalTrue/DigitalFalse` скрывают Threshold/Hysteresis;
- `High/Low` редактируют decimal Threshold и non-negative Hysteresis;
- Server `ProblemDetails.detail` показывается как operation error;
- unsaved selection/refresh требует явного подтверждения discard;
- Web не вычисляет alarm active state, delay/hysteresis transitions или ACK;
- client visibility/enabled state остаётся UX only, Server S10A permission boundary authoritative.

### Результат

Инженер может определить alarm rule без scripting.

---

## [x] V2-S11 — Alarm runtime state machine

Реализовано:

- live `AlarmDefinitionCatalog`, загружаемый из configuration SQLite на startup и обновляемый после S10A CRUD mutations;
- singleton/hosted `AlarmRuntimeService`;
- subscription только на protocol-neutral `TagService.Changed`;
- states:
  - `Normal`;
  - `ActiveUnacknowledged`;
  - `ActiveAcknowledged`;
  - `ReturnedUnacknowledged`;
- transitions:
  - raise;
  - acknowledge;
  - return-to-normal;
  - acknowledge after return;
  - re-raise from `ReturnedUnacknowledged`;
- `High` raise при `value >= Threshold`;
- `High` return только при `value < Threshold - Hysteresis`;
- `Low` raise при `value <= Threshold`;
- `Low` return только при `value > Threshold + Hysteresis`;
- saturating decimal hysteresis bounds без overflow на крайних `decimal` values;
- `DigitalTrue/DigitalFalse` для bool и numeric zero/non-zero values;
- unsupported nonnumeric runtime value не создаёт transition;
- `DelayMilliseconds` применяется только к raise/re-raise;
- pending raise отменяется, если condition перестала быть active до истечения delay;
- delayed raise перед transition повторно проверяет current `TagService` value;
- metadata-only definition update (`Name/Message/Severity`) сохраняет lifecycle state;
- изменение evaluation semantics (`Enabled/TagId/Condition/Threshold/Delay/Hysteresis`) отменяет pending delay, сбрасывает instance в `Normal` и переоценивает current value;
- delete/disable не имитирует `return-to-normal` event: это configuration change, а не физический return condition;
- `TagService.Cleared` при device/tag live apply отменяет pending delay; stale/removed TagId сбрасывается в `Normal`, а state ещё configured tag сохраняется до нового sample;
- automatic raise/return actor-less; internal acknowledge transition принимает optional `EventActor` для будущего S12 API;
- переходы записываются в existing immutable Event Journal:
  - `AlarmRaised`;
  - `AlarmAcknowledged`;
  - `AlarmReturned`;
- alarm transition events используют current `EventCategory.System`, `source = AlarmId` и alarm severity;
- operational SQLite остаётся schema `3`; отдельная mutable current-state table и новый EventCategory не добавлены;
- state-machine tests покрывают active ACK, ACK after return, High/Low hysteresis, digital conditions, delay cancellation/continuous activation и live definition changes.

Не добавлено в S11:

```text
operator ACK REST API
Alarms.Acknowledge enforcement endpoint
Alarm SignalR realtime contract
Active alarms Web runtime
Alarm history Web service
shelving
suppression
alarm groups
complex expressions
```

### Результат

Alarm имеет воспроизводимый four-state lifecycle и durable transition timeline, а не просто boolean flag. Внешний operator workflow остаётся V2-S12.

---

## [x] V2-S12 — Alarm ACK, realtime и Web

Новый operator service:

```text
Alarms
```

Основной экран:

```text
Active alarms
Alarm history/events
```

Для Active alarms:

- severity;
- source/TagId;
- message;
- raised time;
- state;
- acknowledged by;
- acknowledged at;
- current value.

ACK:

```text
authenticated user
+ Alarms.Acknowledge permission
+ timestamp
```

Реализовано:

- `GET /api/alarms/current` возвращает только non-normal runtime instances;
- snapshot содержит severity, AlarmId/TagId, message, state, `RaisedAt`, ACK actor/time, last transition и current logical Tag value;
- `GET /api/alarms/history` читает только `AlarmRaised / AlarmAcknowledged / AlarmReturned` из existing immutable Event Journal;
- history paging сохраняет newest-first ordering и `limit + 1`/`HasMore`;
- `POST /api/alarms/{alarmId}/acknowledge` выполняет ACK одной instance;
- ACK endpoint требует `Alarms.Acknowledge`, а current/history reads — `Runtime.Read`;
- verified actor берётся из authenticated principal и попадает в `AlarmAcknowledged` actor fields;
- `RuntimeHubContract.AlarmChanged` сообщает current runtime changes;
- operator Web `/alarms` показывает current alarms и alarm transition history;
- current table показывает severity, TagId, state, raised time, ACK actor/time, current value и message;
- ACK control доступен только при `Alarms.Acknowledge`;
- existing Alarm Editor перенесён на `/alarms/editor` и по-прежнему требует `Runtime.Read + Alarms.Configure`;
- operational schema остаётся `3`, configuration schema — `7`;
- bulk ACK, shelving, suppression и groups не добавлены.

### Результат Phase 8

Dispatcher имеет полноценный минимальный alarm lifecycle с идентифицированным оператором: definition → runtime transition → current/realtime projection → permission-protected ACK → immutable actor-aware history.

---

# Phase 9 — Templates

## [ ] V2-S13 — Mimic templates

Первый concrete template use case разделён на два проверяемых подшага.

### [x] V2-S13A — Mimic template Server/storage/API foundation

Реализовано:

- configuration SQLite `v7 → v8`;
- concrete table `mimic_templates` без преждевременного generic Template Catalog;
- template хранит `TemplateId`, `Name`, fragment `Width/Height`, parameters и relative elements;
- Tag-bound element использует либо fixed `TagId`, либо `TagParameterId`;
- template parameter задаёт только logical TagId placeholder, protocol address/OID в template domain не попадает;
- CRUD `/api/configuration/mimic-templates`;
- reads требуют `Runtime.Read`, mutations — `Templates.Edit`;
- instantiate `POST /api/configuration/mimics/{mimicId}/templates/{templateId}/instantiate` остаётся mutation target mimic и требует `Mimics.Edit`;
- instantiate проверяет полный parameter binding, добавляет insertion offset, генерирует новые ElementId и сохраняет обычные `MimicElementConfiguration`;
- созданный instance не хранит ссылку на template и не меняется после будущего template update/delete;
- successful template CRUD и instantiate пишут existing actor-aware `ConfigurationChanged`;
- storage/API tests покрывают v7→v8 migration, round-trip, permissions, parameters и copy independence.

Не добавлено в S13A:

```text
Mimic Editor template picker/placement UI
generic Template Catalog / Kind / Version
device/tag templates
linked template instances
automatic propagation
```

### [ ] V2-S13B — Mimic Editor template integration

Web editor должен дать инженерный workflow:

```text
выбрать template
→ заполнить TagId parameters
→ задать insertion position
→ instantiate
→ получить обычные editable mimic elements
```

Template management/editing UI и placement должны соблюдать существующий dense Mimic Editor layout и permission projection `Templates.Edit` / `Mimics.Edit`.

### Результат после V2-S13

Часто повторяющийся visual fragment можно вставлять без ручного повторения всех элементов, при этом работающая мнемосхема не получает скрытой зависимости от последующих изменений template.

---

## [ ] V2-S14 — Device/Tag templates и общий Template Catalog

Второй конкретный use case — повторяющаяся device/tag configuration.

Первый scope должен быть protocol-aware, а не притворяться полностью generic.

Примеры:

```text
Modbus TCP device template
SNMP v2c device template
```

Template может задавать:

- набор tags;
- default names;
- protocol-specific point settings;
- writable flags где применимо;
- параметры экземпляра.

После появления двух concrete use cases:

```text
Mimic template
Device/tag template
```

выделить только действительно общие свойства:

```text
TemplateId
Name
Kind
Version
Parameters
```

### Результат Phase 9

Dispatcher имеет reusable configuration templates без преждевременного универсального template engine.

---

# Phase 10 — Scripting

## [ ] V2-S15 — Script security boundary и runtime foundation

Перед выбором scripting engine фиксируется разрешённый host API.

Скрипт должен получать контролируемые операции наподобие:

```text
ReadTag(TagId)
WriteTag(TagId, value)
Log(...)
EmitEvent(...)
```

`WriteTag` обязан проходить существующую command/permission boundary.

По умолчанию скрипту запрещены прямые:

```text
filesystem
process start
raw network
reflection
application DI container
database connection
```

Нужны:

- execution timeout;
- cancellation;
- error isolation;
- bounded output/log;
- concurrency policy.

Scripting engine выбирается на этом шаге после проверки актуальных supported libraries и sandbox characteristics.

Не использовать произвольную компиляцию пользовательского C# внутри Server только потому, что приложение написано на C#.

### Результат

Есть контролируемая execution boundary до появления пользовательского script editor.

---

## [ ] V2-S16 — Script definitions, Editor и manual execution

Script definition:

```text
ScriptId
Name
Enabled
Source
```

Web:

```text
Scripts
├── list
├── editor
├── validation
├── Run/Test
└── execution result/log
```

Save и Run являются разными действиями.

Permissions:

```text
Scripts.Edit
Scripts.Execute
```

Manual execution создаёт audit/execution record.

### Результат

Инженер может безопасно сохранить и вручную выполнить script.

---

## [ ] V2-S17 — Script triggers и observability

После стабильного manual runtime добавляются triggers.

Первый набор:

```text
Timer
TagChanged
AlarmTransition
```

Для каждого script явно задаётся concurrency policy, например:

```text
SkipIfRunning
QueueOne
```

Нельзя допускать бесконтрольного параллельного запуска одного script.

Execution history:

```text
ScriptId
StartedAt
FinishedAt
Trigger
UserId?
Outcome
Error
Duration
```

Ошибки script должны быть видимы в Web и Event Journal.

### Результат Phase 10

Dispatcher получает контролируемую event-driven automation без обхода logical Tag, permissions и audit boundaries.

---

# Итог Roadmap v2

После V2-S17 целевая цепочка выглядит так:

```text
Protocols
   ↓
Logical Tags
   ├────────────→ Current runtime / Monitoring
   │
   ├────────────→ Historian → Trends
   │
   ├────────────→ Alarm runtime → Alarms / Events
   │
   └────────────→ Scripts
                      ↓
                 controlled commands

Users / Roles
   ↓
permissions + audit
   ↓
writes / ack / configuration / scripts

Templates
   ↓
reusable device + mimic configuration
```

## Осознанно отложено после v2

До отдельного решения не включать автоматически:

```text
alarm shelving/suppression
complex alarm expressions
historian clustering
external time-series database
LDAP / Active Directory / OIDC
distributed script workers
arbitrary filesystem/network scripting
generic plugin framework
redundancy / HA
```

Эти возможности рассматриваются после измерения фактических ограничений Roadmap v2.
