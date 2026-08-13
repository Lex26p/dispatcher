# Архитектура Dispatcher

## 1. Состояние S08

После S08 система разделяет два принципиально разных состояния:

```text
Persistent configuration
        SQLite
          ↓
 ConfigurationCatalog
          ↓
 protocol runtime

Runtime current state
 TagService / DeviceStateService
          ↓
    REST / SignalR
          ↓
         Web
```

Первый слой переживает перезапуск процесса. Второй пересоздаётся при запуске и наполняется protocol workers.

## 2. Общая схема

```text
┌──────────────────────────────────────────────────────────┐
│ Dispatcher.Web — Blazor WebAssembly                     │
│ REST commands/snapshot + SignalR updates                │
└────────────────────────┬─────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────┐
│ Dispatcher.Server                                        │
│                                                          │
│ SQLite ──load──> ConfigurationCatalog                    │
│                     │                                    │
│                     ├──> ModbusRuntimeHostedService      │
│                     ├──> write routing                   │
│                     └──> Writable metadata               │
│                                                          │
│ TagService + DeviceStateService                          │
│          │                                               │
│          └──> REST / SignalR                             │
└────────────────────────┬─────────────────────────────────┘
                         │
                 Dispatcher.Modbus
                  FC03 / FC06
                         │
                  Modbus devices
```

## 3. Persistent configuration

Текущий persistent model намеренно соответствует реализованному Modbus scope.

### ModbusDeviceConfiguration

```text
DeviceId
Name
Enabled
Host
Port
UnitId
PollIntervalMilliseconds
RequestTimeoutMilliseconds
Tags[]
```

### ModbusTagConfiguration

```text
TagId
Name
Address
Writable
```

`Address` — raw Modbus address текущего Holding Register.

Data type пока не хранится как выбираемое поле, потому что Phase 1 реализует только `UInt16`. Когда появляется второй реально поддерживаемый data type, configuration model расширяется вместе с protocol conversion.

## 4. SQLite store

`SqliteConfigurationStore` отвечает только за durable storage.

Таблицы:

```text
modbus_devices
modbus_tags
```

Схема имеет версию через:

```text
PRAGMA user_version = 1
```

Если БД имеет неизвестную ненулевую schema version, Server завершает startup с понятной ошибкой вместо молчаливого чтения несовместимой схемы.

Store предоставляет:

```text
InitializeAsync
LoadAsync
ReplaceAsync
```

`ReplaceAsync` уже даёт минимальную persistence primitive для следующего S09, но S08 не публикует CRUD API.

## 5. ConfigurationCatalog

`ConfigurationCatalog` — in-memory snapshot уже загруженной persistent configuration.

Он нужен потому, что polling и HTTP write routing не должны выполнять SQLite query на каждом poll/write.

Catalog предоставляет:

```text
Devices
FindTag(TagId)
IsTagWritable(TagId)
Replace(...)
```

Snapshot заменяется целиком. На S08 замена происходит только при startup. На S09 тот же механизм будет использован после сохранения изменений.

## 6. Startup order

Hosted services регистрируются в порядке:

```text
1. ConfigurationInitializationHostedService
2. ModbusRuntimeHostedService
3. RuntimeHubPublisher
```

Initialization service синхронно относительно startup:

1. создаёт/проверяет SQLite schema;
2. загружает devices/tags;
3. валидирует configuration;
4. устанавливает snapshot в `ConfigurationCatalog`.

Только затем запускается Modbus runtime.

## 7. Modbus runtime

`ModbusRuntimeHostedService` больше не читает `IOptions<ModbusRuntimeOptions>`.

Он читает `ConfigurationCatalog` и запускает один polling loop на каждое:

```text
Enabled = true
AND
Tags.Count > 0
```

устройство.

Disabled devices сохраняются в SQLite, но не инициируют network connection.

## 8. Write routing

Public write contract не изменился:

```text
POST /api/tags/{tagId}/write
{
  "value": 3456
}
```

Routing:

```text
TagId
  ↓
ConfigurationCatalog.FindTag
  ↓
Device + Tag persistent metadata
  ↓
validation
  ↓
Modbus write target
  ↓
FC06
```

Web по-прежнему не знает Address/UnitId.

## 9. Runtime state

`TagService` по-прежнему хранит только:

```text
TagId
Value
Timestamp
```

`DeviceStateService` хранит connection state.

Ни один из этих сервисов не является persistent configuration store.

## 10. Database location

Infrastructure setting:

```text
ConfigurationDatabase:Path
```

Если пусто, Windows-разработка использует:

```text
%LOCALAPPDATA%\Dispatcher\dispatcher.db
```

Можно задать абсолютный или content-root-relative путь.

## 11. S09 boundary

S08 не добавляет:

- configuration REST DTO;
- CRUD endpoints;
- Web editor;
- live restart/reload protocol workers;
- data types сверх `UInt16`;
- SNMP configuration.

Это следующий шаг S09.
