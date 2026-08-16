# Архитектурные решения

Если решение меняется, старое не удаляется молча: его статус меняется на `Superseded`, а ниже добавляется новое решение с причиной.

---

## D-001 — C#/.NET для Core и Server

**Status:** Accepted

Core и Server реализуются на C#/.NET.

---

## D-002 — Blazor WebAssembly для Web

**Status:** Accepted

Web-клиент реализуется на Blazor WebAssembly.

---

## D-003 — TagService как центр runtime-состояния

**Status:** Accepted

`TagService` хранит текущие значения логических Tag. Connection state хранится отдельно в `DeviceStateService`.

---

## D-004 — Протокольные детали изолированы

**Status:** Accepted

Monitoring, мнемосхемы и общая runtime-логика работают с `TagId`/`DeviceId`, а не protocol address.

---

## D-005 — Начинать просто

**Status:** Accepted

Не вводим преждевременно alarms, historian, roles, brokers, distributed services и generic plugin framework.

---

## D-006 — Логическая модульность раньше физического разделения

**Status:** Accepted

Компоненты имеют явные границы, но ранняя версия выполняется в одном host.

---

## D-007 — REST + SignalR

**Status:** Accepted

REST используется для snapshot/commands, SignalR — для realtime changes.

---

## D-008 — Configuration и Runtime разделены

**Status:** Accepted

Persistent configuration не смешивается с runtime current values.

---

## D-009 — Репозиторий является источником истины

**Status:** Accepted

Перед каждым шагом читается актуальный `master`.

---

## D-010 — Web как плотный инженерный интерфейс

**Status:** Accepted

UI проектируется с приоритетом рабочей области и информационной плотности.

---

## D-011 — NModbus для первой реализации Modbus

**Status:** Accepted

Modbus TCP использует NModbus 3.x (`3.0.83`).

---

## D-012 — Device connection state отделён от TagService

**Status:** Accepted

Connection state хранится в `DeviceStateService`.

---

## D-013 — Reconnect через новое соединение каждого poll-cycle

**Status:** Accepted

Каждый cycle открывает новое TCP-соединение.

---

## D-014 — Public API contracts отделены от Core

**Status:** Accepted

`Dispatcher.Contracts` не зависит от Core/Modbus/Server/Web.

---

## D-015 — Blazor WebAssembly раздаётся тем же ASP.NET Core host

**Status:** Accepted

WASM, REST и SignalR работают с одного origin.

---

## D-016 — Core change-events являются минимальной realtime-границей

**Status:** Accepted

Core `Changed` events преобразуются Server в SignalR.

---

## D-017 — До persistent configuration Modbus host использует стандартную ASP.NET Core configuration

**Status:** Superseded by D-021

S07A/S07B использовали `appsettings` как временный источник device/tag configuration.

---

## D-018 — Write routing выполняется по логическому TagId

**Status:** Accepted

Server разрешает `TagId` в текущей configuration и только затем получает Modbus target.

---

## D-019 — Writable является configuration metadata, а не частью TagService

**Status:** Accepted

`TagService` хранит `TagId/Value/Timestamp`; `Writable` принадлежит configuration.

---

## D-020 — Phase 1 write ограничен UInt16 Holding Register FC06

**Status:** Accepted

Write поддерживает `UInt16` `0..65535` через FC06.

---

## D-021 — Persistent configuration хранится в SQLite

**Status:** Accepted

Начиная с S08 device/tag configuration хранится в SQLite через `Microsoft.Data.Sqlite`.

---

## D-022 — Активная configuration загружается в ConfigurationCatalog

**Status:** Accepted

SQLite — durable source of truth, `ConfigurationCatalog` — активный in-memory snapshot для protocol runtime и write routing.

---

## D-023 — Новая configuration database начинается пустой

**Status:** Accepted

Не создаются скрытые sample devices/tags.

---

## D-024 — Data type не становится фиктивно настраиваемым до реализации второго типа

**Status:** Accepted

Persistent Modbus tag model соответствует реально работающему `Holding Register UInt16`.

---

## D-025 — Configuration mutations сохраняют и применяют целый snapshot

**Status:** Accepted

S09A CRUD не вводит отдельные SQL repositories для каждой сущности.

Каждая mutation:

```text
copy current snapshot
      ↓
change one device/tag
      ↓
validate whole snapshot
      ↓
SQLite ReplaceAsync transaction
      ↓
ConfigurationCatalog.Replace
      ↓
runtime ApplyAsync
```

Причина:

- текущая configuration мала;
- `ReplaceAsync` уже существует и транзакционен;
- целый snapshot упрощает validation global uniqueness `DeviceId`/`TagId`;
- не требуется преждевременная repository/unit-of-work hierarchy.

Если объём configuration станет большим, storage mutation strategy пересматривается.

---

## D-026 — Live apply перезапускает polling loops и сбрасывает runtime current state

**Status:** Superseded by D-031

В S09A один Modbus runtime мог самостоятельно очистить global runtime state. После появления SNMP очистка координируется между протоколами.

---

## D-027 — Configuration API может быть protocol-specific

**Status:** Accepted

Runtime application API остаётся protocol-neutral, но Device Editor должен редактировать реальные настройки протокола.

Поэтому configuration API может иметь protocol-specific contracts/endpoints, тогда как Monitoring и Mimics продолжают работать с logical tags.

---

## D-028 — Device Editor использует explicit Save поверх client-side draft

**Status:** Accepted

Редактирование свойств не вызывает server mutation автоматически.

```text
configuration snapshot
       ↓
client-side draft
       ↓
explicit Save
       ↓
REST mutation
       ↓
live apply
```

Причина:

- каждая configuration mutation приводит к runtime reconfiguration;
- auto-save на каждом вводимом символе создавал бы лишние stop/start polling cycles;
- инженер должен явно видеть момент применения configuration;
- Server остаётся authority по validation.

Dirty draft явно обозначается, а смена выбранного объекта или refresh требует подтверждения потери несохранённых изменений.

---

## D-029 — Первый SNMP scope — v2c GET

**Status:** Accepted

