# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Базовый цикл S00–S12 завершён. Roadmap v2: Phase 5 Historian и Phase 6 Events завершены. V2-S07 Authentication foundation и V2-S08 Permissions/Roles vertical slice завершены. Phase 7 продолжена V2-S09A: Server имеет permission-protected Users/Roles management API поверх существующей configuration SQLite v6; Web admin UI и actor-aware audit остаются следующими подшагами.

## Рабочая цепочка

```text
Modbus TCP ─→ Dispatcher.Modbus ─┐
                                 ├─→ TagService / DeviceStateService
SNMP v2c  ─→ Dispatcher.Snmp ────┘             ↓
                                         REST / SignalR
                                               ↓
          Monitoring / History / Events / Mimic runtime / Device Editor
```

Система умеет:

1. Читать Modbus Holding Register `UInt16` через FC03.
2. Записывать разрешённые Modbus Holding Register через FC06.
3. Опросить SNMP v2c OID через GET.
4. Публиковать Modbus и SNMP через общие logical `TagId`/`DeviceId`.
5. Хранить device/tag configuration в SQLite.
6. Редактировать Modbus TCP и SNMP v2c через общий Device Editor.
7. Хранить определения мнемосхем в той же SQLite database.
8. Показывать мнемосхемы как SVG runtime.
9. Binding-ить `Value`, `Indicator` и `Button` только по `TagId`.
10. Получать realtime values через существующий `RuntimeStateClient` / SignalR.
11. Выполнять простую команду из `Button` через существующий tag write path.
12. Создавать и удалять мнемосхемы через Web.
13. Добавлять/удалять `Text`, `Rectangle`, `Value`, `Indicator`, `Button`.
14. Редактировать координаты и размеры элементов справа.
15. Выбирать logical `TagId` из Modbus/SNMP configuration.
16. Сохранять editor draft в тот же definition, который исполняет runtime.
17. Архивировать выбранные logical tags по `OnChange` или `Periodic` policy.
18. Хранить historian policy отдельно от operational samples.
19. Применять historian policy без restart protocol polling.
20. Удалять history samples по per-tag `RetentionDays`.
21. Запрашивать lossless history одного или нескольких `TagId` через ограниченный REST API.
22. Просматривать history как SVG trend и плотную таблицу через Web `/history`.
23. Записывать immutable operational events для system/device/command/configuration producers.
24. Запрашивать Events по времени/category/severity/source/text с server-side paging.
25. Просматривать Events в Web `/events` и получать новые persisted events через SignalR.
26. Хранить локальные user identities в configuration SQLite без plaintext passwords.
27. Создавать первого local user через явный bootstrap password, используя platform password hashing.
28. Сохранять persistent `Enabled/Disabled` state local user.
29. Проверять local password через platform `PasswordHasher<TUser>` без собственного credential format.
30. Создавать и завершать authenticated cookie session через Server API.
31. Различать anonymous и конкретного authenticated user через `GET /api/auth/current`.
32. Проверять текущую authentication session при старте Blazor WebAssembly и показывать login screen для anonymous user.
33. Показывать current user и logout action в компактном application header после входа.
34. Сохранять текущий Web route при login/logout, не выдавая client-side visibility за server authorization.
35. Хранить roles, role permissions и user-role assignments в configuration SQLite.
36. Вычислять effective permissions пользователя через Server `SecurityCatalog`, не проверяя role names в business endpoints.
37. Защищать REST read boundaries и RuntimeHub permission `Runtime.Read`.
38. Разделять mutation permissions для tag write, device configuration, mimic configuration и historian policies.
39. Возвращать `401` anonymous client и `403` authenticated user без требуемого permission.
40. Проецировать current effective permissions в authenticated login/current-user response без добавления permission claims в cookie.
41. Отражать permissions в Web service navigation, editor route access и tag/mimic mutation controls.
42. Управлять local users через Server API: create, display-name/Enabled update и platform-hashed password reset.
43. Управлять custom security roles и полным набором user-role assignments без изменения built-in role definitions.
44. Немедленно обновлять `SecurityCatalog` после security configuration mutation.
45. Не допускать security mutation, которая оставит систему без enabled user с `Users.Manage + Roles.Manage`.

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- SQLite: Microsoft.Data.Sqlite 10.0.10.
- Modbus: NModbus 3.0.83.
- SNMP: Lextm.SharpSnmpLib 12.5.7.
- Mimic renderer: SVG в Blazor WebAssembly.
- Local password hashing: ASP.NET Core Identity `PasswordHasher<TUser>`.

