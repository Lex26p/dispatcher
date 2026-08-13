# Архитектура Dispatcher

## 1. Состояние S09A

Configuration теперь не только persistent, но и изменяется во время работы:

```text
Blazor Device Editor (S09B)
          ↓
Configuration REST API (S09A)
          ↓
validate
          ↓
SQLite
          ↓
ConfigurationCatalog
          ↓
Modbus runtime reconfigure
```

Runtime current state остаётся отдельным:

```text
TagService / DeviceStateService
          ↓
REST / SignalR
          ↓
Monitoring
```

## 2. Configuration API boundary

S09A добавляет protocol-specific editor API:

```text
/api/configuration/modbus/...
```

Это допустимо, потому что configuration editor редактирует именно Modbus-specific настройки.

Runtime application boundary остаётся protocol-neutral:

```text
/api/tags
/api/devices
/hubs/runtime
```

Web-monitoring и будущие мнемосхемы не получают Modbus Address/UnitId.

## 3. Configuration contracts

Публичные DTO:

```text
ModbusDeviceConfigurationDto
ModbusTagConfigurationDto
ModbusDeviceUpsertRequest
ModbusTagUpsertRequest
```

Device:

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

Tag:

```text
TagId
Name
Address
Writable
```

Data type пока не является выбираемым полем: текущий реализованный protocol scope всё ещё `Holding Register UInt16`.

## 4. ConfigurationEditorService

Все mutation-команды сериализуются через один `SemaphoreSlim`.

Алгоритм:

```text
current ConfigurationCatalog snapshot
          ↓ copy
apply requested mutation
          ↓
ModbusConfigurationValidator
          ↓
SqliteConfigurationStore.ReplaceAsync
          ↓
ConfigurationCatalog.Replace
          ↓
ModbusRuntimeHostedService.ApplyAsync
          ↓
SignalR ConfigurationChanged
```

SQLite update выполняется транзакционно существующим `ReplaceAsync`.

Duplicate `DeviceId`/`TagId` возвращаются как conflict. Неизвестные объекты — not found. Невалидные Modbus параметры — bad request.

## 5. Dynamic Modbus runtime

`ModbusRuntimeHostedService` теперь является управляемым `IHostedService`, зарегистрированным и как singleton, и как hosted service.

При startup:

```text
ConfigurationInitializationHostedService
          ↓
ConfigurationCatalog
          ↓
ModbusRuntimeHostedService.StartAsync
          ↓
ApplyAsync(current snapshot)
```

При configuration mutation:

```text
ApplyAsync(new snapshot)
          ↓
cancel old polling loops
          ↓
await graceful completion
          ↓
clear TagService / DeviceStateService
          ↓
start one loop per enabled device with tags
```

Сброс current runtime state сознательный: после изменения IP, UnitId, Address или состава tags старое значение больше нельзя считать актуальным.

Новые polling loops начинают poll немедленно, поэтому valid current state появляется заново через обычные `TagChanged`/`DeviceStateChanged`.

## 6. ConfigurationChanged

SignalR contract дополнен:

```text
ConfigurationChanged
```

Он не содержит configuration payload.

Получив событие, monitoring Web повторно читает:

```text
GET /api/tags
GET /api/devices
```

Это удаляет из UI runtime objects, которые исчезли из configuration или были сброшены при live apply.

Configuration snapshot для Device Editor запрашивается отдельным configuration API.

## 7. Persistent store

SQLite остаётся durable source of truth:

```text
modbus_devices
modbus_tags
PRAGMA user_version = 1
```

`ConfigurationCatalog` остаётся активным in-memory snapshot.

Protocol polling/write не выполняют SQLite query на каждом I/O cycle.

## 8. REST endpoints S09A

```text
GET    /api/configuration/modbus/devices

POST   /api/configuration/modbus/devices
PUT    /api/configuration/modbus/devices/{deviceId}
DELETE /api/configuration/modbus/devices/{deviceId}

POST   /api/configuration/modbus/devices/{deviceId}/tags
PUT    /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
DELETE /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
```

## 9. Concurrency model

На текущем этапе configuration mutations редкие и выполняются одним Server process.

Поэтому один in-process mutation lock достаточен.

Не вводятся:

- optimistic concurrency tokens;
- distributed locks;
- change journal;
- message broker.

Если появится multi-writer/distributed configuration, решение пересматривается.

## 10. S09B boundary

S09A не меняет основную компоновку Web.

S09B использует зафиксированные правила редакторов:

```text
слева  → selection/structure
центр  → work area
справа → selected-object properties
сверху → только необходимые actions
```

`docs/WEB_UI.md` остаётся источником UI-правил.