S10A использует `Lextm.SharpSnmpLib 12.5.7`.

Поддерживается:

```text
SNMP v2c
GET
UDP
Community
OID polling
```

Не добавляются пока:

```text
SNMP SET
SNMP v3
TRAP/INFORM receiver
WALK discovery
MIB browser
```

Причина: первый use case — polling конкретных OID в общие logical tags.

---

## D-030 — DeviceId и TagId глобально уникальны между протоколами

**Status:** Accepted

Modbus и SNMP не могут использовать одинаковые logical IDs.

```text
Modbus DeviceId ─┐
SNMP DeviceId   ─┴─ unique

Modbus TagId ────┐
SNMP TagId ──────┴─ unique
```

Причина: `TagService` и `DeviceStateService` являются общими runtime stores и индексируются этими ID.

---

## D-031 — Global runtime state очищает только RuntimeConfigurationCoordinator

**Status:** Accepted

Individual protocol hosted services:

```text
ModbusRuntimeHostedService
SnmpRuntimeHostedService
```

управляют только собственными polling loops.

При configuration live apply общий coordinator выполняет:

```text
stop all protocol polling
        ↓
clear TagService / DeviceStateService
        ↓
start all protocol polling
```

Это предотвращает ситуацию, когда изменение одного протокола уничтожает current state другого и не запускает его заново.

---

## D-032 — SQLite schema v2 добавляет SNMP с миграцией v1 → v2

**Status:** Accepted

Schema version `2` добавляет:

```text
snmp_devices
snmp_tags
```

При upgrade с version `1` Modbus tables и records сохраняются.

Не допускается требование удалить существующую user database ради добавления второго протокола.

---

## D-033 — SNMP values нормализуются до обычных CLR values до TagService

**Status:** Accepted

`TagService` не хранит SharpSnmpLib-specific `ISnmpData`.

Перед публикацией выполняется conversion:

```text
Integer32 / counters / gauge / timeticks / string
        ↓
CLR primitive / string
        ↓
TagService
```

Таким образом Server/Web/mimics не зависят от SNMP library types.

---

## D-034 — Modbus и SNMP configuration mutations сериализуются одним lock

**Status:** Accepted

После появления второго protocol editor Modbus и SNMP mutations выполняет один singleton `ConfigurationEditorService` с одним `SemaphoreSlim`.

Причина:

- оба протокола разделяют `ConfigurationCatalog`;
- `DeviceId` и `TagId` имеют cross-protocol uniqueness;
- после каждой mutation перезапускается общий protocol runtime;
- независимые locks позволили бы двум параллельным mutation работать от разных snapshots и применять runtime в недетерминированном порядке.

Storage остаётся protocol-specific:

```text
ReplaceAsync(modbus)
ReplaceSnmpAsync(snmp)
```

но sequencing configuration changes является общим.

---

## D-035 — Protocol существующего устройства не конвертируется field update-ом

**Status:** Accepted

В Device Editor protocol выбирается при создании:

```text
Modbus TCP
SNMP v2c
```

Для persisted устройства protocol selector read-only.

Причина: Modbus и SNMP имеют разные protocol-specific свойства и tag schemas. Изменение одного enum-поля не определяет, как преобразовать:

```text
UnitId / Address / Writable
        ↕
Community / OID
```

Текущий явный workflow смены протокола:

```text
delete device
create device with another protocol
```

Если в будущем потребуется migration/conversion wizard, это будет отдельная операция, а не обычный update.

---

## D-036 — Runtime мнемосхемы хранится как persistent definition в SQLite

**Status:** Accepted

S11 добавляет `mimics` в общую configuration database и повышает schema version до `3`.

Definition содержит canvas metadata и список элементов.

Причина:

- S12 editor должен сохранять тот же definition, который исполняет runtime;
- отдельный файл/временная hard-coded схема создали бы второй источник истины;
- SQLite уже является durable configuration storage приложения.

Новая БД остаётся пустой: sample-мнемосхема автоматически не создаётся.

---

## D-037 — Первый mimic renderer использует SVG в Blazor WebAssembly

**Status:** Accepted

Runtime elements рендерятся через SVG `viewBox`.

Причина:

- S11 нужны absolute coordinates;
- Text/Rectangle/Indicator естественно выражаются SVG primitives;
- canvas масштабируется без собственной JavaScript rendering loop;
- Blazor event handling достаточно для простого Button;
- coordinate model можно повторно использовать в S12 editor.

JavaScript canvas/WebGL не вводится до появления реальной необходимости.

---

## D-038 — Mimic binding хранит только TagId и использует существующий RuntimeStateClient

**Status:** Accepted

`Value`, `Indicator`, `Button` связываются с runtime только через logical `TagId`.

Mimic definition не содержит:

```text
Modbus address
UnitId
SNMP OID
Community
protocol type
```

Realtime отдельного hub/service не создаёт.

Mimic page использует существующий `RuntimeStateClient`, который уже объединяет REST snapshot и SignalR `TagChanged`.

---

## D-039 — Mimic Button использует существующий tag write path

**Status:** Accepted

Первый Button хранит:

```text
TagId
CommandValue UInt16
Text
```

Кнопка доступна только при `TagValueDto.Writable == true`.

Command выполняется через:

```text
RuntimeStateClient.WriteTagAsync
        ↓
POST /api/tags/{tagId}/write
        ↓
existing write routing
```

Отдельный command bus или protocol-specific command в mimic definition не создаётся.

SNMP-bound Button read-only/disabled в текущем scope.

---

## D-040 — Mimic Editor использует client-side draft и explicit Save

**Status:** Accepted

S12 повторяет уже проверенный Device Editor interaction model:

```text
server MimicDefinitionDto
        ↓
client-side draft
        ↓
local property changes
        ↓
explicit Save
        ↓
PUT whole definition
```

Причина:

- координаты и свойства меняются часто во время редактирования;
- auto-save на каждый input создавал бы лишние HTTP writes;
- пользователь должен явно видеть границу сохранённой и несохранённой схемы;
- S11 persistence уже атомарно сохраняет whole definition.