## Runtime tags

`TagService` остаётся protocol-neutral:

```text
TagId
Value
Timestamp
```

`DeviceStateService` хранит общий Online/Offline state.

Мнемосхема не хранит:

```text
Modbus Address
SNMP OID
```

Она хранит только logical:

```text
TagId
```

## SQLite schema

Configuration SQLite schema version после V2-S08A:

```text
6
```

Таблицы:

```text
modbus_devices
modbus_tags
snmp_devices
snmp_tags
mimics
historian_policies
local_users
security_roles
security_role_permissions
security_user_roles
```

Существующая schema `1/2/3/4/5` автоматически мигрируется в `6` без удаления protocol/mimic/historian/user configuration.

Таблица `mimics` хранит:

```text
mimic_id
name
width
height
elements_json
```

Elements сохраняются как internal configuration JSON. Это позволяет S12 добавить editor без смены runtime API.

`local_users` хранит:

```text
user_id
user_name
normalized_user_name
display_name
enabled
password_hash
```

`normalized_user_name` уникален и используется как case-insensitive logical lookup key. Plaintext password в SQLite не сохраняется.

## Local authentication foundation

V2-S07A вводит durable identity storage/bootstrap, V2-S07B добавляет Server authentication session boundary, а V2-S07C интегрирует эту boundary в Web shell.

Local user:

```text
UserId
UserName
NormalizedUserName
DisplayName
Enabled
PasswordHash
```

Password hash создаётся и проверяется штатным ASP.NET Core Identity `PasswordHasher<TUser>`; собственная криптографическая схема не реализуется.

Bootstrap первого пользователя выполняется только когда:

```text
local_users is empty
AND
Authentication:BootstrapAdministrator:Password is explicitly configured
```

Configuration:

```json
"Authentication": {
  "BootstrapAdministrator": {
    "UserName": "admin",
    "DisplayName": "Administrator",
    "Password": ""
  }
}
```

Default password отсутствует. Для первого bootstrap startup пароль следует передавать через secret/environment configuration, например:

```text
Authentication__BootstrapAdministrator__Password
```

После появления хотя бы одного local user bootstrap больше не создаёт пользователя.

Server authentication API:

```text
POST /api/auth/login
POST /api/auth/logout
GET  /api/auth/current
```

`login` принимает:

```text
UserName
Password
```

и при успешной проверке создаёт non-persistent ASP.NET Core cookie session. Cookie:

```text
name       Dispatcher.Auth
HttpOnly   true
SameSite   Strict
lifetime   8 hours, sliding
```

`GET /api/auth/current` возвращает explicit anonymous/authenticated state:

```text
Authenticated
UserId?
UserName?
DisplayName?
EffectivePermissions[]
```

Unknown user, неверный password и disabled user не создают session и получают одинаковый `401`.

V2-S07B намеренно не добавляет role/permission claims и не закрывает существующие runtime/configuration endpoints. Server authorization появляется в V2-S08.

### Web authentication

V2-S07C добавляет scoped `AuthenticationClient` в Blazor WebAssembly. При старте Web читает:

```text
GET /api/auth/current
```

и использует Server response как источник текущего identity state. HttpOnly cookie напрямую из Web-кода не читается.

Anonymous Web state:

```text
compact Dispatcher / Вход header
login form
no service drawer
no service workspace
```

После успешного login текущий browser route сохраняется, поэтому вход с `/events`, `/history` или другого service URL возвращает пользователя в тот же контекст.

Authenticated global header показывает:

```text
DisplayName
UserName
Выйти
```

Logout вызывает existing `POST /api/auth/logout`, после чего Web возвращается к anonymous login state без client-side token/localStorage.

Скрытие рабочего shell/navigation для anonymous user является только UX boundary. На V2-S07C существующие Server endpoints ещё не получают permission enforcement; реальная server-side authorization начинается в V2-S08.

