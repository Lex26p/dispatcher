# Архитектура Dispatcher

## 1. Состояние после V2-S10B

Application layer содержит operational services, permission-based administration и Alarm configuration editor:

```text
Protocol workers
      ↓
TagService / DeviceStateService
      ↓
REST + SignalR
      ↓
┌────────────┬──────────────────┬──────────┬───────────────┐
│ Monitoring │ History / Trends │ Events   │ Mimic runtime │
└────────────┴──────────────────┴──────────┴───────────────┘

Device Editor  → device configuration
Mimic Editor   → mimic definition
Security Admin → users / roles
Alarm Editor   → alarm definitions

Configuration SQLite
      ↓
local user identity/password hash
      ↓
LocalAuthenticationService
      ↓
ASP.NET Core cookie → HttpContext.User
      ↓
GET /api/auth/current
      ↓
identity + EffectivePermissions[]
      ↓
Web AuthenticationClient
      ├── login / current-user shell
      └── permission-aware routes/navigation/actions

Configuration SQLite v7
      ↓
roles + role permissions + user-role assignments
      ↓
SecurityCatalog → effective permissions
      ↓
PermissionRequirement / PermissionAuthorizationHandler
      ↓
REST + RuntimeHub permission enforcement

/api/security/users + /api/security/roles
      ↓ Users.Manage / Roles.Manage
SecurityManagementService
      ↓ durable mutation + reload
Configuration SQLite v7 → SecurityCatalog.ReplaceAll
```

Mimic runtime и Mimic Editor не знают protocol-specific address.

V2-S07A сохраняет local user identities/password hashes, V2-S07B добавляет login/logout/current-user и ASP.NET Core cookie session boundary, V2-S07C отображает identity state в Web, V2-S08A/B/C формируют permission vertical slice, V2-S09A/B/C добавляют Users/Roles management и actor-aware audit, V2-S10A добавляет durable alarm definitions/Server CRUD, а V2-S10B завершает configuration slice Web Alarm Editor. Server остаётся единственной authoritative authorization boundary.

## 2. Главная граница binding

Обязательная цепочка:

```text
Modbus Address / SNMP OID
           ↓ protocol configuration
         TagId
           ↓
       TagService
           ↓
      Mimic element
```

Мнемосхема сохраняет только `TagId`.

Это позволяет один и тот же runtime renderer использовать Modbus, SNMP и будущие протоколы.

## 3. Mimic configuration

Server-side model:

```text
MimicConfiguration
├── MimicId
├── Name
├── Width
├── Height
└── MimicElementConfiguration[]
```

Element:

```text
ElementId
Type
X / Y
Width / Height
Text?
TagId?
CommandValue?
```

Типы:

```text
Text
Rectangle
Value
Indicator
Button
```

Public boundary находится в `Dispatcher.Contracts.Mimics`.

## 4. Persistence

Configuration SQLite schema version:

```text
7
```

Tables, добавленные после базовой protocol configuration:

```text
mimics
historian_policies
local_users
security_roles
security_role_permissions
security_user_roles
alarm_definitions
```

Schema migration:

```text
v1
 ↓
v2  SNMP
 ↓
v3  mimics
 ↓
v4  historian_policies
 ↓
v5  local_users
 ↓
v6  security roles / permissions / assignments
 ↓
v7  alarm_definitions
```

При `v4 → v5` protocol/mimic/historian tables не перестраиваются и не очищаются. При `v6 → v7` existing protocol/mimic/historian/user/security configuration также сохраняется; добавляется только `alarm_definitions`.

Operational SQLite schema развивается независимо: V2-S05 поднял её до `2`, а V2-S09C — до `3` для nullable audit actor columns.

## 5. Почему elements_json

На S11 объём и структура mimic definition малы.

Отдельные SQL tables для каждого типа элемента сейчас не дают функциональной пользы.

`elements_json` даёт:

- atomic save одной схемы;
- простой load;
- минимальный persistence code;
- достаточную основу для S12 editor.

Если в будущем потребуется server-side query по тысячам элементов или partial SQL updates, storage strategy можно пересмотреть.

## 6. Mimic validation

Server проверяет:

- `MimicId` и `Name`;
- canvas size;
- уникальный `ElementId`;
- bounds каждого элемента;
- положительный element size;
- `TagId` для `Value`, `Indicator`, `Button`;
- `CommandValue` для `Button`;
- максимальное количество элементов.

Tag binding намеренно не обязан существовать в текущей device configuration.

Причина: logical reference может временно отсутствовать или configuration устройства может быть изменена позже.

Runtime тогда показывает missing/no-value state.

## 7. Mimic API

Runtime:

```text
GET /api/mimics
GET /api/mimics/{mimicId}
```

Configuration foundation:

```text
PUT    /api/configuration/mimics/{mimicId}
DELETE /api/configuration/mimics/{mimicId}
```

PUT выполняет create/update целого definition.

S12 использует этот boundary вместо создания нового persistence API.

## 8. Web runtime

URL:

```text
/mimics
```

Spatial model:

```text
┌──────────────┬───────────────────────────────────────────────┐
│ Mimics       │ name / dimensions / SignalR / refresh       │
│              ├───────────────────────────────────────────────┤
│ list         │                                               │
│              │               SVG canvas                      │
│              │                                               │
└──────────────┴───────────────────────────────────────────────┘
```

Правая properties panel отсутствует, потому что runtime screen не редактирует элементы.

S12 editor вернётся к стандартной схеме:

```text
left structure
center canvas
right properties
top actions/tools
```

## 9. SVG renderer

Blazor рендерит definition как `<svg viewBox>`.

Причины:

- absolute engineering coordinates без JavaScript;
- масштабирование canvas средствами SVG;
- Rectangle/Text/Indicator естественно выражаются SVG primitives;
- Button можно обработать обычным Blazor `@onclick`;
- editor S12 сможет использовать ту же coordinate model.

JS canvas на этом этапе не нужен.

## 10. Realtime binding

Mimic page использует существующий scoped:

```text
RuntimeStateClient
```

Он уже:

- загружает REST snapshot;
- держит current tags;
- принимает `TagChanged` через SignalR;
- переподключается;
- обновляет snapshot после reconnect.

Mimic page подписывается только на `RuntimeStateClient.Changed`.

Отдельный SignalR hub для мнемосхем не создаётся.

## 11. Value

`Value` находит current `TagValueDto` по `TagId`.

Если значения нет:

```text
—
```

Никакая protocol-specific логика в renderer не выполняется.

## 12. Indicator

Indicator имеет три visual runtime состояния:

```text
active
inactive
missing
```

Базовая truthiness S11:

```text
true                 → active
non-zero number      → active
other nonempty text  → active

false                → inactive
0                    → inactive
empty / null         → inactive

missing TagValue     → missing
```

Более сложные expressions/conditions пока не вводятся.

## 13. Button

Button definition:

```text
TagId
CommandValue UInt16
Text
```

Runtime enable rule:

```text
TagValue exists
AND Writable == true
AND CommandValue exists
```

Command path:

```text
Button
 ↓
RuntimeStateClient.WriteTagAsync
 ↓
POST /api/tags/{tagId}/write
 ↓
existing Modbus write routing
```

S11 не создаёт новый command bus.

SNMP tag автоматически read-only, поэтому SNMP-bound Button disabled.

## 14. Configuration separation

Mimic change не требует restart protocol polling.

Поэтому mimic persistence использует отдельный `MimicConfigurationService` и отдельный mutation lock.

Он не входит в `RuntimeConfigurationCoordinator`.

Это сохраняет разницу:

```text
device protocol configuration change → protocol runtime apply
mimic definition change              → save definition only
```

## 15. Mimic Editor

URL:

```text
/mimics/editor
```

Spatial model:

```text
┌──────────────┬──────────────────────────────────────┬───────────────┐
│ Mimics       │ element tools / Save / Runtime      │ Properties    │
│              ├──────────────────────────────────────┤               │
│ list         │                                      │ mimic/element │
│              │             SVG canvas               │               │
│              │                                      │               │
└──────────────┴──────────────────────────────────────┴───────────────┘
```

Editor загружает тот же `MimicDefinitionDto`, который исполняет runtime.

Рабочая модель:

```text
GET definition
      ↓
client-side MimicDraft
      ↓
edit locally
      ↓
explicit Save
      ↓
PUT whole MimicDefinitionDto
      ↓
SQLite mimics.elements_json
```

Изменение mimic definition не затрагивает protocol polling и не вызывает `RuntimeConfigurationCoordinator`.

## 16. Tag picker