При смене схемы или refresh dirty draft требует подтверждения потери изменений.

---

## D-041 — Минимальный S12 редактирует position/size через properties panel без drag-and-drop

**Status:** Accepted

Выбор элемента выполняется кликом на SVG canvas.

Position и size редактируются численно:

```text
X
Y
Width
Height
```

в правой properties panel.

Причина:

- это полностью покрывает текущий S12 scope;
- сохраняется одна coordinate model с S11 SVG runtime;
- не требуется JavaScript pointer/drag layer;
- drag handles, snapping, zoom/pan и multi-select можно добавлять только при подтверждённой необходимости.

Отсутствие drag-and-drop не меняет persistent/runtime contracts.

---

## D-042 — Operational data хранится отдельно от configuration database

**Status:** Accepted

Historian и будущие high-frequency operational records используют отдельную SQLite database:

```text
dispatcher.db
    configuration

dispatcher-operational.db
    operational records
```

Причина:

- history/event volume и write rate отличаются от configuration;
- рост operational data не должен менять lifecycle configuration database;
- будущая замена historian/event storage может выполняться независимо от device/mimic configuration.

Operational database имеет собственный `PRAGMA user_version`, начиная с version `1`.

---

## D-043 — Historian ingestion использует bounded Channel и не блокирует TagService callback

**Status:** Accepted

`HistorianService` подписывается на synchronous `TagService.Changed`.

Callback выполняет только normalization и:

```text
Channel.Writer.TryWrite(sample)
```

SQLite I/O выполняет отдельный background writer.

Channel bounded, потому что бесконечная память при остановившемся disk writer недопустима.

Baseline overflow behavior:

```text
buffer full
    ↓
drop incoming sample
    ↓
DroppedSampleCount++
    ↓
warning log
```

Polling/runtime callback не ждёт освобождения channel capacity.

Текущий batch при persistence error retry-ится background writer-ом и не выбрасывается молча.

---

## D-044 — History sample хранит protocol-neutral typed canonical value

**Status:** Accepted

Historian record содержит:

```text
TagId
Timestamp UTC
HistoryValueType
ValueText
```

Типы:

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

Причина:

- Historian не должен зависеть от Modbus/SNMP library types;
- `UInt64` не всегда помещается в SQLite signed INTEGER;
- `decimal` не должен принудительно терять точность через `REAL`;
- canonical text сохраняет точное исходное значение;
- будущий query API может восстановить public typed value без изменения ingest boundary.

Protocol address/OID в history record отсутствует.

---

## D-045 — Historian policies хранятся в configuration DB schema v4 и не имеют FK на protocol tags

**Status:** Accepted

V2-S02 добавляет:

```text
historian_policies
```

в `dispatcher.db` и повышает configuration schema до version `4`.

Policy идентифицируется logical `TagId`.

SQL foreign key на `modbus_tags` или `snmp_tags` не создаётся.

Причина:

- `TagId` protocol-neutral;
- policy должна переживать whole-snapshot replacement protocol configuration;
- удалённый/переименованный tag должен оставлять явный stale policy, а не silently cascade-delete archival intent.

API сообщает stale state через `TagExists=false`.

---

## D-046 — Historian sampling policy поддерживает OnChange и Periodic с live apply

**Status:** Accepted

Отсутствие policy означает:

```text
не архивировать
```

Enabled policy имеет mode:

```text
OnChange
Periodic
```

`OnChange` сохраняет timestamp runtime change.

`Periodic` требует interval `100 ms .. 24 h`, читает current value из `TagService` и ставит timestamp фактического sample time.

Policy mutation обновляет `HistorianPolicyCatalog` без restart Modbus/SNMP polling.

Stale policy и disabled policy sampling не выполняют.

Deadband в V2-S02 не вводится, потому что он добавляет numeric-specific semantics, не требуемую для закрытия двух базовых sampling modes.

---

## D-047 — Retention применяется к сохранённой policy независимо от Enabled/TagExists

**Status:** Accepted

`RetentionDays` принадлежит historian policy.

Cleanup удаляет operational samples per `TagId` старше cutoff.

Retention продолжает работать если:

```text
Enabled = false
TagExists = false
```

Причина: остановка нового sampling или временно сломанный binding не должны превращать накопленную историю в бесконтрольно растущие данные.

Удаление самой policy не удаляет history немедленно и прекращает automatic cleanup для этого `TagId`.

Такое удаление данных должно оставаться отдельным явным действием, если оно понадобится позже.

---

## D-048 — History query возвращает series per TagId и ограничивает points per series

**Status:** Accepted

V2-S03 вводит:

```text
GET /api/history
```

с repeated `tagId`, обязательными `from/to`, `order` и `limit`.

`limit` трактуется как maximum points на одну series.

Baseline limits:

```text
MaxTagCount = 16
DefaultLimit = 1000
MaxLimit = 2000
```

Ответ группируется:

```text
Series[]
├── TagId
├── Truncated
└── Samples[]
```

а не возвращает один смешанный поток нескольких tags.

Причина:

- trend UI работает естественно с отдельными series;
- semantics limit остаётся одинаковой независимо от частоты разных tags;
- пустой TagId range можно представить empty series;
- maximum response заранее ограничен.

---

## D-049 — History API публикует ValueType + canonical ValueText вместо общего object

**Status:** Accepted

Public history sample:

```text
Timestamp
ValueType
ValueText
```

не преобразует canonical history value в общий `object`.

Причина:

- сохранить полный `UInt64`;
- сохранить `Decimal` precision;
- не зависеть от numeric behaviour JavaScript/JSON clients;
- оставить `Json` payload lossless;
- public contract явно сообщает семантический type.

V2-S04 самостоятельно преобразует numeric `ValueText` в display/chart representation.

---

## D-050 — History query допускает deleted/stale TagId

**Status:** Accepted

Read API не требует current device tag или active historian policy.

Сохранённая history должна оставаться доступной после удаления/rename configuration.

Разделение:

```text
current configuration / policy
    → определяет future sampling

operational history
    → определяется фактически сохранённым TagId
```

Это сохраняет диагностическую ценность history и соответствует stale-policy semantics V2-S02.