## Roles / permissions и Server authorization

V2-S08A добавил durable authorization configuration, V2-S08B включил permission-based Server enforcement для REST и SignalR без role-name checks, а V2-S08C проецирует current effective permissions в Web и использует их только как UX state.

Configuration SQLite schema `v5 → v6` добавляет:

```text
security_roles
security_role_permissions
security_user_roles
```

Начальный permission catalog:

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

Built-in roles являются system-managed definitions:

```text
Viewer        → Runtime.Read
Operator      → Runtime.Read, Tags.Write, Alarms.Acknowledge
Engineer      → operational/configuration engineering permissions, без Users.Manage/Roles.Manage
Administrator → all declared permissions
```

Первый bootstrap user получает built-in `Administrator`. При migration существующей V2-S07 installation с ровно одним local user и ещё без role assignments этот единственный user также получает `Administrator` как one-time compatibility bridge. Если существующих users несколько, Server не угадывает администратора автоматически.

`SecurityCatalog` строит effective permissions как union permissions всех назначенных roles. Disabled user имеет пустой effective permission set. Cookie по-прежнему содержит только identity claims; role/permission claims не копируются в session ticket. Successful login и `GET /api/auth/current` возвращают current `EffectivePermissions[]` как Server-projected access state для Web.

V2-S08B добавляет ASP.NET Core authorization policies/requirements, которые на каждом protected request читают `UserId` из authenticated principal и проверяют current `SecurityCatalog`.

Server permission matrix:

```text
GET/HEAD runtime/configuration/history/events/mimics → Runtime.Read
RuntimeHub connection                              → Runtime.Read
POST /api/tags/{tagId}/write                       → Tags.Write
Modbus/SNMP configuration mutations                → Devices.Edit
Mimic configuration mutations                      → Mimics.Edit
Historian policy mutations                         → Historian.Configure
```

`/health` и `/api/auth/login|logout|current` остаются public boundaries. Anonymous request к protected boundary получает `401`; authenticated user без требуемого permission — `403`. Неизвестная non-read `/api` mutation fail-closed и не проходит без явно определённой permission mapping.

V2-S08C использует `AuthenticationClient.HasPermission(...)` поверх Server-projected `EffectivePermissions[]`. Web mapping:

```text
Monitoring / Mimics runtime / History / Events → Runtime.Read
Device Editor                                → Runtime.Read + Devices.Edit
Mimic Editor                                 → Runtime.Read + Mimics.Edit
Monitoring tag write                         → Tags.Write
Mimic Button command                         → Tags.Write
```

Недоступные editor services не показываются в navigation и не рендерятся при direct route; writable tag controls без `Tags.Write` заменяются read-only marker, а mimic commands становятся disabled. Это только UX projection: REST/SignalR enforcement V2-S08B остаётся окончательной security authority.

## Users / Roles management API

V2-S09A добавляет Server management boundary поверх уже существующей configuration SQLite schema `6`; новая таблица или migration не требуется.

Endpoints:

```text
GET  /api/security/users
POST /api/security/users
PUT  /api/security/users/{userId}
PUT  /api/security/users/{userId}/password
PUT  /api/security/users/{userId}/roles

GET    /api/security/roles
POST   /api/security/roles
PUT    /api/security/roles/{roleId}
DELETE /api/security/roles/{roleId}
```

Permission boundary:

```text
user list/create/profile/Enabled  → Users.Manage
user role assignments             → Roles.Manage
password reset                    → Users.Manage + Roles.Manage
role list/CRUD                     → Roles.Manage
```

`UserName` после создания immutable; профиль меняет только `DisplayName` и `Enabled`. `GET /api/auth/current` теперь проецирует актуальные `UserName/DisplayName` из durable user record по cookie `UserId`, поэтому profile change не требует нового login для обновления identity metadata в Web. Новый password проверяется по тем же limits `12..256` и хешируется штатным `PasswordHasher<LocalUserConfiguration>`; plaintext/hash не возвращаются public API.

Built-in roles остаются system-managed и не изменяются management API. Custom role хранит только известные `PermissionNames`; assigned custom role нельзя удалить до снятия assignments.