Editor получает configured tags из существующих:

```text
GET /api/configuration/modbus/devices
GET /api/configuration/snmp/devices
```

и объединяет только их logical `TagId`.

В mimic definition по-прежнему сохраняется только строка `TagId`.

Если persisted definition содержит binding на уже удалённый tag, editor сохраняет этот ID в selector, чтобы пользователь мог увидеть и исправить stale binding.

## 17. Минимальная coordinate editing model

S12 не добавляет drag-and-drop.

Выбор элемента:

```text
click SVG element
```

Редактирование:

```text
X
Y
Width
Height
```

в правой properties panel.

Причина: это закрывает функциональный scope editor и сохраняет одну coordinate model с S11 runtime без JavaScript interaction layer. Drag/resize handles могут быть добавлены позднее, если подтвердится необходимость.

## 18. Завершение базового roadmap

S00–S12 дают законченный вертикальный срез:

```text
protocol configuration
        ↓
Modbus / SNMP polling
        ↓
logical runtime tags
        ↓
Monitoring
        ↓
Mimic definition/editor/runtime
```

Phase 5 выбирается после оценки эксплуатации, а не проектируется заранее.

## 19. Historian operational boundary

V2-S01 добавляет долговременное хранение runtime tag changes без изменения protocol drivers.

Цепочка:

```text
Modbus / SNMP
      ↓
TagService.Set(...)
      ↓
TagService.Changed
      ├────────────→ RuntimeHubPublisher
      └────────────→ HistorianService
                          ↓
                    bounded Channel
                          ↓
                    background writer
                          ↓
                 Operational SQLite
```

Historian подписывается только на `TagService.Changed` и не знает protocol-specific addressing.

## 20. Configuration DB и operational DB разделены

Configuration database остаётся:

```text
dispatcher.db
```

и продолжает хранить низкочастотную configuration:

```text
devices
tags
mimics
historian policies
local users
```

Operational database:

```text
dispatcher-operational.db
```

хранит:

```text
history samples
events
```

Причина разделения:

- history/event volume и write rate отличаются от configuration;
- configuration lifecycle не должен зависеть от роста operational data;
- security identities являются низкочастотной configuration, а не operational journal;
- будущая замена historian/events storage не должна требовать переноса device/mimic/security configuration.

Обе базы пока используют `Microsoft.Data.Sqlite`, но имеют независимые schema versions.

## 21. Operational SQLite schema

V2-S01 создал operational schema v1:

```text
history_samples
├── sample_id              INTEGER PK AUTOINCREMENT
├── tag_id                 TEXT
├── timestamp_utc_ticks    INTEGER
├── value_type             INTEGER
└── value_text             TEXT NULL
```

Index:

```text
(tag_id, timestamp_utc_ticks, sample_id)
```

`sample_id` обеспечивает однозначный порядок records даже при одинаковом timestamp.

V2-S05 поднял operational schema до `2`, а V2-S09C добавляет actor identity и делает current version `3`:

```text
PRAGMA user_version = 3
```

Migration:

```text
v1 history_samples
 ↓ preserve history
v2 history_samples + events
 ↓ preserve history/events
v3 events + nullable actor identity
```

Неизвестная future schema version вызывает startup error вместо неявной попытки использовать несовместимую DB.

## 22. Typed history value

Historian не сохраняет protocol/library objects.

До persistence runtime value нормализуется в:

```text
HistoryValueType
ValueText
```

Поддерживаемые категории:

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

Каноническое строковое хранение сохраняет:

- `UInt64`, который может не помещаться в SQLite signed INTEGER;
- `decimal` без принудительного преобразования в binary floating point;
- invariant numeric representation.

Неизвестный CLR object сериализуется как JSON и получает `HistoryValueType.Json`.

Timestamp нормализуется в UTC и хранится как ticks.

## 23. Bounded asynchronous ingestion

`TagService.Changed` является synchronous callback и может исполняться непосредственно в protocol polling path.

Поэтому callback Historian не выполняет:

```text
SQLite open
transaction
disk write
retry
```

Он выполняет только:

```text
TagValue
   ↓ normalize
HistorySample
   ↓ Channel.Writer.TryWrite
return
```

Channel bounded.

Baseline V2-S01:

```text
BufferCapacity = 10000
BatchSize      = 256
```

оба значения configurable через `Historian`.

При заполненном buffer `TryWrite` возвращает `false`.

Текущий incoming sample отбрасывается, а:

```text
DroppedSampleCount++
warning log
```

Polling при этом не блокируется.

Это baseline overflow behavior. Policies/retention и дальнейшая tuning выполняются в V2-S02.

## 24. Background writer и retry

Один reader собирает batch до `BatchSize`.

Batch сохраняется одной SQLite transaction.

Если persistence временно падает:

```text
same batch
   ↓
retry after delay
```

Batch не считается успешно обработанным до завершения `AppendAsync`.

Пока writer занят retry, bounded buffer может заполниться; такие потери отражаются через `DroppedSampleCount`, а не скрываются.

## 25. Startup order

Hosted services регистрируются в порядке:

```text
ConfigurationInitializationHostedService
HistorianService
ModbusRuntimeHostedService
SnmpRuntimeHostedService
RuntimeHubPublisher
```

Configuration initialization начиная с V2-S08A:

1. создаёт/мигрирует configuration schema;
2. при пустом `local_users` пытается выполнить explicit bootstrap пользователя; если user создаётся, его `Administrator` assignment сохраняется в той же SQLite transaction;
3. idempotently приводит built-in role definitions к canonical mapping и выполняет one-time legacy owner transition при необходимости;
4. загружает protocol configuration, historian policies и security configuration;
5. публикует active configuration catalogs, включая `SecurityCatalog`.

Historian:

1. создаёт/проверяет operational schema;
2. подписывается на `TagService.Changed`;
3. запускает writer;
4. только затем начинают стартовать protocol polling services.

Это исключает нормальное startup-окно, в котором первый successful poll мог бы пройти до Historian subscription.

## 26. V2-S01 scope boundary

V2-S01 создал bounded asynchronous ingestion и operational SQLite.

V2-S02 поверх этой границы добавляет policy-driven sampling и retention.

History query API и Trend Web по-прежнему остаются V2-S03/V2-S04.

## 27. Historian policy configuration

V2-S02 добавляет в configuration database:

```text
historian_policies
├── tag_id
├── enabled
├── mode
├── period_ms
└── retention_days
```

Configuration schema version на этом шаге стала:

```text
4
```

V2-S07A позже повышает общую configuration schema до `5`, не изменяя layout `historian_policies`.

Policy не имеет SQL foreign key на protocol tag tables.

Причина:

- logical `TagId` глобален между протоколами;
- Modbus/SNMP configuration заменяется whole-snapshot операциями;
- policy должна пережить временное удаление/переименование тега как явный stale binding;
- автоматический cascade delete уничтожил бы retention configuration и скрыл бы факт сломанного binding.

`HistorianPolicyCatalog` является thread-safe in-memory snapshot для hot runtime path.

Startup:

```text
SqliteConfigurationStore.Initialize
        ↓
load devices/tags
load historian policies
        ↓
ConfigurationCatalog
HistorianPolicyCatalog
        ↓
HistorianService start
```

## 28. Policy API и live apply

Configuration boundary:

```text
GET    /api/configuration/historian/policies
PUT    /api/configuration/historian/policies/{tagId}
DELETE /api/configuration/historian/policies/{tagId}
```

Новая policy создаётся только для существующего configured `TagId`.

Если policy уже существует, а tag позже удалён, policy остаётся доступной для update/delete.

DTO возвращает:

```text
TagExists
```

чтобы stale binding был явным.

Policy mutation:

```text
validate
   ↓
SQLite upsert/delete
   ↓
HistorianPolicyCatalog.ReplaceAll
   ↓
HistorianService sees new snapshot
```

Protocol polling не перезапускается.

## 29. Sampling modes

### OnChange

```text
TagService.Changed
      ↓
find enabled policy
      ↓ mode == OnChange
      ↓ configured TagId still exists
      ↓
bounded channel
```

Сохраняется timestamp исходного `TagValue`.

### Periodic

Policy требует:

```text
100 ms <= PeriodMilliseconds <= 24 h
```

Отдельный lightweight periodic sampler сканирует enabled periodic policies.

Scan interval configurable как `Historian:PeriodicScanMilliseconds` в диапазоне `10..100 ms`, поэтому scan granularity не может быть медленнее минимального policy interval.

Когда policy due:

```text
ConfigurationCatalog.ContainsTagId
        ↓
TagService.Get(TagId)
        ↓
current Value
        ↓
HistorySample(timestamp = sample time)
        ↓
bounded channel
```

Periodic mode не создаёт catch-up burst после задержки. Следующий due рассчитывается от фактического текущего scan time.

Если current value ещё отсутствует, sample пропускается до следующего периода.

Deadband в V2-S02 не добавляется.

## 30. Stale/disabled policy semantics

Отсутствие policy:

```text
no sampling
no automatic retention cleanup for that TagId
```

`Enabled=false`:

```text
sampling off
retention still active
```

Stale policy (`TagExists=false`):

```text
sampling off
retention still active
policy remains manageable
```

При rename старый `TagId` не переносит policy автоматически на новый `TagId`.

Это требует явного инженерного решения и не создаёт скрытый change of archival identity.

Удаление policy не удаляет накопленные samples и прекращает automatic retention для этого `TagId`.

## 31. Retention cleanup

`HistorianRetentionHostedService` запускается после Historian operational store initialization.

Default interval:

```text
60 minutes
```

Для каждой сохранённой policy:

```text
cutoff = now UTC - RetentionDays
        ↓
DELETE history_samples
WHERE tag_id = policy.TagId
  AND timestamp < cutoff
```

Retention применяется независимо от `Enabled` и `TagExists`.

Diagnostics:

```text
CleanupRunCount
DeletedSampleCount
```

Ошибка cleanup логируется и не останавливает основной protocol/Historian ingestion runtime.

Retention по-прежнему использует существующий индекс `(tag_id, timestamp_utc_ticks, sample_id)` и не требует изменения layout `history_samples`. V2-S05 повышает operational schema до `2` только для добавления Event Journal.

## 32. V2-S02 scope boundary

После V2-S02 есть:

```text
persistent policy
OnChange
Periodic
retention
configuration API
live apply
```

Ещё нет:

```text
History query API
public history DTO
Trend Web
```

Следующий шаг V2-S03 вводит read/query boundary поверх существующего operational storage.

## 33. History read boundary

V2-S03 добавляет protocol-neutral read API поверх operational storage:

```text
Operational SQLite
      ↓
IHistorySampleStore.QueryAsync
      ↓
HistoryQueryService
      ↓
GET /api/history
      ↓
Dispatcher.Contracts.Historian
```

Query не обращается к protocol drivers и не преобразует `TagId` обратно в Modbus address/SNMP OID.

## 34. History query contract

Endpoint:

```text
GET /api/history
```

Query:

```text
tagId   repeated, required
from    required
to      required
order   asc | desc
limit   points per series
```

Limits:

```text
1 <= tag count <= 16
1 <= limit <= 2000
default limit = 1000
```

Таким образом один request возвращает не более:

```text
16 × 2000 = 32000 samples
```

Большие time ranges допускаются, потому что SQL query всё равно ограничен index-backed `LIMIT`.

`from` и `to` inclusive.

## 35. Multi-tag response format

Ответ не смешивает samples разных tags в один неявный поток:

```text
HistoryQueryResponseDto
├── From
├── To
├── Order
├── Limit
└── Series[]
    ├── TagId
    ├── Truncated
    └── Samples[]
```

Порядок `Series` совпадает с порядком repeated `tagId` query parameters.

Для запрошенного tag без samples возвращается empty series, а не пропускается весь TagId.

Duplicate `tagId` считается invalid query и возвращает `400`.

## 36. Stable sample order

Operational index уже существует:

```text
(tag_id, timestamp_utc_ticks, sample_id)
```

Ascending query:

```text
ORDER BY timestamp_utc_ticks ASC, sample_id ASC
```

Descending query:

```text
ORDER BY timestamp_utc_ticks DESC, sample_id DESC
```

`sample_id` используется только как internal deterministic tie-breaker, когда несколько samples имеют одинаковый timestamp.

Storage ID не входит в public contract.

## 37. Per-series limit и Truncated

`limit` означает maximum returned points **для каждого TagId**.

Для определения truncation storage вызывается с:

```text
limit + 1
```

Если прочитано больше `limit`:

```text
Truncated = true
return first limit samples
```

Полный `COUNT(*)` не выполняется, потому что first Web trend scope должен знать только факт существования дополнительных points, а не точное total count.

## 38. Lossless public history value

Public sample:

```text
Timestamp
ValueType
ValueText
```

`ValueText` сохраняет canonical representation V2-S01 без дополнительного преобразования в общий JSON numeric type.

Причина:

- `UInt64` может превышать безопасный range некоторых JSON consumers;
- `Decimal` не должен терять precision;
- `Json` должен сохранять исходный raw payload;
- `Null/String/Boolean` остаются однозначны благодаря `ValueType`.

V2-S04 Web преобразует `ValueText` для trend rendering только там, где `ValueType` является numeric.

## 39. Query не зависит от current configuration

History query intentionally не проверяет:

```text
ConfigurationCatalog.ContainsTagId
HistorianPolicyCatalog.Contains
```

Причина: retained operational history должна оставаться читаемой после:

```text
tag delete
tag rename
policy delete
policy stale
```

Current configuration определяет future sampling, но не право существования уже сохранённых samples.

## 40. V2-S03 scope boundary

После V2-S03 есть:

```text
Historian storage
OnChange / Periodic policies
Retention
History REST query
multi-tag response
bounded response size
```

Ещё нет:

```text
History / Trends Web
saved trend selections
aggregation/downsampling
historian realtime stream
```

Следующий шаг V2-S04 строит Web UI поверх `GET /api/history` без изменения protocol/Historian ingestion boundary.

## 41. History / Trends Web service

V2-S04 добавляет Web screen:

```text
/history
```

и не меняет Historian Server/storage boundary.

Цепочка:

```text
Configuration API
    ↓ configured TagId list

GET /api/history
    ↓
HistoryClient
    ↓
History.razor
    ├── SVG trend
    ├── dense table
    └── series properties
```

Global navigation получает:

```text
История / Тренды
```

## 42. History Web spatial model

Экран следует общему engineering UI contract:

```text
┌──────────────┬────────────────────────────────────────┬──────────────┐
│ Tags         │ time range / order / limit / query     │ Series       │
│              ├────────────────────────────────────────┤ properties   │
│ selection    │ SVG trend                              │              │
│              ├────────────────────────────────────────┤              │
│              │ dense history table                    │              │
└──────────────┴────────────────────────────────────────┴──────────────┘
```

Left panel:

- configured Modbus/SNMP tags;
- text filter;
- manual TagId entry.

Manual TagId нужен для retained operational history, которая может остаться после удаления current configuration.

Center:

- query toolbar;
- query summary;
- trend;
- table.

Right:

- selected series metadata/statistics.

## 43. Web series selection limits

Server V2-S03 допускает:

```text
16 tags × 2000 samples
```

Но первый desktop Web screen ограничивает selection:

```text
MaxSelectedTags = 8
```

Причина: SVG и table rendering выполняются в browser WASM и должны оставаться предсказуемыми без virtualization/chart library.

API contract не меняется и по-прежнему допускает 16 tags для других clients.

## 44. SVG trend без chart library

Первый trend renderer использует SVG:

```text
polyline
line
rect
foreignObject labels
```

External chart dependency не добавляется.

Numeric trend поддерживает:

```text
Boolean
Int64
UInt64
Double
Decimal
```

`Boolean` визуализируется как `0/1`.

`String`, `Json`, `Null` не строятся как line series, но остаются доступны в history table.

Преобразование numeric `ValueText → double` выполняется только для visual plotting/statistics.

Lossless API/storage representation не меняется.

Для `UInt64`/`Decimal` chart является display approximation; таблица остаётся источником точного canonical value.

## 45. Client display caps

API может вернуть до `2000` samples на series.

Trend SVG ограничивает display до:

```text
1000 points per series
```

Если API вернул больше, Web выбирает равномерно распределённые source points только для SVG rendering.

Это display reduction, а не server-side aggregation и не изменение stored data.

Dense table ограничена:

```text
2000 total rows
```

и явно показывает:

```text
displayed / total
```

Если API series имеет:

```text
Truncated = true
```

это также показывается отдельно.

Эти ограничения уменьшают риск тяжёлого DOM/SVG rendering до появления подтверждённой необходимости в virtualization/downsampling engine.

## 46. Time range semantics в Web

Toolbar предоставляет presets:

```text
15 min
1 h
6 h
24 h
```

и `datetime-local` поля `from/to`.

User-entered values трактуются в browser local timezone и передаются `HistoryClient` как `DateTimeOffset`, после чего query serializes UTC ISO-8601.

