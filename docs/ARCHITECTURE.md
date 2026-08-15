# Архитектура Dispatcher

## 1. Состояние после V2-S01

В application layer появляется третий пользовательский runtime service:

```text
Protocol workers
      ↓
TagService / DeviceStateService
      ↓
REST + SignalR
      ↓
┌────────────┬───────────────┐
│ Monitoring │ Mimic runtime │
└────────────┴───────────────┘

Device Editor → device configuration
Mimic Editor  → mimic definition
```

Mimic runtime и Mimic Editor не знают protocol-specific address.

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

SQLite schema version:

```text
3
```

Новая table:

```text
mimics
├── mimic_id
├── name
├── width
├── height
└── elements_json
```

Schema migration:

```text
v1
 ↓ existing migration
v2
 ↓ create mimics
v3
```

При `v2 → v3` Modbus/SNMP tables не перестраиваются и не очищаются.

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
```

V2-S01 вводит отдельную operational database:

```text
dispatcher-operational.db
```

Причина разделения:

- history samples имеют существенно более высокую частоту записи;
- configuration lifecycle не должен зависеть от роста operational data;
- будущая замена historian/events storage не должна требовать переноса device/mimic configuration.

Обе базы пока используют `Microsoft.Data.Sqlite`, но имеют независимые schema versions.

## 21. Operational SQLite schema v1

Первая operational schema:

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

Operational schema version:

```text
PRAGMA user_version = 1
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

Historian:

1. создаёт/проверяет operational schema;
2. подписывается на `TagService.Changed`;
3. запускает writer;
4. только затем начинают стартовать protocol polling services.

Это исключает нормальное startup-окно, в котором первый successful poll мог бы пройти до Historian subscription.

## 26. V2-S01 scope boundary

На этом шаге Historian сохраняет все `TagService.Changed`.

Не реализуются ещё:

```text
Historian policies
Periodic sampling
Retention cleanup
History query API
Trend Web
```

Они остаются соответственно V2-S02, V2-S03 и V2-S04.

Configuration schema v3 не меняется.