---

## D-051 — Первый History/Trends renderer использует SVG без внешней chart library

**Status:** Accepted

V2-S04 строит trend средствами Blazor + SVG.

Причина:

- V2-S03 уже ограничивает число samples;
- первый Web scope требует только line trend;
- SVG сохраняет C#-first implementation;
- не добавляется тяжёлая dependency до подтверждённой необходимости.

Для trend `ValueText` numeric/boolean values преобразуются в `double` только как display representation.

Lossless `ValueType + ValueText` остаётся неизменным в API и таблице.

---

## D-052 — History Web ограничивает browser rendering сильнее Server API

**Status:** Accepted

Server query boundary допускает:

```text
16 series
2000 samples per series
```

Первый Web screen использует:

```text
8 selected series
1000 rendered SVG points per series
2000 total table rows
```

Причина: browser WASM/DOM rendering должен оставаться bounded без premature virtualization/downsampling framework.

Ограничение является только Web display policy и не меняет REST API.

---

## D-053 — History Web позволяет ручной TagId для retained/stale history

**Status:** Accepted

Основной tag selector строится из current Modbus/SNMP configuration.

Дополнительно пользователь может вручную указать logical `TagId`.

Причина: V2-S03 намеренно разрешает читать retained history deleted/stale tags, которых уже нет в current configuration.

Manual TagId не создаёт device tag или historian policy и используется только как history query selection.

---

## D-054 — Event Journal хранится в operational SQLite schema v2 как immutable append-only records

**Status:** Accepted

V2-S05 повышает:

```text
dispatcher-operational.db
v1 → v2
```

и добавляет:

```text
events
```

Existing `history_samples` сохраняется без перестройки.

Event Journal не хранится в configuration database.

Для journal существуют append/read foundation operations; update/delete event API не вводится.

Причина:

- event — факт, произошедший во времени, а не редактируемая configuration;
- historian/events имеют общий operational lifecycle;
- будущие alarms/audit смогут ссылаться на ту же временную модель;
- configuration database не должна расти от operational records.

---

## D-055 — Event ingestion bounded/asynchronous и device status дедуплицируется в EventJournalService

**Status:** Accepted

Producer вызывает:

```text
EventJournalService.Publish
        ↓
Channel.Writer.TryWrite
```

SQLite write выполняет отдельный background writer.

Baseline:

```text
BufferCapacity = 4096
BatchSize = 128
```

При full buffer incoming event отбрасывается и учитывается в `DroppedEventCount`; producer не блокируется.

`DeviceStateService.Changed` не меняется. Поскольку он публикует state после каждого poll, Event Journal самостоятельно фиксирует только first status/status transition.

Причина: journal должен отражать operational transitions, а не частоту polling.

---

## D-056 — Event type отделён от Message, а producer-specific данные хранятся как nullable DataJson

**Status:** Accepted

Event record разделяет:

```text
Category
Type
Severity
Source
Message
DataJson
```

`Type` — stable machine-readable identifier.

`Message` — human-readable описание.

`DataJson` — optional structured details конкретного producer.

Первый набор categories:

```text
System
Device
Command
Configuration
```

Event Journal не является AlarmService. Alarm states/transitions появятся отдельной subsystem после Event Journal и security foundation.

---

## D-057 — Events query использует newest-first page/limit и HasMore без COUNT

**Status:** Accepted

`GET /api/events` имеет required `from/to` и optional:

```text
category
severity
source
text
```

Первый paging contract:

```text
page
limit
HasMore
```

Default/max:

```text
DefaultLimit = 200
MaxLimit = 500
```

Ordering:

```text
Timestamp DESC
EventId DESC
```

`HasMore` определяется чтением `limit + 1`.

Причина:

- плотный journal UI не требует total count;
- `COUNT(*)` не нужен для каждой фильтрации;
- простой page contract закрывает первый Events Web use case.

---

## D-058 — EventAdded отправляется через существующий RuntimeHub только после persistence

**Status:** Accepted

Event producer не отправляет SignalR напрямую.

Realtime chain:

```text
Publish
 ↓ bounded queue
SQLite persistence
 ↓
EventJournalService.Persisted
 ↓
EventHubPublisher
 ↓
RuntimeHubContract.EventAdded
```

`EventAdded` содержит уже назначенный SQLite `EventId`.

Причина:

- Web не должен видеть event до durable journal persistence;
- существующий `/hubs/runtime` уже является realtime transport Dispatcher;
- отдельный Events hub сейчас не даёт функциональной пользы.

Historical events всегда читаются REST.

---

## D-059 — Events Web разделяет server filters/paging и realtime Live merge

**Status:** Accepted

Events Web:

```text
/events
```

использует server-side filters/paging для historical state.

SignalR применяется только к новым persisted events.

В Live mode новые matching records merge-ятся в page 1 по `EventId` и сортируются newest-first.

На historical pages новые events не перестраивают текущую страницу; Web показывает pending counter.

Причина:

- оператор не теряет контекст при просмотре старой страницы;
- realtime не заменяет REST как источник исторической истины;
- UI остаётся bounded и предсказуемым.

---

## D-060 — Local users хранятся в configuration SQLite schema v5

**Status:** Accepted

V2-S07A добавляет:

```text
local_users
```

в `dispatcher.db` и повышает configuration schema:

```text
v4 → v5
```

Record:

```text
UserId
UserName
NormalizedUserName
DisplayName
Enabled
PasswordHash
```

`NormalizedUserName` имеет unique constraint и вычисляется из username как trimmed `ToUpperInvariant()` value.

Причина:

- user identity, password credential и disabled state являются низкочастотной durable security configuration;
- они не являются operational history/events и не должны увеличивать operational DB schema;
- отдельный immutable `UserId` нужен для будущих role assignments/audit references независимо от возможного изменения display/login name;
- нормализованный lookup key обеспечивает одну identity для case-вариантов username без зависимости от locale-specific SQLite collation.

Roles/permissions и audit records в schema v5 не добавляются: это V2-S08/V2-S09.

---