Response timestamps отображаются в local time.

Server inclusive semantics V2-S03 сохраняется:

```text
from <= sample.Timestamp <= to
```

## 47. Series properties

Selected series properties включают:

```text
TagId
Samples
Truncated
First sample
Last sample
Numeric point count
Distinct value type count
Min
Max
```

`Min/Max` считаются только по numeric/boolean points, которые могут быть plotted.

Selection синхронизируется между:

```text
legend
table rows
right properties
```

## 48. V2-S04 scope boundary

После V2-S04 Historian Phase 5 завершает первый end-to-end slice:

```text
TagService
   ↓
Historian ingestion
   ↓
policy / retention
   ↓
operational SQLite
   ↓
history query API
   ↓
History / Trends Web
```

Не реализуются пока:

```text
saved trend selections
realtime history stream
server aggregation
server downsampling
CSV export
advanced cursors/zoom
```

Следующий шаг V2-S05 начинает отдельную Phase 6 — Event Journal.

## 49. Event Journal operational boundary

V2-S05 добавляет второй тип operational records рядом с history:

```text
dispatcher-operational.db
├── history_samples
└── events
```

Event Journal не использует configuration database.

Цепочка:

```text
system/device/command/configuration producer
        ↓
EventJournalService.Publish(...)
        ↓ TryWrite
bounded Channel<EventRecord>
        ↓
background batch writer
        ↓
SqliteOperationalStore
        ↓
events
```

Producer path не ждёт SQLite transaction/disk I/O.

## 50. Event record

Internal event model:

```text
EventRecord
├── EventId
├── Timestamp
├── Category
├── Type
├── Severity
├── Source
├── Message
└── DataJson
```

`EventId` создаётся SQLite `AUTOINCREMENT`.

Timestamp нормализуется в UTC ticks.

Categories V2-S05:

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

`Type` остаётся строковым stable identifier:

```text
SystemStarted
SystemStopping
DeviceOnline
DeviceOffline
TagWriteSucceeded
TagWriteFailed
RuntimeConfigurationApplied
```

Это позволяет V2-S06 фильтровать события без смешивания human-readable `Message` с machine-readable event type.

`DataJson` содержит producer-specific structured details и nullable.

## 51. Operational SQLite schema v2

V2-S05 migration:

```text
v1
├── history_samples
└── ix_history_samples_tag_time

       ↓

v2
├── history_samples
├── events
├── ix_history_samples_tag_time
├── ix_events_time
├── ix_events_category_time
├── ix_events_severity_time
└── ix_events_source_time
```

Existing history table/records не перестраиваются и не очищаются.

`events`:

```text
event_id             INTEGER PK AUTOINCREMENT
timestamp_utc_ticks  INTEGER
category             INTEGER
type                 TEXT
severity             INTEGER
source               TEXT
message              TEXT
data_json             TEXT NULL
```

Store API Event Journal является append/read-only foundation:

```text
AppendEventsAsync
LoadAllEventsAsync
```

Update/delete event methods отсутствуют.

V2-S06 добавит query boundary, но не должен превращать immutable journal в editable collection.

## 52. Bounded asynchronous Event Journal

Baseline:

```text
BufferCapacity = 4096
BatchSize      = 128
```

настраивается через:

```text
EventJournal:BufferCapacity
EventJournal:BatchSize
```

При full buffer:

```text
TryWrite == false
    ↓
drop incoming event
    ↓
DroppedEventCount++
    ↓
warning
```

Current batch при SQLite error retry-ится background writer-ом.

Это сохраняет важную границу:

```text
protocol/device state callback
command/configuration request
        ≠
SQLite wait
```

`RejectedEventCount` учитывает event payload, который не удалось сериализовать в `DataJson`.

## 53. Initial Event producers

### System lifecycle

`EventJournalService.StartAsync`:

```text
SystemStarted
```

`StopAsync` перед завершением channel:

```text
SystemStopping
```

Shutdown event best-effort в рамках normal graceful shutdown и flush channel.

### Device status

`DeviceStateService.Changed` вызывается при каждом `SetOnline/SetOffline`, включая повторный status на очередном poll-cycle.

Event Journal поэтому держит собственный last-status snapshot:

```text
first observed Online/Offline → event
Online → Online               → no event
Offline → Offline             → no event
Online → Offline              → event
Offline → Online              → event
```

Это не меняет `DeviceStateService`; deduplication принадлежит event producer.

`DeviceOffline` имеет `Warning`, `DeviceOnline` — `Information`.

### Tag write

Existing `/api/tags/{tagId}/write` публикует:

```text
TagWriteSucceeded
TagWriteFailed
```

Failure включает текущие branches:

```text
tag not found
protocol read-only
device disabled
tag read-only
invalid UInt16
protocol error
```

Event publishing не меняет HTTP status semantics существующего write API.

### Configuration

Каждая успешная Modbus/SNMP mutation всё ещё проходит:

```text
persist
ConfigurationCatalog
RuntimeConfigurationCoordinator.ApplyAsync
```

После успешного runtime apply создаётся:

```text
RuntimeConfigurationApplied
```

Если apply бросил exception, event `Applied` не создаётся.

V2-S05 намеренно фиксирует runtime device configuration apply как первый configuration producer. Более детальный audit actor/action model появится в Users/Roles/Audit phase.

## 54. V2-S05 scope boundary

После V2-S05 есть:

```text
immutable operational events
system lifecycle producer
device transition producer
tag write producer
runtime configuration producer
bounded async persistence
operational DB migration v1 → v2
```

Ещё нет:

```text
Events REST query
Events Web
Events SignalR
audit actor identity
AlarmService
alarm transitions
event retention
```

Следующий шаг V2-S06 добавляет read/query + Web UI поверх Event Journal.

## 55. Events read/query boundary

V2-S06 добавляет read-only boundary поверх immutable Event Journal:

```text
Operational SQLite events
        ↓
IEventJournalStore.QueryEventsAsync
        ↓
EventQueryService
        ↓
GET /api/events
        ↓
Dispatcher.Contracts.Events
```

Public DTO:

```text
EventRecordDto
├── EventId
├── Timestamp
├── Category
├── Type
├── Severity
├── Source
├── Message
└── DataJson
```

Public enums сериализуются строками:

```text
EventCategoryDto
EventSeverityDto
```

Server не публикует internal integer enum codes как application contract.

## 56. Events query semantics

Endpoint:

```text
GET /api/events
```

Required:

```text
from
to
```

Optional:

```text
category
severity
source
text
```

Paging:

```text
page  = 1..100000
limit = 1..500
default limit = 200
```

Time range inclusive:

```text
from <= Timestamp <= to
```

Ordering фиксирован как:

```text
timestamp_utc_ticks DESC
event_id DESC
```

Поэтому журнал отображается newest-first, а `event_id` стабилизирует порядок records с одинаковым timestamp.

`source` — exact case-sensitive match.

`text` использует SQLite `instr` и ищет case-sensitive Unicode substring в:

```text
type
source
message
data_json
```

Paging на первом scope реализован через `LIMIT/OFFSET`.

Для определения следующей страницы query запрашивает:

```text
limit + 1
```

и возвращает:

```text
HasMore
```

Полный `COUNT(*)` не выполняется.

## 57. Persisted-event realtime

V2-S05 producer записывал `EventRecord(EventId=0)` в bounded channel.

V2-S06 меняет persistence boundary:

```text
AppendEventsAsync
    ↓ SQLite INSERT
SELECT last_insert_rowid()
    ↓
persisted EventRecord with real EventId
```

После successful transaction `EventJournalService` публикует internal notification:

```text
Persisted(EventRecord)
```

Notification не вызывается до successful persistence.

Отдельный hosted service:

```text
EventHubPublisher
```

подписывается на `Persisted` и отправляет через существующий hub:

```text
/hubs/runtime
RuntimeHubContract.EventAdded
```

Цепочка:

```text
producer
   ↓
EventJournal channel
   ↓
SQLite commit
   ↓
Persisted
   ↓
EventHubPublisher
   ↓
SignalR EventAdded
   ↓
EventClient
```

Это исключает состояние, когда Web показывает event, которого ещё нет в operational database.

SignalR не используется для historical replay.

## 58. Events Web service

Web route:

```text
/events
```

Spatial model:

```text
┌──────────────┬───────────────────────────────────────────┬───────────────┐
│ filters      │ time toolbar / realtime state             │ event details │
│ category     ├───────────────────────────────────────────┤ DataJson      │
│ severity     │ dense event table                         │               │
│ source/text  ├───────────────────────────────────────────┤               │
│              │ server paging                             │               │
└──────────────┴───────────────────────────────────────────┴───────────────┘
```

