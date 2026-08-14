# Архитектура Dispatcher

## 1. Состояние после S11

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

Device Editor → configuration
```

Mimic runtime не знает protocol-specific address.

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

## 15. Следующий этап

S12 добавляет visual editor поверх существующих:

```text
MimicDefinitionDto
MimicConfigurationService
SQLite mimics
SVG coordinate model
```

Runtime contract S11 должен остаться основной точкой исполнения.
