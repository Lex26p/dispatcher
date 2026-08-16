# Архитектура Dispatcher

## 1. Состояние после V2-S07B

Application layer содержит несколько пользовательских operational services и начало security configuration boundary:

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

Device Editor → device configuration
Mimic Editor  → mimic definition

Configuration SQLite
      ↓
local user identity/password hash
      ↓
LocalAuthenticationService
      ↓
ASP.NET Core cookie → HttpContext.User
```

Mimic runtime и Mimic Editor не знают protocol-specific address.

V2-S07A сохраняет local user identities/password hashes, а V2-S07B добавляет login/logout/current-user и ASP.NET Core cookie session boundary. Roles/permissions и Web login ещё не введены.

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
5
```

Tables, добавленные после базовой protocol configuration:

```text
mimics
historian_policies
local_users
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
```

При `v4 → v5` protocol/mimic/historian tables не перестраиваются и не очищаются.

Operational SQLite schema независимо остаётся version `2`.

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

Начиная с V2-S05 current operational schema:

```text
PRAGMA user_version = 2
```

Migration:

```text
v1 history_samples
 ↓ preserve history
v2 history_samples + events
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

Configuration initialization начиная с V2-S07A:

1. создаёт/мигрирует configuration schema;
2. при пустом `local_users` пытается выполнить explicit bootstrap пользователя;
3. загружает protocol configuration и historian policies;
4. публикует active configuration catalogs.

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
