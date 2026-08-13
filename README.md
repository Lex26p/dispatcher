# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Первый вертикальный срез Modbus → Web завершён. Phase 2 переводит конфигурацию устройств и тегов в постоянное хранилище и Web-редактор.

## Рабочий вертикальный срез

```text
Modbus TCP device
        ↕
   Modbus service
        ↕
     Tag service
        ↕
 ASP.NET Core server
        ↕
 REST + SignalR
        ↕
 Blazor WebAssembly
```

Система умеет:

1. Читать несколько Holding Register `UInt16` через FC03.
2. Хранить текущие значения и Online/Offline state.
3. Показывать значения в Web через REST + SignalR.
4. Записывать явно разрешённые Holding Register через FC06.
5. Хранить device/tag configuration в SQLite.
6. CRUD-ить Modbus configuration через REST.
7. Применять изменённую configuration к polling без перезапуска Server.

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- SQLite provider: Microsoft.Data.Sqlite 10.0.10.
- SQLite native bundle: SQLitePCLRaw.bundle_e_sqlite3 2.1.12.
- Modbus: NModbus 3.0.83.
- Первый протокол: Modbus TCP.
- Второй протокол: SNMP.

Target framework централизованно зафиксирован как `net10.0` в `Directory.Build.props`.

## Configuration и Runtime

```text
Persistent configuration
SQLite
  ↓
ConfigurationCatalog
  ↓
Modbus runtime

Runtime state
TagService + DeviceStateService
  ↓
REST / SignalR
```

`TagService` не хранит IP, Unit ID, Address, Writable или другие configuration fields.

## SQLite configuration

Server использует:

```text
modbus_devices
modbus_tags
```

`modbus_devices`:

```text
DeviceId
Name
Enabled
Host
Port
UnitId
PollIntervalMilliseconds
RequestTimeoutMilliseconds
```

`modbus_tags`:

```text
TagId
DeviceId
Name
Address
Writable
```

Schema version = `1` через SQLite `PRAGMA user_version`.

Если `ConfigurationDatabase:Path` пустой, Windows-разработка использует:

```text
%LOCALAPPDATA%\Dispatcher\dispatcher.db
```

## Configuration API S09A

```text
GET    /api/configuration/modbus/devices

POST   /api/configuration/modbus/devices
PUT    /api/configuration/modbus/devices/{deviceId}
DELETE /api/configuration/modbus/devices/{deviceId}

POST   /api/configuration/modbus/devices/{deviceId}/tags
PUT    /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
DELETE /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
```

API работает с Modbus configuration DTO, потому что это configuration/editor boundary. Runtime Web по-прежнему работает с protocol-neutral `TagId`/`DeviceId`.

Каждая успешная mutation выполняет:

```text
build new snapshot
      ↓
validate
      ↓
SQLite ReplaceAsync transaction
      ↓
ConfigurationCatalog.Replace
      ↓
stop old polling loops
      ↓
clear old runtime current state
      ↓
start polling from new snapshot
      ↓
SignalR ConfigurationChanged
```

`ConfigurationChanged` заставляет уже открытый monitoring-клиент перечитать `/api/tags` и `/api/devices`, поэтому удалённые или переименованные объекты не остаются на экране как stale runtime state.

## Runtime API

```text
GET  /health
GET  /api/tags
GET  /api/devices
POST /api/tags/{tagId}/write

SignalR /hubs/runtime
```

SignalR events:

```text
TagChanged
DeviceStateChanged
ConfigurationChanged
```

## Текущий Modbus scope

- Modbus TCP.
- FC03 read.
- FC06 write.
- Holding Register `UInt16`.
- несколько тегов на устройство;
- несколько persisted устройств;
- live restart polling при изменении configuration;
- Writable — configuration metadata.

## Следующий шаг

**S09B** добавит сам Blazor Device Editor:

```text
слева  → список устройств/тегов
центр  → рабочая таблица/структура
справа → свойства выбранного объекта
сверху → Create/Delete/Save actions
```

Он будет использовать уже готовый S09A configuration API.

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