Left filters:

```text
Category
Severity
Source
Text
```

Center:

```text
15 min / 1 h / 6 h / 24 h
from / to
100 / 200 / 500 rows
Refresh
Live
realtime connection state
dense table
Previous / Next
```

Right properties:

```text
EventId
Timestamp
Category
Type
Severity
Source
Message
pretty DataJson
```

## 59. Live-mode semantics

`EventClient` использует отдельную hub connection, но тот же:

```text
RuntimeHubContract.Path
```

Это не смешивает event query state с `RuntimeStateClient` tag/device snapshots.

В `Live` mode:

```text
page == 1
matching EventAdded
    ↓
merge by EventId
    ↓
sort Timestamp DESC / EventId DESC
    ↓
keep current Web limit
```

Если user находится на historical page или Live выключен, matching realtime events не меняют текущую страницу, а увеличивают:

```text
Новые: N
```

Нажатие `Новые` возвращает page 1, двигает `to` к current local time и перечитывает диапазон через REST.

Таким образом:

```text
historical truth → REST
new notification → SignalR
```

Reconnect не является historical replay; оператор всегда может выполнить refresh REST.

## 60. V2-S06 scope boundary

После V2-S06 Phase 6 завершена:

```text
Event Journal persistence
device/command/config/system producers
Events REST filters
server-side paging
persisted-event SignalR
Events Web
```

Operational schema остаётся:

```text
2
```

Не реализуются на V2-S06:

```text
event retention policy
audit actor identity
AlarmService
alarm transitions
```

Следующий security phase начинается V2-S07.

## 61. Local user configuration boundary

V2-S07A добавляет local user identity как низкочастотную configuration:

```text
dispatcher.db
└── local_users
```

Таблица:

```text
local_users
├── user_id                 TEXT PRIMARY KEY
├── user_name               TEXT
├── normalized_user_name    TEXT UNIQUE
├── display_name            TEXT
├── enabled                 INTEGER 0/1
└── password_hash           TEXT
```

Configuration schema повышается:

```text
v4 → v5
```

Operational database не меняется:

```text
dispatcher-operational.db
PRAGMA user_version = 2
```

Причина: user identity/password hash/disabled state являются durable security configuration, а не временными operational records.

## 62. Username identity

Public login spelling хранится как:

```text
UserName
```

Lookup identity хранится отдельно:

```text
NormalizedUserName = UserName.Trim().ToUpperInvariant()
```

`normalized_user_name` имеет SQL `UNIQUE` constraint.

Это даёт одну local identity для case-вариантов имени и не требует зависеть от SQLite locale/collation semantics.

`UserId` остаётся отдельным immutable identifier и не равен username.

## 63. Password storage

Dispatcher не реализует собственный password KDF/hash format.

Цепочка bootstrap hashing:

```text
plaintext bootstrap secret
        ↓
ASP.NET Core Identity PasswordHasher<LocalUserConfiguration>
        ↓
PasswordHash
        ↓
SQLite local_users.password_hash
```

В persistent user record отсутствует plaintext password.

Password verification в V2-S07B использует тот же platform hasher и его encoded hash metadata вместо собственного salt/iteration format.

## 64. Bootstrap первого local user

Bootstrap выполняется во время configuration initialization до protocol polling.

Условие:

```text
LoadLocalUsersAsync().Count == 0
AND
Authentication:BootstrapAdministrator:Password is non-empty
```

Тогда создаётся один enabled local user.

Defaults только для identity metadata:

```text
UserName    = admin
DisplayName = Administrator
```

Default password **не существует**.

Bootstrap password ожидается через configuration provider/secret/environment variable. Он не записывается в configuration SQLite.

После появления хотя бы одного local user bootstrap больше не создаёт пользователей, даже если bootstrap password остался в process configuration.

Если users пусты и password не задан, Server продолжает запускаться и пишет warning. После V2-S07B такой host остаётся anonymous-only: `current` работает, но login не может завершиться успешно до явного bootstrap первого local user.

## 65. Disabled user semantics на V2-S07A

`Enabled` сохраняется как часть local user configuration.

На V2-S07A это только durable state:

```text
Enabled = true / false
```

V2-S07B использует этот flag при login: disabled user не может создать новую authenticated session.

V2-S07A намеренно не добавляет:

```text
role
permissions
is_admin authorization bypass
audit actor identity
```

Название bootstrap administrator описывает bootstrap intent, но не создаёт скрытого authorization privilege. Permission model появляется только в V2-S08.

## 66. V2-S07A scope boundary

После V2-S07A есть:

```text
persistent local user identity
configuration schema v5
unique normalized username
platform password hash
explicit first-user bootstrap
persistent disabled flag
```

Ещё нет:

```text
login
logout
current user
authenticated cookie/session
server authorization
roles/permissions
audit actor identity
authentication Web UI
```

Следующий шаг:

```text
V2-S07B — Server authentication session, login/logout/current user
```


## 67. Local authentication request boundary

V2-S07B добавляет HTTP authentication boundary поверх existing `local_users` без изменения configuration schema:

```text
POST /api/auth/login
POST /api/auth/logout
GET  /api/auth/current
```

Login path:

```text
UserName + Password
        ↓
NormalizeUserName
        ↓
SqliteConfigurationStore.FindLocalUserByNormalizedUserNameAsync
        ↓
PasswordHasher<LocalUserConfiguration>.VerifyHashedPassword
        ↓
Enabled == true
        ↓
ASP.NET Core SignInAsync
        ↓
Dispatcher.Auth cookie
```

Unknown user, неверный password и disabled user дают одинаковый `401` и не создают authenticated session.

Для unknown user выполняется dummy platform password verification, чтобы failure path не превращался в очевидный fast-path только по факту отсутствия username.

Audit login success/failure в V2-S07B не записывается: actor-aware security audit относится к V2-S09.

## 68. Cookie session и identity claims

Dispatcher не создаёт собственный bearer/session token format.

Используется standard ASP.NET Core cookie authentication scheme:

```text
Scheme = Dispatcher.Local
Cookie = Dispatcher.Auth
```

Cookie policy:

```text
HttpOnly = true
SameSite = Strict
SecurePolicy = SameAsRequest
IsPersistent = false
Ticket lifetime = 8 hours
SlidingExpiration = true
```

`IsPersistent=false` означает browser-session cookie; authentication ticket при этом имеет bounded lifetime и может обновляться sliding expiration механизмом.

Principal содержит только identity claims:

```text
NameIdentifier → UserId
Name           → UserName
dispatcher:display_name → DisplayName
```

Role/permission claims отсутствуют. Это сохраняет границу:

```text
V2-S07 authentication
        ≠
V2-S08 authorization
```

## 69. Current user и logout semantics

`GET /api/auth/current` является public state endpoint.

Anonymous response:

```text
Authenticated = false
UserId        = null
UserName      = null
DisplayName   = null
```

Authenticated response:

```text
Authenticated = true
UserId
UserName
DisplayName
```

Login и current-user responses имеют `Cache-Control: no-store`.

`POST /api/auth/logout` вызывает platform `SignOutAsync` и возвращает `204 No Content`. Logout остаётся idempotent для anonymous client.

На V2-S07B existing runtime/configuration/history/events/mimic endpoints не получают `.RequireAuthorization()`. Middleware уже умеет заполнить `HttpContext.User`, но enforcement по permissions добавляется только в V2-S08.

## 70. V2-S07B scope boundary

После V2-S07B есть:

```text
persistent local users
platform password hashing + verification
explicit bootstrap
login
logout
current user
authenticated cookie session
disabled user login rejection
```

Ещё нет:

```text
Web login/current-user shell
roles
permissions
server authorization policies
audit actor identity
user management API/Web
session revocation on user mutation
```

Следующий шаг:

```text
V2-S07C — Web authentication integration
```

## 71. Web authentication state boundary

V2-S07C добавляет client-side state boundary поверх Server authentication API:

```text
browser-managed Dispatcher.Auth cookie
        ↓ same-origin request
GET /api/auth/current
        ↓
AuthenticationClient.CurrentUser
        ↓
App.razor
   ├── anonymous      → Login
   └── authenticated  → Router + MainLayout
```

`AuthenticationClient` является scoped Web service и хранит только Server-projected `CurrentUserDto`. Web не читает HttpOnly cookie и не создаёт отдельный bearer/localStorage token. После browser refresh identity восстанавливается повторным `GET /api/auth/current`.