После каждой mutation, влияющей на users/roles/assignments, Server перечитывает durable security configuration и атомарно заменяет in-memory projection `SecurityCatalog`. Поэтому текущие authorization checks применяют новую конфигурацию без перевыпуска cookie.

Fail-safe invariant перед уменьшением authority:

```text
at least one Enabled user
    has Users.Manage
    AND Roles.Manage
```

Проверка выполняется по effective permissions, а не по role name. Это предотвращает случайный administrative lockout.

S09A ещё не добавляет Web admin service и actor-aware audit events; они остаются V2-S09B/V2-S09C.

## Historian foundation

V2-S01 добавляет отдельное operational storage:

```text
TagService.Changed
      ↓ TryWrite
bounded Channel<HistorySample>
      ↓ background writer
dispatcher-operational.db
```

Configuration database и operational database разделены:

```text
dispatcher.db
    configuration

dispatcher-operational.db
    high-frequency operational records
```

Operational database имеет собственную schema version:

```text
2
```

Tables:

```text
history_samples
├── sample_id
├── tag_id
├── timestamp_utc_ticks
├── value_type
└── value_text

events
├── event_id
├── timestamp_utc_ticks
├── category
├── type
├── severity
├── source
├── message
└── data_json
```

Operational schema `v1 → v2` добавляет только `events` и сохраняет существующий `history_samples`.

`HistoryValueType`:

```text
Null
Boolean
Int64
UInt64
Double
Decimal
String
Json
```

Historian подписывается на protocol-neutral `TagService.Changed`, поэтому не знает Modbus address или SNMP OID.

Callback protocol/runtime path не пишет SQLite. Он только пытается положить sample в bounded channel через `TryWrite`.

Если buffer заполнен:

- polling/runtime callback не блокируется;
- новый sample отбрасывается;
- `DroppedSampleCount` увеличивается;
- потеря не скрывается и логируется.

Background writer сохраняет samples batch-ами. При transient persistence error текущий batch не выбрасывается, а повторяется.

Начиная с V2-S02 Historian пишет только теги, для которых существует enabled policy.

Policy:

```text
TagId
Enabled
Mode
PeriodMilliseconds?
RetentionDays
```

Modes:

```text
OnChange
Periodic
```

`OnChange` сохраняет исходный timestamp `TagValue`.

`Periodic` снимает current value с заданным периодом и использует время periodic sample. Минимальный период — `100 ms`.

Policy хранится в configuration DB, а samples — в operational DB.

Configuration API:

```text
GET    /api/configuration/historian/policies
PUT    /api/configuration/historian/policies/{tagId}
DELETE /api/configuration/historian/policies/{tagId}
```

Если tag удалён/переименован, policy не удаляется автоматически. API возвращает `TagExists = false`, sampling прекращается, но retention старой истории продолжает работать.

`Enabled = false` также прекращает sampling, но retention продолжает действовать.

Retention cleanup запускается hosted service и удаляет samples старше `RetentionDays` отдельно для каждого `TagId`.

## History query API

V2-S03 добавляет read boundary поверх operational storage:

```text
GET /api/history
```

Query parameters:

```text
tagId   repeated, 1..16 values
from    required ISO-8601 timestamp
to      required ISO-8601 timestamp
order   asc | desc, default asc
limit   max points per TagId, default 1000, max 2000
```

Пример:

```text
/api/history?tagId=plc01.temperature&tagId=switch01.uptime&from=2026-08-15T10:00:00Z&to=2026-08-15T11:00:00Z&order=asc&limit=1000
```

Диапазон времени inclusive:

```text
from <= Timestamp <= to
```

Response всегда группируется по series:

```text
HistoryQueryResponse
├── From
├── To
├── Order
├── Limit
└── Series[]
    ├── TagId
    ├── Truncated
    └── Samples[]
        ├── Timestamp
        ├── ValueType
        └── ValueText
```

`Series` сохраняет порядок запрошенных `tagId`.

`limit` применяется отдельно к каждой series. Query читает `limit + 1` запись, поэтому `Truncated=true` означает, что в запрошенном диапазоне существовали дополнительные samples.

Для одинакового timestamp порядок стабилизируется internal `sample_id`, но storage ID наружу не публикуется.