## D-061 — Password hashing выполняет ASP.NET Core Identity PasswordHasher, bootstrap password не имеет default value

**Status:** Accepted

Dispatcher не реализует собственную криптографическую схему для local passwords.

V2-S07A использует штатный:

```text
PasswordHasher<LocalUserConfiguration>
```

для создания `PasswordHash`.

Persistent `local_users` не содержит plaintext password, salt columns или собственные iteration/version fields. Format/version metadata контролирует platform hasher.

Первый local user создаётся только если:

```text
local_users empty
AND
Authentication:BootstrapAdministrator:Password explicitly configured
```

Default bootstrap password отсутствует.

`UserName=admin` и `DisplayName=Administrator` являются только defaults identity metadata. V2-S07A не создаёт `IsAdministrator`, role или authorization bypass. Реальная permission model появляется в V2-S08.

Причина:

- custom password KDF/salt/version format создаёт ненужный security risk;
- скрытый/default password неприемлем для bootstrap;
- bootstrap должен быть повторяемым только до появления первого local user и не должен создавать дополнительные accounts на каждом startup;
- authentication и authorization остаются разными границами.


---

## D-062 — Local authentication session использует ASP.NET Core cookie authentication и identity-only claims

**Status:** Accepted

V2-S07B не вводит собственный session/bearer token format.

Server использует:

```text
ASP.NET Core authentication
        ↓
Dispatcher.Local cookie scheme
        ↓
Dispatcher.Auth HttpOnly cookie
```

Cookie session:

```text
SameSite = Strict
SecurePolicy = SameAsRequest
IsPersistent = false
Ticket lifetime = 8 hours
SlidingExpiration = true
```

Login проверяет existing `local_users` через normalized username и тот же `PasswordHasher<LocalUserConfiguration>`, который используется для bootstrap hash.

Authenticated principal содержит только:

```text
UserId
UserName
DisplayName
```

Role и permission claims не создаются.

Existing runtime/configuration endpoints на V2-S07B не закрываются authentication-only check. Authorization policies и permission enforcement вводятся отдельно в V2-S08.

Причина:

- ASP.NET Core cookie authentication даёт platform session/ticket protection без собственного token format;
- Dispatcher Web, REST и SignalR размещены на одном origin, поэтому cookie является минимальной подходящей session boundary;
- authentication identity не должна преждевременно кодировать roles/permissions;
- разделение V2-S07/V2-S08 позволяет сначала стабилизировать identity/session semantics, а затем добавить server-side permission enforcement;
- generic `401` для invalid/disabled credentials не раскрывает через response причину отказа.

---

## D-063 — Web authentication state следует Server current-user API; UI gating не является security boundary

**Status:** Accepted

V2-S07C использует один scoped Web service:

```text
AuthenticationClient
```

как projection текущей Server session. При startup/refresh Web вызывает:

```text
GET /api/auth/current
```

и не читает `Dispatcher.Auth` напрямую. HttpOnly cookie остаётся browser/server-managed credential; отдельный bearer token, localStorage token или дублирующий client credential format не создаётся.

Anonymous user получает compact login shell без service drawer/workspace. Successful login сохраняет исходный browser route. Authenticated header показывает `DisplayName`/`UserName` и logout action, который вызывает Server `POST /api/auth/logout`.

Client-side hiding navigation/content не считается authorization. V2-S07C не добавляет role/permission checks в Web и не закрывает Server endpoints. V2-S08 обязан сначала ввести permission-based Server enforcement и только затем использовать effective permissions для Web visibility/enabled state.

Причина:

- Server уже является source of truth для authenticated identity через V2-S07B;
- same-origin HttpOnly cookie не нужно дублировать в browser storage;
- direct route refresh должен восстанавливать session через Server, а не зависеть от transient WASM memory;
- сохранение current route уменьшает лишнюю navigation state machine и возвращает инженера в исходный service context;
- UI visibility не должна создавать ложное ощущение защиты до появления Server authorization.

---

## D-064 — Roles, permissions и user-role assignments хранятся в configuration SQLite schema v6

**Status:** Accepted

V2-S08A повышает configuration schema `v5 → v6` и добавляет:

```text
security_roles
security_role_permissions
security_user_roles
```

`Dispatcher.Contracts.Authorization.PermissionNames` задаёт stable application permission identifiers. Role является набором permissions, а user получает effective permission set через one-or-more role assignments. Operational database не меняется.

Built-in roles `Viewer`, `Operator`, `Engineer`, `Administrator` имеют stable IDs и system-managed canonical mappings. Custom roles могут быть добавлены позднее через management boundary V2-S09, не меняя authorization model.

Причина:

- roles/assignments являются durable low-frequency security configuration;
- permission IDs позволяют Server endpoints проверять capability, а не hard-coded role name;
- many-to-many tables поддерживают несколько roles на user без permission duplication в user record;
- security configuration не относится к high-frequency operational/audit storage.

---

## D-065 — Effective permissions вычисляются Server catalog; cookie остаётся identity-only

**Status:** Accepted

V2-S08A вводит singleton `SecurityCatalog`, который строится из current users/roles/assignments и вычисляет union permissions per `UserId`. Disabled user не имеет effective permissions независимо от assignments.

V2-S07 cookie не расширяется role/permission claims. Principal продолжает содержать только identity (`UserId`, `UserName`, `DisplayName`).

Initial transition:

- новый bootstrap user и его built-in `Administrator` assignment сохраняются одной SQLite transaction;
- при первой v6 security initialization единственный existing local user без assignments получает `Administrator` как one-time compatibility bridge;
- при нескольких existing users Server не угадывает administrator;
- автоматическое назначение не повторяется после завершённой first role initialization, чтобы future explicit role removal не отменялся startup-ом.

Причина:

- permission snapshot в 8-hour cookie мог бы устареть после role/user disable changes;
- current Server catalog является подходящей authority для следующего permission handler;
- one-time bridge сохраняет управляемость single-user installations при первом включении authorization;
- ambiguous multi-user migration должна fail-safe не выдавать privilege случайному user.

---

## D-066 — Server authorization проверяет effective permissions через current SecurityCatalog