Login и logout также используют existing Server endpoints:

```text
POST /api/auth/login
POST /api/auth/logout
```

После каждого successful action `AuthenticationClient` обновляет current identity и уведомляет application shell через `Changed`.

## 72. Anonymous и authenticated shell

Anonymous state не рендерит service content за login form:

```text
┌──────────────────────────────────────────────────────┐
│ Dispatcher   Вход                                   │
├──────────────────────────────────────────────────────┤
│            local username / password                │
│                         Войти                        │
└──────────────────────────────────────────────────────┘
```

В anonymous header отсутствует `☰`, потому что service navigation до login не является полезным workflow. Login screen не является editor, поэтому искусственные left navigation/right properties panels не добавляются.

Authenticated state сохраняет существующий engineering application shell. Справа в compact global header показываются:

```text
DisplayName
UserName
Выйти
```

Current browser route не заменяется специальным `/login` route. Если anonymous user открыл `/events`, `/history` или другой service URL, login form отображается поверх этого routing intent; после successful login Router открывает тот же URL. Logout также не меняет URL, поэтому повторный login возвращает пользователя в прежний service context.

## 73. Web visibility не является authorization

V2-S07C намеренно различает:

```text
Web authentication UX
        ≠
Server authorization
```

Скрытие service drawer/workspace для anonymous user уменьшает UI ambiguity, но не защищает REST/SignalR endpoints. Existing runtime/configuration/history/events/mimic endpoints на V2-S07C сохраняют semantics V2-S07B и не получают permission requirements.

V2-S08 должен сначала добавить permission-based Server authorization, и только после этого Web visibility/enabled state сможет отражать effective permissions. Client-side checks никогда не заменяют Server enforcement.

## 74. V2-S07 result и scope boundary

После V2-S07A/B/C полный authentication foundation содержит:

```text
persistent local users
platform password hash + verification
explicit bootstrap
disabled-user login rejection
ASP.NET Core cookie session
login / logout / current user Server API
Web current-session initialization
anonymous login screen
authenticated current-user header
logout action
```

Configuration schema остаётся version `5`, operational schema — version `2`.

Ещё нет:

```text
roles
permissions
server authorization policies
audit actor identity
user/role management API/Web
session revocation on user mutation
```

Следующий шаг:

```text
V2-S08 — Permissions и Roles
```

## 75. Durable permission/role configuration boundary

V2-S08A повышает configuration SQLite schema:

```text
v5 → v6
```

и добавляет только security configuration tables:

```text
security_roles
├── role_id
├── name
├── normalized_name UNIQUE
└── built_in

security_role_permissions
├── role_id FK → security_roles
└── permission
   PK(role_id, permission)

security_user_roles
├── user_id FK → local_users
└── role_id FK → security_roles
   PK(user_id, role_id)
```

Operational SQLite не меняется и остаётся schema `2`. Role definitions и assignments являются низкочастотной security configuration, а не operational/audit records.

Permission identifiers централизованы в `Dispatcher.Contracts.Authorization.PermissionNames`. Storage принимает только известные permission identifiers; business authorization в следующем подшаге будет ссылаться на permissions, а не role names.

## 76. Built-in roles и initial ownership transition

Server поддерживает четыре built-in role definitions:

```text
Viewer
Operator
Engineer
Administrator
```

Baseline mapping:

```text
Viewer
  Runtime.Read

Operator
  Runtime.Read
  Tags.Write
  Alarms.Acknowledge

Engineer
  Runtime.Read
  Tags.Write
  Devices.Edit
  Mimics.Edit
  Historian.Configure
  Alarms.Configure
  Alarms.Acknowledge
  Templates.Edit
  Scripts.Edit
  Scripts.Execute

Administrator
  all declared permissions
```

Built-in definitions system-managed: startup idempotently восстанавливает их canonical metadata/permission mapping. Future user-created roles могут храниться рядом, но не заменяют built-in identifiers.

Initial ownership transition нужен, чтобы introduction authorization не блокировал существующую installation:

```text
new bootstrap user
    → local user + Administrator assignment in one transaction

first v6 security initialization
+ exactly one existing local user
+ no assignments
    → that user gets Administrator once
```

Если existing users несколько, Server не выбирает администратора по имени/порядку и пишет warning. Если первый startup создал built-in roles без user, а bootstrap user появился позже после задания secret, факт нового bootstrap используется для назначения `Administrator`.

One-time legacy bridge не повторяется на каждом startup: наличие уже инициализированных built-in roles означает, что последующее явное снятие role не должно автоматически возвращаться.

## 77. Effective permission catalog

`SecurityCatalog` является singleton in-memory projection security configuration:

```text
local_users
security_roles + permissions
security_user_roles
        ↓
SecurityCatalog
        ↓
UserId → Enabled + RoleIds + EffectivePermissions
```

Effective permissions вычисляются как set union permissions всех assigned roles.

Disabled user:

```text
Enabled = false
    ↓
HasPermission = false
EffectivePermissions = []
```

Cookie principal V2-S07 остаётся identity-only. Roles/permissions не копируются в long-lived cookie claims, поэтому Server authority для authorization может опираться на current security configuration/catalog, а не на potentially stale permission snapshot в ticket.

## 78. V2-S08A scope boundary

После V2-S08A есть:

```text
durable roles
durable role permissions
durable user-role assignments
built-in role definitions
bootstrap/legacy initial Administrator transition
effective-permission SecurityCatalog
```

Ещё нет:

```text
permission authorization requirement/handler
RequireAuthorization on REST endpoints
SignalR permission enforcement
401/403 permission integration tests
Web permission visibility/enabled state
Users/Roles management API/Web
audit actor records
```

Следующий шаг:

```text
V2-S08B — Server permission enforcement
```

## 79. Permission authorization policy boundary

V2-S08B регистрирует отдельную ASP.NET Core authorization policy для каждого stable identifier из `PermissionNames`.

Каждая policy содержит:

```text
RequireAuthenticatedUser
        +
PermissionRequirement(permission)
        ↓
PermissionAuthorizationHandler
        ↓
ClaimTypes.NameIdentifier → UserId
        ↓
SecurityCatalog.HasPermission(UserId, permission)
```

Handler не проверяет built-in/custom role names и не использует role claims. Cookie остаётся identity-only, поэтому effective permission определяется current `SecurityCatalog` на каждом authorization evaluation.

Это даёт важное runtime-свойство:

```text
cookie already issued
        +
current SecurityCatalog user disabled / permission removed
        ↓
protected request denied
```

Перевыпуск cookie для применения текущей authorization configuration не требуется.

## 80. Server permission matrix и HTTP semantics

V2-S08B централизует mapping существующих REST/SignalR boundaries к permissions:

```text
GET/HEAD /api/tags
GET/HEAD /api/devices
GET/HEAD /api/configuration/modbus/*
GET/HEAD /api/configuration/snmp/*
GET/HEAD /api/configuration/historian/*
GET/HEAD /api/history
GET/HEAD /api/events
GET/HEAD /api/mimics/*
    → Runtime.Read

/hubs/runtime
    → Runtime.Read

POST /api/tags/{tagId}/write
    → Tags.Write

non-read /api/configuration/modbus/*
non-read /api/configuration/snmp/*
    → Devices.Edit

non-read /api/configuration/mimics/*
    → Mimics.Edit

non-read /api/configuration/historian/*
    → Historian.Configure
```

Public boundaries сохраняются намеренно:

```text
/health
/api/auth/login
/api/auth/logout
/api/auth/current
static Web application assets/routes
```

Unknown non-read `/api` path не получает implicit read permission и fail-closed через deny policy до появления явной permission mapping.

Response semantics:

```text
anonymous + protected boundary
    → 401 Unauthorized

authenticated + missing permission
    → 403 Forbidden

authenticated + permission
    → endpoint normal semantics
```

Cookie handler для login/access-denied challenge возвращает status code вместо redirect, чтобы REST/SignalR clients не получали HTML navigation вместо `401/403`.

## 81. V2-S08B scope boundary

После V2-S08B есть:

```text
durable roles/permissions/assignments
current effective-permission SecurityCatalog
permission requirements + handler
REST permission enforcement
RuntimeHub permission enforcement
401 anonymous / 403 insufficient permission
fail-closed unmapped API mutations
```

Ещё нет:

```text
Web effective-permission projection
permission-aware service navigation
permission-aware mutation enabled state
Users/Roles management API/Web
audit actor records
```

Следующий шаг:

```text
V2-S08C — Web permission visibility/enabled state
```

## 82. Web effective-permission projection