Query API не требует, чтобы `TagId` существовал в текущей device configuration или historian policy. Это позволяет читать сохранённую историю удалённых/stale tags.

Public value остаётся lossless:

```text
ValueType + ValueText
```

вместо преобразования в общий JSON `number`, которое могло бы потерять точность `UInt64`/`Decimal`.

V2-S03 query API не менял schema; текущая operational schema после V2-S05 — version `2`.

## History / Trends Web

URL:

```text
/history
```

Глобальная навигация получает сервис:

```text
История / Тренды
```

Компоновка:

```text
слева  → configured/manual TagId selection
центр  → SVG trend + dense history table
справа → свойства выбранной series
сверху → presets / from / to / order / point limit / query
```

Первый Web scope:

- до `8` series одновременно;
- configured Modbus/SNMP tags в левом списке;
- ручной TagId для retained history удалённых/stale tags;
- presets `15 min / 1 h / 6 h / 24 h`;
- произвольные `from/to` через `datetime-local`;
- `100 / 500 / 1000 / 2000` points per API series;
- SVG trend без внешней chart library;
- trend отображает numeric/boolean values;
- `String/Json/Null` остаются доступны в таблице;
- до `1000` display points на series в SVG;
- до `2000` total rows в Web table;
- `Truncated` явно показывается;
- свойства выбранной series:
  - samples;
  - truncated;
  - first/last timestamp;
  - numeric count;
  - min/max.

Web display limits не меняют lossless API/storage data.

Configuration:

```json
"OperationalDatabase": {
  "Path": ""
},
"Historian": {
  "BufferCapacity": 10000,
  "BatchSize": 256,
  "PeriodicScanMilliseconds": 100,
  "RetentionCleanupIntervalMinutes": 60
},
"EventJournal": {
  "BufferCapacity": 4096,
  "BatchSize": 128
}
```

`PeriodicScanMilliseconds` допускается в диапазоне `10..100`; policy interval — `100..86400000 ms`.

Default operational database:

```text
%LOCALAPPDATA%\Dispatcher\dispatcher-operational.db
```

или `data/dispatcher-operational.db`, если LocalApplicationData недоступен.

## Event Journal

V2-S05 добавляет единый immutable operational journal до AlarmService/Audit.

Record:

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

Initial categories:

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

Initial event types:

```text
SystemStarted
SystemStopping
DeviceOnline
DeviceOffline
TagWriteSucceeded
TagWriteFailed
RuntimeConfigurationApplied
```

Event ingestion:

```text
producer
   ↓ EventJournalService.Publish
bounded Channel<EventRecord>
   ↓ background batch writer
Operational SQLite events
```

Producer path не ждёт SQLite. При заполненном buffer incoming event отбрасывается, увеличивается `DroppedEventCount` и пишется warning.

Device producer подписывается на `DeviceStateService.Changed`, но journal самостоятельно дедуплицирует status и создаёт event только при фактическом переходе:

```text
first Online / Offline
status change
```

Повторный `Online → Online` или `Offline → Offline` от очередного poll-cycle event не создаёт.

Tag write journal покрывает:

```text
success
unknown tag
protocol/tag read-only
disabled device
invalid UInt16
protocol write error
```

Configuration event создаётся после успешного runtime apply Modbus/SNMP configuration.

Event Journal не имеет update/delete API.

## Events query API

V2-S06 добавляет read-only endpoint:

```text
GET /api/events
```

Query parameters:

```text
from       required ISO-8601
to         required ISO-8601
category   optional: System | Device | Command | Configuration
severity   optional: Information | Warning | Error
source     optional exact case-sensitive source
text       optional case-sensitive substring in Type / Source / Message / DataJson
page       default 1, max 100000
limit      default 200, max 500
```

Порядок всегда newest-first:

```text
timestamp DESC
event_id DESC
```

Response:

```text
EventQueryResponse
├── Page
├── Limit
├── HasMore
└── Items[]
    ├── EventId
    ├── Timestamp
    ├── Category
    ├── Type
    ├── Severity
    ├── Source
    ├── Message
    └── DataJson
```

`HasMore` определяется чтением `limit + 1`, без `COUNT(*)`.

API остаётся immutable/read-only.

## Events Web

URL:

```text
/events
```

Компоновка:

```text
слева  → category / severity / source / text filters
центр  → time toolbar + dense event table + paging
справа → details selected event
```

Global navigation:

```text
События
```

Web поддерживает:

- presets `15 min / 1 h / 6 h / 24 h`;
- arbitrary local `from/to`;
- server-side paging `100 / 200 / 500`;
- severity/category/source/type/message columns;
- formatted `DataJson` справа;
- Live mode;
- realtime status.

Realtime использует существующий:

```text
/hubs/runtime
```

с отдельным message:

```text
EventAdded
```

Event realtime публикуется только **после успешной SQLite persistence**, поэтому Web получает настоящий `EventId`.

Исторические страницы читаются только через REST. SignalR не используется для исторического replay.

При Live mode новые matching events merge-ятся только в page `1`; на других страницах Web показывает счётчик `Новые`.

На V2-S06 ещё нет:

```text
AlarmService
Audit actor identity
event retention policy
```

Они остаются последующим security/alarm scope.

## Mimic contracts

Runtime definition:

```text
MimicDefinition
├── MimicId
├── Name
├── Width
├── Height
└── Elements[]
```

Element:

```text
ElementId
Type
X
Y
Width
Height
Text
TagId
CommandValue
```

Типы S11:

```text
Text
Rectangle
Value
Indicator
Button
```

### Text

Статическая подпись.

### Rectangle

Простая геометрия для фона/группировки.

### Value

Показывает current value указанного `TagId`.

Если runtime value ещё нет:

```text
—
```

### Indicator

Использует current value указанного `TagId`.

Active:

- `true`;
- ненулевое число;
- непустая строка кроме `0`, `false`, `off`.

Inactive:

- `false`;
- `0`;
- `null`;
- пустая строка.

Если binding отсутствует в runtime snapshot, indicator получает отдельное `missing` состояние.

### Button

Хранит:

```text
TagId
CommandValue (UInt16)
Text
```

Кнопка активна только если текущий tag существует и `Writable = true`.

Команда проходит по уже существующей цепочке:

```text
Mimic Button
    ↓
RuntimeStateClient.WriteTagAsync
    ↓
POST /api/tags/{tagId}/write
    ↓
existing write routing
```

SNMP tags read-only, поэтому button, привязанный к SNMP tag, автоматически disabled.

## Mimic API

Runtime read:

```text
GET /api/mimics
GET /api/mimics/{mimicId}
```

Минимальный configuration boundary, подготовленный для S12:

```text
PUT    /api/configuration/mimics/{mimicId}
DELETE /api/configuration/mimics/{mimicId}
```

S11 не добавляет Web-editor. PUT нужен для persistence/integration testing и является backend foundation для S12.

## Web

Глобальная навигация:

```text
Мониторинг
Редактор устройств
Мнемосхемы
История / Тренды
События
```

URL runtime:

```text
/mimics
```

Layout:

```text
слева  → список мнемосхем
центр  → SVG runtime canvas
сверху → имя / размер / SignalR / refresh
```

Runtime `/mimics` остаётся operator screen без properties panel.

Editor:

```text
/mimics/editor
```

Layout:

```text
слева  → список мнемосхем
центр  → SVG canvas
справа → свойства схемы или выбранного элемента
сверху → create / element tools / save / delete / runtime
```

Editor использует client-side draft и explicit `Сохранить`. Изменения координат и свойств не отправляются на Server до Save.

Минимальный S12 не использует drag-and-drop. Position/size редактируются численно в правой properties panel; выбор элемента выполняется кликом по canvas.

## Новая БД

Новая configuration database по-прежнему не создаёт sample devices/tags/mimics. Local user также не создаётся без явно заданного bootstrap password. V2-S08A создаёт schema version `6` и idempotently seeds built-in security roles при startup; user assignment создаётся только по bootstrap/one-time legacy transition rules.

## Roadmap v2

Подробное продолжение:

```text
docs/ROADMAP_V2.md
```

Текущий подготовленный подшаг:

```text
V2-S09A — Users/Roles management API foundation
```

Следующий шаг после локальной проверки и нового Git SHA:

```text
V2-S09B — Users/Roles Web admin service
```

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