**Status:** Accepted

V2-S08B создаёт ASP.NET Core authorization policy для каждого declared `PermissionNames` identifier. Policy содержит authenticated-user requirement и `PermissionRequirement`, а handler разрешает request только если current `SecurityCatalog` подтверждает permission для `UserId` из `NameIdentifier` claim.

Authorization не использует:

```text
role-name checks
IsInRole("Administrator")
role claims в cookie
permission claims в cookie
```

Server permission matrix:

```text
runtime/configuration/history/events/mimic reads → Runtime.Read
RuntimeHub                                  → Runtime.Read
tag write                                   → Tags.Write
Modbus/SNMP configuration mutation          → Devices.Edit
mimic configuration mutation                → Mimics.Edit
historian policy mutation                   → Historian.Configure
```

`/health` и `/api/auth/*` остаются public. Unknown non-read `/api` mutation fail-closed до появления explicit permission mapping.

HTTP semantics:

```text
anonymous protected request                 → 401
authenticated without required permission   → 403
authenticated with required permission      → normal endpoint result
```

Cookie challenge/access-denied handlers возвращают status codes и не redirect-ят API client в Web UI.

Причина:

- Server должен быть authoritative security boundary до Web gating;
- permission evaluation по current catalog применяет disable/role changes без ожидания expiration 8-hour cookie ticket;
- business endpoints зависят от capability, а не от built-in role taxonomy;
- единый mapping существующих Server boundaries уменьшает риск случайно оставить mutation без permission;
- fail-closed unknown mutation безопаснее implicit allow при появлении нового API action.


---

## D-067 — Web получает effective permissions из current-user projection и использует их только как UX state

**Status:** Accepted

V2-S08C расширяет authenticated `CurrentUserDto`:

```text
Authenticated
UserId
UserName
DisplayName
EffectivePermissions[]
```

`EffectivePermissions[]` формируется Server из current `SecurityCatalog` при successful login и `GET /api/auth/current`. Authentication cookie остаётся identity-only; role/permission claims в ticket не добавляются.

Web использует projection для:

```text
service navigation visibility
editor route visibility/access state
Tags.Write mutation controls
Mimic Button enabled state
```

Current mapping:

```text
Monitoring / Mimics / History / Events → Runtime.Read
Device Editor                          → Runtime.Read + Devices.Edit
Mimic Editor                           → Runtime.Read + Mimics.Edit
Tag write / Mimic command              → Tags.Write
```

Direct route к editor без required permission не рендерит editor component и показывает explicit insufficient-permission state. Это не является authorization mechanism: Server policies V2-S08B остаются authoritative и могут вернуть `401/403` независимо от client projection.

Причина:

- Web должен отражать capability model, уже enforced Server-side, а не дублировать role taxonomy;
- reuse `CurrentUserDto` избегает отдельного access-token/role-claims state и дополнительного permission endpoint только для shell initialization;
- current `SecurityCatalog` остаётся source of truth, а Web projection может быть перечитана через existing current-session flow;
- editor workspace без mutation permission создаёт ложное ожидание доступности действий, поэтому dedicated editor route скрывается/не рендерится;
- runtime command elements остаются видимой частью operational representation, но mutation interaction disabled без `Tags.Write`;
- client-side hiding/disable никогда не заменяет Server authorization.

---

## D-068 — Security management mutations используют permissions, proposed-state lockout guard и current SecurityCatalog refresh

**Status:** Accepted

V2-S09A добавляет `/api/security/users` и `/api/security/roles` поверх существующей configuration SQLite schema `6`. Новая security schema не создаётся.

Authorization management boundary:

```text
user list/create/profile/Enabled → Users.Manage
role assignments                 → Roles.Manage
password reset                   → Users.Manage + Roles.Manage
custom role CRUD                 → Roles.Manage
```

Built-in roles остаются system-managed. Business code не проверяет role names для granting authority.

Password reset требует оба management permissions: `Users.Manage` сам по себе не должен становиться косвенным способом получить `Roles.Manage` через takeover credentials более привилегированного пользователя. Password hash создаётся тем же ASP.NET Core Identity `PasswordHasher<LocalUserConfiguration>`, который уже используется bootstrap/login foundation.

Перед user-disable, role-assignment replacement и custom-role update proposed state должен сохранить хотя бы одного enabled user с effective `Users.Manage + Roles.Manage`. Это capability invariant, а не special-case `Administrator`.

Security mutations сериализуются внутри одного Server process; multi-row role replacement выполняется SQLite transaction. После successful durable mutation Server перечитывает users/roles/assignments и вызывает `SecurityCatalog.ReplaceAll`, поэтому current request authorization не зависит от stale permission claims в cookie. Mutable `DisplayName` также не считается immutable cookie truth: `/api/auth/current` по `UserId` перечитывает актуальный durable user record.

Причина:

- user/role administration должна использовать уже принятую permission model;
- built-in role name не является security bypass;
- accidental removal последней management authority должен fail-safe до commit;
- current `SecurityCatalog` должен изменяться вместе с durable configuration, иначе S08B authorization временно применял бы stale authority;
- credential reset является более сильной операцией, чем display-name/Enabled update, и требует обе management capabilities.

---

## D-069 — Users/Roles Web отражает отдельные management capabilities и refresh-ит current actor projection

**Status:** Accepted

V2-S09B добавляет `/security` как Web administration service поверх S09A API. Доступ к самому route/navigation определяется:

```text
Users.Manage OR Roles.Manage
```

Внутри screen возможности разделены так же, как Server endpoints:

```text
Users.Manage                 → user management
Roles.Manage                 → role management
Users.Manage + Roles.Manage  → role assignments + password reset
```

Web не проверяет имена `Viewer`, `Operator`, `Engineer`, `Administrator` для authorization. `SecurityRoleDto.BuiltIn` используется только чтобы показать system-managed role read-only.

Layout сохраняет engineering spatial model: local Users/Roles navigation слева, плотная таблица в центре, свойства/effective permissions справа. После successful security mutation client вызывает `AuthenticationClient.RefreshAsync()` и перечитывает доступные management lists, поэтому self-change текущего actor немедленно отражается в header/navigation.

