# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Phase 1 Modbus → Web, Phase 2 Device Editor, Phase 3 SNMP завершены. Phase 4 начата runtime-мнемосхемой.

## Рабочая цепочка

```text
Modbus TCP ─→ Dispatcher.Modbus ─┐
                                 ├─→ TagService / DeviceStateService
SNMP v2c  ─→ Dispatcher.Snmp ────┘             ↓
                                         REST / SignalR
                                               ↓
                         Monitoring / Mimic runtime / Device Editor
```

После S11 система умеет:

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

Schema version после S11:

```text
3
```

Таблицы:

```text
modbus_devices
modbus_tags
snmp_devices
snmp_tags
mimics
```

Существующая schema version `2` автоматически мигрируется в `3` без удаления Modbus/SNMP configuration.

Таблица `mimics` хранит:

```text
mimic_id
name
width
height
elements_json
```

Elements сохраняются как internal configuration JSON. Это позволяет S12 добавить editor без смены runtime API.

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

Правая properties panel отсутствует намеренно: S11 — runtime screen, а не editor.

## Новая БД

Новая configuration database по-прежнему пустая.

S11 не создаёт скрытую sample-мнемосхему. До S12 определение можно создать через configuration API.

## Следующий этап

**S12 — минимальный редактор мнемосхемы.**

Он должен использовать существующие S11 persistence/contracts/runtime:

```text
создание/удаление mimic
добавление/удаление элементов
position / size
properties справа
TagId picker
Save
```

Runtime renderer переделывать не требуется.

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
