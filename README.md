# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Базовый цикл S00–S12 завершён. Roadmap v2 начат с Historian foundation.

## Рабочая цепочка

```text
Modbus TCP ─→ Dispatcher.Modbus ─┐
                                 ├─→ TagService / DeviceStateService
SNMP v2c  ─→ Dispatcher.Snmp ────┘             ↓
                                         REST / SignalR
                                               ↓
                         Monitoring / Mimic runtime / Device Editor
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

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- SQLite: Microsoft.Data.Sqlite 10.0.10.
- Modbus: NModbus 3.0.83.
- SNMP: Lextm.SharpSnmpLib 12.5.7.
- Mimic renderer: SVG в Blazor WebAssembly.

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

Configuration SQLite schema version после V2-S02:

```text
4
```

Таблицы:

```text
modbus_devices
modbus_tags
snmp_devices
snmp_tags
mimics
historian_policies
```

Существующая schema `1/2/3` автоматически мигрируется в `4` без удаления protocol/mimic configuration.

Таблица `mimics` хранит:

```text
mimic_id
name
width
height
elements_json
```

Elements сохраняются как internal configuration JSON. Это позволяет S12 добавить editor без смены runtime API.

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
1
```

Первая table:

```text
history_samples
├── sample_id
├── tag_id
├── timestamp_utc_ticks
├── value_type
└── value_text
```

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
}
```

`PeriodicScanMilliseconds` допускается в диапазоне `10..100`; policy interval — `100..86400000 ms`.

Default operational database:

```text
%LOCALAPPDATA%\Dispatcher\dispatcher-operational.db
```

или `data/dispatcher-operational.db`, если LocalApplicationData недоступен.

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

Новая configuration database по-прежнему пустая.

S11 не создаёт скрытую sample-мнемосхему. До S12 определение можно создать через configuration API.

## Roadmap v2

Подробное продолжение:

```text
docs/ROADMAP_V2.md
```

Текущий завершённый шаг:

```text
V2-S02 — Historian policies и retention
```

Следующий шаг:

```text
V2-S03 — History query API
```

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