Причина:

- Web должен отражать уже существующую S09A Server capability boundary, а не создавать новую role taxonomy;
- admin-only custom role без `Runtime.Read` должен иметь доступ к security service без фиктивного operational permission;
- не следует вызывать endpoint, который current actor заведомо не имеет права читать, только для построения UI;
- built-in/custom — свойство редактируемости role entity, а не authorization shortcut;
- current actor projection после self-mutation не должна оставаться stale до browser refresh;
- client-side visibility/disabled state по-прежнему не заменяет Server policies и administrative survivability invariant.


---

## D-070 — Audit actor хранится как nullable поля immutable EventRecord в operational schema v3

**Status:** Accepted

V2-S09C повышает operational SQLite schema:

```text
v2 → v3
```

и добавляет в `events`:

```text
actor_user_id TEXT NULL
actor_user_name TEXT NULL
```

Actor identity не прячется исключительно в producer-specific `DataJson`. Existing records и system/device events могут иметь `NULL` actor. Verified authenticated actions получают actor из identity principal; login success использует verified `LocalUser`, а login failure остаётся actor-less и может хранить только bounded attempted username в payload.

Первый audit set включает login, security-management mutations, tag writes и current configuration mutations (Modbus/SNMP, Mimic, Historian policy). Password/plaintext hash в events не записывается. Existing `RuntimeConfigurationApplied` сохраняется как отдельный operational event; actor-aware `ConfigurationChanged` описывает user action.

Event Journal остаётся append-only/read-only и продолжает использовать bounded asynchronous ingestion. Отдельный mutable audit database/API или compliance-grade synchronous ledger в S09C не создаётся.

Причина:

- actor должен быть machine-readable независимо от `DataJson` конкретного producer;
- nullable columns позволяют мигрировать existing operational events без синтетической identity;
- неподтверждённый login identifier нельзя выдавать за authenticated actor;
- audit должен повторно использовать уже существующий immutable Event Journal/REST/SignalR boundary, а не создавать параллельную историю;
- separating `ConfigurationChanged` от `RuntimeConfigurationApplied` различает user intent и runtime consequence;
- credential secrets не являются audit metadata.

---

## D-071 — Alarm definitions хранятся в configuration schema v7 как protocol-neutral TagId rules без runtime state

**Status:** Accepted

V2-S10A повышает configuration SQLite:

```text
v6 → v7
```

и добавляет `alarm_definitions`. Definition имеет stable immutable `AlarmId` и binding только по logical `TagId`; отдельные Modbus address/SNMP OID в Alarm domain не попадают.

Первый condition set:

```text
DigitalTrue
DigitalFalse
High
Low
```

Digital conditions не имеют threshold/hysteresis. High/Low требуют decimal threshold и non-negative decimal hysteresis; numeric values сохраняются invariant text. Delay задаётся non-negative milliseconds. Alarm severity использует отдельный typed contract со значениями `Information/Warning/Error`, намеренно выровненными с текущей Event Journal severity taxonomy.

Create/update валидируют наличие current `TagId`, но SQLite FK на protocol tag tables не создаётся. Причина: TagId является cross-protocol logical identity, а позднейшее удаление source tag не должно каскадно уничтожать инженерную alarm configuration; stale definition должна оставаться наблюдаемой.

CRUD authorization:

```text
read      → Runtime.Read
mutation  → Alarms.Configure
```

Successful mutations используют existing actor-aware `ConfigurationChanged`. S10A не подписывается на `TagService`, не вычисляет active state, не применяет delay/hysteresis и не реализует ACK. Эти runtime semantics остаются V2-S11/V2-S12.

Причина:

- definition является low-frequency configuration, а не operational state/history;
- Alarm binding должен сохранять существующую protocol-neutral `TagId` architecture;
- lossless decimal configuration не должна зависеть от binary floating representation;
- отдельный Alarm severity type не связывает definition contract с Events implementation detail, но сохраняет одну текущую severity taxonomy;
- Server permission enforcement должен появиться раньше Web editor visibility;
- separating definition persistence from runtime state machine делает первый Alarm шаг отдельно проверяемым.

---

## D-072 — Alarm Editor использует S10A CRUD, client-side draft и `Runtime.Read + Alarms.Configure`

**Status:** Superseded by D-074 for route/navigation split; draft/editor semantics remain accepted

V2-S10B добавляет `/alarms` как engineering editor без нового Server/storage boundary. Web читает и изменяет definitions только через S10A `/api/configuration/alarms/definitions`.

Route/navigation доступны при:

```text
Runtime.Read + Alarms.Configure
```

`Runtime.Read` нужен для чтения definitions и current configured tags, а `Alarms.Configure` — для mutation workflow. Это client-side UX projection; Server S10A остаётся authoritative и проверяет read/mutation permissions отдельно.

Editor использует client-side draft + explicit Save/Delete. Persisted `AlarmId` read-only. Tag picker объединяет logical `TagId` из Modbus/SNMP configuration; stale persisted binding показывается, но не скрывается. Сохранение stale definition без current tag не имитируется Web-обходом — пользователь должен выбрать существующий TagId, после чего Server повторно валидирует mutation.

Condition-aware form не дублирует runtime evaluator: digital condition убирает Threshold/Hysteresis из request, High/Low редактируют decimal values, но delay/hysteresis/active-state semantics остаются V2-S11.

Причина:

- S10B должен завершить configuration vertical slice, а не преждевременно смешивать editor с runtime AlarmService;
- reuse S10A API сохраняет один durable source of truth;
- explicit draft/Save согласован с Device/Mimic editor interaction model;
- stale logical binding должен быть наблюдаемым, а не молча удаляться;
- permission-aware navigation не должна превращаться в альтернативную authorization систему;
- runtime condition semantics должны иметь одно место реализации в будущем Alarm state machine.


---

## D-073 — Alarm runtime использует four-state lifecycle, live definition catalog и existing Event Journal без новой operational schema

**Status:** Accepted

