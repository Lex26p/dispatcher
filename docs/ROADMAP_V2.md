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

## [ ] V2-S07 — Authentication foundation

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

### [ ] V2-S07B — Server authentication session

Следующий scope:

- login endpoint;
- logout endpoint;
- current-user endpoint;
- authenticated cookie/session;
- password verification через тот же platform hasher;
- disabled user не может создать новую authenticated session;
- server различает anonymous и конкретного authenticated user.

Permissions/roles по-прежнему не входят в этот подшаг.

### [ ] V2-S07C — Web authentication integration

Следующий Web scope после Server boundary:

- login screen/state;
- current user в application shell;
- logout action;
- корректное anonymous/authenticated navigation behavior без попытки подменить server authorization Web-видимостью.

### Результат V2-S07

Server различает anonymous и конкретного authenticated user.

---

## [ ] V2-S08 — Permissions и Roles

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

Начальные built-in roles могут быть:

```text
Viewer
Operator
Engineer
Administrator
```

Но Server проверяет permission.

Обязательный порядок:

```text
Server authorization
      ↓
Web visibility/enabled state
```

Скрытая Web-кнопка не считается защитой.

### Результат

Операторские и инженерные действия имеют server-side permission boundary.

---

## [ ] V2-S09 — Users/Roles Web + Audit

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

### Результат Phase 7

Dispatcher имеет базовую локальную модель безопасности и трассируемость действий пользователя.

---

# Phase 8 — Alarms

## [ ] V2-S10 — Alarm definitions и Alarm Editor

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

### Результат

Инженер может определить alarm rule без scripting.

---

## [ ] V2-S11 — Alarm runtime state machine

Alarm runtime подписывается на logical Tag changes.

Минимальные состояния должны явно различать:

```text
Normal
ActiveUnacknowledged
ActiveAcknowledged
ReturnedUnacknowledged
```

Нужно определить transitions для:

```text
raise
acknowledge
return-to-normal
ack after return
```

Alarm transition записывается в Event Journal/operational storage.

Delay и hysteresis применяются в runtime, а не в Web.

Не добавлять пока:

```text
shelving
suppression
alarm groups
complex expressions
```

### Результат

Alarm имеет воспроизводимый lifecycle, а не просто boolean flag.

---

## [ ] V2-S12 — Alarm ACK, realtime и Web

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

SignalR сообщает alarm transitions.

Первый scope — acknowledge одной выбранной alarm instance. Bulk ACK можно добавить позже.

### Результат Phase 8

Dispatcher имеет полноценный минимальный alarm lifecycle с идентифицированным оператором.

---

# Phase 9 — Templates

## [ ] V2-S13 — Mimic templates

Первый конкретный template use case.

Цель: повторно использовать фрагменты мнемосхем.

Template может содержать:

```text
elements
relative positions
visual properties
TagId placeholders/parameters
```

Первый instantiate workflow:

```text
template
   ↓ instantiate
copy elements into MimicDefinition
```

Экземпляр не должен автоматически изменяться после изменения template.

Это избегает неочевидных каскадных изменений работающих экранов.

### Результат

Часто повторяющийся visual fragment можно вставлять без ручного повторения всех элементов.

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