V2-S08C расширяет authenticated Server projection, но не authentication cookie.

Successful login и `GET /api/auth/current` возвращают:

```text
Authenticated
UserId
UserName
DisplayName
EffectivePermissions[]
```

`EffectivePermissions[]` вычисляется через current `SecurityCatalog` в момент ответа. Anonymous response содержит empty permission list. Cookie principal остаётся identity-only и по-прежнему не содержит roles/permissions.

Web `AuthenticationClient` хранит эту Server projection и предоставляет только UX helpers:

```text
HasPermission(permission)
HasAllPermissions(...)
```

Если security configuration изменена другим actor/process, Server enforcement V2-S08B применяется немедленно независимо от того, успел ли Web refresh-нуть свою projection. Web state обновляется при login/current-session refresh и в будущих management flows может быть перечитан через existing `RefreshAsync`.

## 83. Permission-aware Web routes и navigation

Current Web service mapping:

```text
Monitoring      → Runtime.Read
Mimics runtime → Runtime.Read
History        → Runtime.Read
Events         → Runtime.Read
Device Editor  → Runtime.Read + Devices.Edit
Mimic Editor   → Runtime.Read + Mimics.Edit
```

Global navigation показывает только доступные current services. Editor-specific entry point скрывается без соответствующего edit permission.

Direct URL не является обходом UX gating:

```text
/devices         without Devices.Edit
/mimics/editor   without Mimics.Edit
        ↓
MainLayout + insufficient-permission state
        ↓
editor component is not rendered
```

`Runtime.Read` также требуется editor routes, потому что editor workflow сначала читает current configuration/runtime state, а уже затем выполняет mutation.

User без `Runtime.Read` остаётся authenticated, видит identity/logout в global header, но не получает current operational services в drawer. Это корректное состояние для future custom/admin-only roles.

## 84. Permission-aware mutation controls

S08C отражает только уже существующие mutation permissions:

```text
Monitoring writable tag control
    → Tags.Write

Mimic runtime Button command
    → Tags.Write

Device Editor route/workspace
    → Devices.Edit + Runtime.Read

Mimic Editor route/workspace
    → Mimics.Edit + Runtime.Read
```

Если configured tag writable, но user не имеет `Tags.Write`, Monitoring не показывает editable input/write button и отображает read-only marker. Mimic Button остаётся visible как часть runtime definition, но получает disabled interaction state.

History и Events на текущем этапе read-only. `Historian.Configure` уже защищает Server mutation API, но отдельного historian-policy mutation control в current Web нет, поэтому S08C не создаёт искусственный UI только ради permission demonstration.

Client checks являются UX only:

```text
Web control hidden/disabled
        ≠
permission granted/denied
```

Окончательный результат всегда определяет Server policy V2-S08B.

## 85. V2-S08 result и scope boundary

После V2-S08A/B/C есть полный permission vertical slice:

```text
durable roles / role permissions / user-role assignments
        ↓
current effective-permission SecurityCatalog
        ↓
Server REST + SignalR enforcement
        ↓
Server-projected EffectivePermissions[]
        ↓
Web route/navigation/action visibility
```

По-прежнему не реализованы:

```text
Users/Roles management API/Web
audit actor records
login audit
security configuration mutation UI
```

Следующий шаг:

```text
V2-S09 — Users/Roles Web + Audit
```

## 86. Users/Roles management boundary

V2-S09A добавляет management API без изменения configuration schema `6`:

```text
/api/security/users
/api/security/roles
```

Цепочка mutation:

```text
authenticated actor
      ↓ permission policy
SecurityManagementEndpoints
      ↓
SecurityManagementService
      ↓ serialized mutation gate
SqliteConfigurationStore
      ↓ durable v6 users/roles/assignments
reload full security state
      ↓
SecurityCatalog.ReplaceAll
```

`SecurityManagementService` сериализует низкочастотные security mutations одним process-local `SemaphoreSlim`. Это не заменяет SQLite transaction внутри multi-row assignment replacement, а предотвращает race между proposed-state validation и commit/catalog refresh в одном Server process.

## 87. User management semantics

Public management projection пользователя не содержит `PasswordHash`:

```text
UserId
UserName
DisplayName
Enabled
RoleIds[]
EffectivePermissions[]
```

После create `UserName` immutable. S09A изменяет только `DisplayName`, `Enabled`, password hash и role assignments.

Password reset:

```text
plaintext request
  ↓ validate 12..256
PasswordHasher<LocalUserConfiguration>
  ↓
local_users.password_hash
```

Plaintext password и encoded hash не возвращаются API. Existing cookie format не меняется. `GET /api/auth/current` использует cookie `UserId` только как identity key, а актуальные `UserName/DisplayName` перечитывает из `local_users`; поэтому mutable display metadata не застывает до следующего login. Disable/role removal немедленно влияет на protected authorization через refreshed `SecurityCatalog`; отдельный durable per-session revocation token в S09A не вводится.

## 88. Role management и permission boundaries

Built-in roles остаются canonical system-managed definitions и отклоняют update/delete через management API. Custom role получает stable generated `RoleId`, unique normalized name и набор только известных `PermissionNames`. Assigned custom role нельзя удалить до явного снятия assignments.

Management authorization:

```text
users list/create/profile/Enabled → Users.Manage
user role assignments            → Roles.Manage
password reset                   → Users.Manage + Roles.Manage
roles list/create/update/delete  → Roles.Manage
```

Password reset требует оба permissions, потому что credential takeover пользователя с более широкими roles не должен быть побочным способом обойти разделение user/role administration.

## 89. Administrative survivability invariant

Перед mutation, которая может уменьшить authority, Server строит proposed effective-permission state и требует:

```text
exists user where
    Enabled
    && HasPermission(Users.Manage)
    && HasPermission(Roles.Manage)
```

Проверка применяется к user disable, replacement role assignments и custom-role update. Она не проверяет имя `Administrator` и допускает custom role/composed roles, если effective capabilities сохраняются.

После successful mutation durable state перечитывается целиком и заменяет singleton `SecurityCatalog`, поэтому уже выданные identity-only cookies получают новую authorization authority на следующем protected request.

S09A не добавляет Web admin screen и actor-aware audit events. Следующий подшаг — V2-S09B.

## 90. Web security management boundary

V2-S09B добавляет Web service поверх уже authoritative S09A API:

```text
CurrentUser.EffectivePermissions[]
        ↓
AuthenticationClient
        ↓
/security
        ↓
SecurityManagementClient
        ↓ same-origin cookie requests
/api/security/users + /api/security/roles
```

`SecurityManagementClient` не хранит отдельный security catalog и не вычисляет privileges из role names. Он сериализует/читает public S09A contracts и сохраняет Server `ProblemDetails.detail` для наблюдаемого UI error.

Route `/security` является отдельным administrative service и не требует `Runtime.Read`. Web допускает его когда:

```text
Users.Manage OR Roles.Manage
```

Server policies S09A остаются окончательной authority для каждого конкретного endpoint.

## 91. Users / Roles spatial model и capability split

Admin screen следует общему engineering layout:

```text
┌──────────────┬──────────────────────────────────────┬─────────────────┐
│ Users/Roles  │ toolbar + dense users/roles table   │ selected object │
│ local nav    │                                      │ properties      │
└──────────────┴──────────────────────────────────────┴─────────────────┘
```

Capability projection:

```text
Users.Manage
    → users section/list/create/profile/Enabled

Roles.Manage
    → roles section/list/custom-role CRUD

Users.Manage + Roles.Manage
    → selected-user role assignments
    → password reset
```

Web не делает forbidden API call только ради отображения недоступного раздела: user-only actor не запрашивает roles list, role-only actor не запрашивает users list. Если доступны обе capability, один screen позволяет связать user и roles.

Built-in role определяется public `SecurityRoleDto.BuiltIn`: его properties/permissions visible, mutation controls отсутствуют. Этот flag управляет editability конкретной configuration entity и не используется для granting access.

## 92. Current actor refresh после security mutation

После successful user/role mutation Web выполняет:

```text
mutation API response
      ↓
AuthenticationClient.RefreshAsync()
      ↓ GET /api/auth/current
current DisplayName + EffectivePermissions[]
      ↓
MainLayout / App route projection
      ↓
reload доступных security lists
```

Это важно для self-management случаев: изменение собственного display name, disable или role assignment сразу отражается в текущем header/navigation/route visibility. Cookie не расширяется и не становится permission snapshot.

S09B не добавляет actor-aware Event Journal records. Следующий подшаг:

```text
V2-S09C — Actor-aware security audit wiring
```