V2-S11 добавляет `AlarmDefinitionCatalog` и singleton hosted `AlarmRuntimeService`. Runtime подписывается на protocol-neutral `TagService.Changed` и различает:

```text
Normal
ActiveUnacknowledged
ActiveAcknowledged
ReturnedUnacknowledged
```

High/Low hysteresis применяется только в runtime:

```text
High raise  >= Threshold
High return <  Threshold - Hysteresis
Low raise   <= Threshold
Low return  >  Threshold + Hysteresis
```

Raise delay требует непрерывно active condition; pending raise отменяется при return condition, semantic definition change или shutdown и перед фактическим raise повторно проверяет current Tag value. Pending delay не становится отдельным public state.

Live definition CRUD обновляет `AlarmDefinitionCatalog`. `TagService.Cleared` при device/tag live apply отменяет pending raises; runtime state stale/removed TagId сбрасывается в `Normal`, а state всё ещё configured TagId сохраняется до нового sample. Metadata-only (`Name/Message/Severity`) change сохраняет state; evaluation-semantic change (`Enabled/TagId/Condition/Threshold/Delay/Hysteresis`) сбрасывает instance в `Normal` и переоценивает current value. Disable/delete не создают fake physical-return event.

Alarm lifecycle persistence повторно использует current immutable Event Journal:

```text
Category = System
Type = AlarmRaised | AlarmAcknowledged | AlarmReturned
Source = AlarmId
Severity = Alarm definition severity
```

Operational SQLite остаётся schema `3`; новый `EventCategory.Alarm` и отдельная mutable alarm-state table в S11 не создаются. Automatic raise/return actor-less. Internal acknowledge transition принимает optional `EventActor`, а public permission-protected ACK boundary добавляется в V2-S12.

Причина:

- Alarm evaluator не должен читать SQLite из synchronous protocol/tag callback;
- explicit four-state lifecycle закрывает active/returned ACK semantics без boolean shortcut;
- hysteresis/delay должны иметь одну Server implementation, а не дублироваться Web;
- reusing bounded Event Journal сохраняет non-blocking polling path и уже существующую durable timeline;
- новый event category сам по себе не даёт S11 функциональной ценности, но потребовал бы operational table migration/rebuild;
- configuration disable/delete нельзя выдавать за физический return condition;
- actor-aware ACK уже подготовлен в domain boundary, но authorization/API остаются отдельно проверяемым V2-S12.


---

## D-074 — Alarm operator runtime разделяет Runtime.Read, Alarms.Acknowledge и Alarms.Configure

**Status:** Accepted

V2-S12 раскрывает V2-S11 lifecycle через отдельный operator boundary:

```text
/alarms
GET /api/alarms/current
GET /api/alarms/history
RuntimeHub.AlarmChanged
```

Чтение current/history и hub connection требует `Runtime.Read`. ACK одной selected instance выполняется через:

```text
POST /api/alarms/{AlarmId}/acknowledge
```

и требует `Alarms.Acknowledge`. Engineering configuration editor переносится на `/alarms/editor` и сохраняет `Runtime.Read + Alarms.Configure`.

ACK actor извлекается только после Server authorization из authenticated principal и передаётся в existing `AlarmRuntimeService.Acknowledge`, поэтому `AlarmAcknowledged` Event Journal record получает machine-readable `ActorUserId/ActorUserName` и transition timestamp. Role names в этой цепочке не проверяются.

Current alarm state остаётся process runtime state, а durable history — existing immutable operational `events`. Dedicated history query фильтрует только `AlarmRaised`, `AlarmAcknowledged`, `AlarmReturned`; operational schema остаётся `3`, configuration schema — `7`.

SignalR не создаёт новый hub: `AlarmChanged` добавляется в existing permission-protected `RuntimeHub`. Current Tag value между lifecycle transitions продолжает приходить через existing `TagChanged`.

Причина:

- operator ACK и engineering configuration являются разными capabilities;
- actor-aware ACK должен повторно использовать уже принятую audit identity boundary;
- separate alarm table не нужна, пока Event Journal полностью хранит transition timeline;
- server-side alarm history filtering корректно работает с paging, в отличие от post-filter generic Events page;
- reuse existing RuntimeHub сохраняет один realtime transport;
- `/alarms` должен быть доступен Viewer-like runtime user, тогда как `/alarms/editor` остаётся engineering workflow;
- bulk ACK, shelving/suppression и alarm groups не нужны для минимального Phase 8 lifecycle.

---

## D-075 — Первый template use case остаётся concrete Mimic template и instantiate создаёт независимую копию

**Status:** Accepted

V2-S13A повышает configuration SQLite `v7 → v8` и добавляет `mimic_templates`. В таблице хранится только concrete Mimic fragment: `TemplateId`, name, fragment bounds, TagId parameters и relative elements. Generic `TemplateKind`, versioning и общий Template Catalog не вводятся до V2-S14, когда появится второй concrete Device/Tag use case.

Tag-bound template element задаёт либо fixed logical `TagId`, либо `TagParameterId`. Parameter resolution происходит только при instantiate:

```text
template parameter
      ↓ binding request
logical TagId
      ↓
new MimicElementConfiguration
```

Server генерирует новые `ElementId`, применяет insertion offset и сохраняет скопированные элементы прямо в current `MimicConfiguration`. Созданный instance не содержит back-reference/version template и поэтому не изменяется после template update/delete.

Permissions разделены по изменяемой сущности:

```text
template read      → Runtime.Read
template mutation  → Templates.Edit
instantiate/copy   → Mimics.Edit
```

Successful mutations повторно используют actor-aware `ConfigurationChanged`. Operational schema и runtime renderer не меняются.

Причина:

- первый concrete use case ещё не доказывает полезную generic template abstraction;
- template placeholder должен сохранять protocol-neutral `TagId` boundary;
- copy semantics исключает скрытые каскадные изменения работающих мнемосхем;
- target mimic после instantiate должен оставаться обычным existing definition, понятным текущему runtime/editor;
- право редактировать reusable template и право изменять конкретную мнемосхему являются разными capabilities;
- generic catalog/versioning следует извлекать только после появления второго реального use case V2-S14.