## 93. Actor-aware Event Journal record

V2-S09C расширяет существующий immutable `EventRecord` nullable actor identity:

```text
ActorUserId?
ActorUserName?
```

Operational SQLite schema повышается:

```text
v2 → v3
```

Migration добавляет nullable columns:

```text
actor_user_id
actor_user_name
```

Existing `history_samples` и `events` не перестраиваются и не удаляются; старые records после migration имеют `NULL` actor. System/device events также могут быть actor-less, потому что у них нет authenticated user action.

Actor является отдельной частью record, а не только `DataJson`: это сохраняет machine-readable identity независимо от producer-specific payload. Cookie/session format не меняется.

## 94. Audit actor semantics

Для already-authorized HTTP mutations actor извлекается до business mutation из authenticated principal:

```text
ClaimTypes.NameIdentifier → ActorUserId
ClaimTypes.Name           → ActorUserName
```

Если `Name` отсутствует, stable `UserId` используется как fallback name; отсутствие authenticated `UserId` считается invalid audit boundary до выполнения mutation. Permission granting по-прежнему выполняет S08/S09 authorization и не зависит от actor event fields.

Authentication имеет отдельную семантику:

```text
login success
    → verified LocalUser → ActorUserId + ActorUserName

login failure
    → actor = null
    → bounded attempted username только в DataJson
```

Неподтверждённый login identifier не записывается как actor. Password/plaintext hash никогда не входит в audit payload.

## 95. S09C audit producers

Минимальный actor-aware набор:

```text
Authentication
  LoginSucceeded
  LoginFailed

Security configuration
  SecurityUserCreated
  SecurityUserUpdated
  SecurityUserPasswordReset
  SecurityUserRolesChanged
  SecurityRoleCreated
  SecurityRoleUpdated
  SecurityRoleDeleted

Commands
  TagWriteSucceeded
  TagWriteFailed

Configuration
  ConfigurationChanged
    Modbus / SNMP
    Mimic
    Historian policy
```

`RuntimeConfigurationApplied` остаётся отдельным operational event без обязательного actor. Это различает:

```text
user changed configuration
        ≠
runtime applied current configuration
```

Security/configuration audit event создаётся только после successful durable/business mutation. Authorization-denied request не доходит до producer и отдельным S09C event не журналируется.

## 96. Audit persistence boundary

Actor-aware audit не создаёт mutable audit subsystem. Он повторно использует существующую цепочку:

```text
producer
  ↓ EventJournalService.Publish
bounded channel
  ↓
operational SQLite v3 events
  ↓
Events REST / EventAdded SignalR
```

Journal остаётся append-only и не получает update/delete API. Actor fields входят в REST/realtime `EventRecordDto`, поэтому machine consumers могут читать audit identity без разбора `DataJson`.

S09C намеренно сохраняет существующий bounded asynchronous ingestion contract (`DroppedEventCount` / retry persistence). Это базовая operational traceability Dispatcher, а не отдельный compliance-grade synchronous audit ledger.

После V2-S09C Phase 7 завершена. Следующий шаг — V2-S10 Alarm definitions / Alarm Editor.

## 97. Alarm definition configuration boundary

V2-S10A начинает Phase 8 только с durable definitions. Alarm runtime state machine не запускается.

Цепочка configuration:

```text
AlarmDefinitionDto / requests
        ↓
AlarmDefinitionEndpoints
        ↓
AlarmDefinitionService
        ↓
SqliteConfigurationStore
        ↓
dispatcher.db / alarm_definitions
```

Configuration SQLite повышается:

```text
v6 → v7
```

Таблица хранит:

```text
alarm_id
name
enabled
tag_id
condition
threshold_text?
severity
message
delay_ms
hysteresis_text?
```

`AlarmId` является stable immutable identifier: create принимает его в body, update/delete адресуют его path и не переименовывают definition.

## 98. Alarm condition и numeric semantics

Первый condition set без expression language:

```text
DigitalTrue
DigitalFalse
High
Low
```

Semantics definition validation:

```text
DigitalTrue / DigitalFalse
    Threshold  = null
    Hysteresis = null

High / Low
    Threshold  = required decimal
    Hysteresis = required decimal >= 0

DelayMilliseconds >= 0
```

Threshold/Hysteresis сохраняются в SQLite как invariant decimal text. Это избегает необязательной binary floating conversion configuration values до появления runtime evaluator.

Alarm severity имеет отдельный typed alarm contract, но первый value set намеренно выровнен с текущей Event Journal taxonomy:

```text
Information
Warning
Error
```

Так Alarm domain не зависит от Event DTO type, при этом S10A не изобретает вторую severity scale.

## 99. Alarm TagId binding, CRUD authorization и audit

Definition binding остаётся logical:

```text
AlarmDefinition → TagId
```

Create/update требуют, чтобы `ConfigurationCatalog.ContainsTagId(TagId)` был true в момент mutation. `alarm_definitions` не имеет FK на Modbus/SNMP tag tables: logical TagId живёт поверх нескольких protocol stores, а последующее удаление tag не должно неявно стирать инженерную alarm configuration. Stale binding остаётся читаемым и будет явно обработан будущим Alarm runtime/editor.

Server API:

```text
GET    /api/configuration/alarms/definitions  → Runtime.Read
POST   /api/configuration/alarms/definitions  → Alarms.Configure
PUT    /api/configuration/alarms/definitions/{alarmId} → Alarms.Configure
DELETE /api/configuration/alarms/definitions/{alarmId} → Alarms.Configure
```

Successful mutation после durable write публикует existing actor-aware:

```text
Category = Configuration
Type     = ConfigurationChanged
Area     = Alarms
ActorUserId / ActorUserName
```

Web visibility в S10A не добавляется; Server permission boundary является authoritative.

## 100. V2-S10A scope boundary

После S10A есть:

```text
durable alarm definitions
configuration schema v7
validated Digital/High/Low definition contract
permission-protected Server CRUD
actor-aware configuration audit
```

Ещё нет:

```text
Alarm Editor Web
alarm evaluation
delay/hysteresis runtime behavior
alarm instance state
raise / return transitions
acknowledgement
realtime alarms
```

Следующий подшаг:

```text
V2-S10B — Alarm Editor Web
```

## 101. Alarm Editor Web boundary

V2-S10B добавляет engineering Web service поверх уже authoritative S10A configuration API:

```text
CurrentUser.EffectivePermissions[]
        ↓
App / MainLayout
        ↓ Runtime.Read + Alarms.Configure
/alarms
        ↓
AlarmClient
        ↓
/api/configuration/alarms/definitions
        ↓
S10A AlarmDefinitionService / SQLite v7
```

Web не получает отдельный alarm configuration cache. После create/update/delete editor перечитывает durable definitions через existing REST boundary. `AlarmClient` только сериализует public contracts и сохраняет `ProblemDetails.detail` как наблюдаемую operation error.

`Runtime.Read` требуется вместе с `Alarms.Configure`, потому что editor одновременно читает alarm definitions и current Modbus/SNMP tag configuration для logical `TagId` picker. Client gating является UX projection; Server middleware по-прежнему отдельно авторизует каждый request.

## 102. Alarm Editor spatial/draft model и S10 boundary

Spatial model повторяет общий engineering editor contract:

```text
┌──────────────┬──────────────────────────────────────┬─────────────────┐
│ Alarms       │ add / save / delete / refresh       │ Properties      │
│ local list   ├──────────────────────────────────────┤                 │
│              │ dense alarm rules table              │ selected rule   │
└──────────────┴──────────────────────────────────────┴─────────────────┘
```

Редактирование выполняется через client-side draft:

```text
GET definitions
      ↓
select definition
      ↓
AlarmDraft
      ↓ local edits
explicit Save
      ↓
POST create / PUT update
      ↓
reload definitions
```

Persisted `AlarmId` не редактируется. Picker строится из current Modbus/SNMP `TagId`; если definition стал stale после удаления source tag, его старый `TagId` остаётся видимым с explicit warning, но Save требует выбрать current configured tag, что соответствует S10A create/update validation.

Condition-aware properties отображают только definition semantics: `DigitalTrue/DigitalFalse` не показывают Threshold/Hysteresis, `High/Low` показывают оба decimal поля. Web не интерпретирует delay/hysteresis и не вычисляет active state.

После V2-S10A/B завершён alarm configuration vertical slice:

```text
durable definition
      ↓
Server CRUD + permissions + audit
      ↓
Web Alarm Editor
```

Alarm evaluation, transitions и operational state начинаются только в V2-S11.
