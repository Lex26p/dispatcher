# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Первый вертикальный срез Modbus → Web завершён. Phase 2 переводит конфигурацию устройств и тегов из bootstrap-файла в постоянное хранилище и затем добавляет Web-редактор.

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
5. Загружать persistent device/tag configuration из SQLite при старте.

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- SQLite provider: Microsoft.Data.Sqlite 10.0.10.
- SQLite native bundle: SQLitePCLRaw.bundle_e_sqlite3 2.1.12 (explicitly pinned to avoid the vulnerable 2.1.11 transitive resolution).
- Modbus: NModbus 3.0.83.
- Первый протокол: Modbus TCP.
- Второй протокол: SNMP.

Target framework централизованно зафиксирован как `net10.0` в `Directory.Build.props`.

## Configuration и Runtime

С S08 эти два слоя явно разделены:

```text
Persistent configuration
SQLite
  ↓ startup load
ConfigurationCatalog
  ↓
protocol runtime

Runtime state
TagService + DeviceStateService
  ↓
REST / SignalR
```

`TagService` не хранит IP, Unit ID, Address, Writable или другие configuration fields.

### SQLite configuration

Server использует:

```text
modbus_devices
modbus_tags
```

`modbus_devices` хранит:

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

`modbus_tags` хранит:

```text
TagId
DeviceId
Name
Address
Writable
```

Текущий schema version = `1` через SQLite `PRAGMA user_version`.

После загрузки БД данные копируются в in-memory `ConfigurationCatalog`. Polling, REST writable metadata, SignalR и write routing используют один и тот же catalog snapshot.

`ConfigurationDatabase:Path` разрешается при создании `SqliteConfigurationStore` из финальной DI-конфигурации host. Это позволяет стандартным configuration providers, включая test overrides, корректно задавать путь к БД.

На S08 catalog загружается один раз при старте. Live-применение изменений будет добавлено в S09 вместе с редактором.

### Где находится БД

Настройка:

```text
ConfigurationDatabase:Path
```

Если `Path` пустой, Server использует локальное application-data хранилище пользователя:

```text
%LOCALAPPDATA%\Dispatcher\dispatcher.db
```

на Windows.

Путь можно переопределить через `appsettings.json` или environment variable:

```text
ConfigurationDatabase__Path
```

Относительный явно заданный путь разрешается относительно content root `Dispatcher.Server`.

Новая БД создаётся пустой. Это намеренно: S08 не добавляет скрытую sample-конфигурацию. Создание/изменение устройств через Web относится к S09.

## Startup

Порядок:

```text
ConfigurationInitializationHostedService
        ↓
create/check SQLite schema
        ↓
load devices/tags
        ↓
ConfigurationCatalog
        ↓
ModbusRuntimeHostedService
        ↓
one polling loop per enabled device with tags
```

Таким образом persistent configuration загружена до начала protocol runtime.

## Runtime API

```text
GET  /health
GET  /api/tags
GET  /api/devices
POST /api/tags/{tagId}/write

SignalR /hubs/runtime
```

Web по-прежнему работает только с logical `TagId`; SQLite/Modbus details в public runtime API не выходят.

## Текущий Modbus scope

- Modbus TCP.
- FC03 read.
- FC06 write.
- Holding Register `UInt16`.
- несколько тегов на устройство.
- несколько persisted устройств могут быть загружены при старте.
- timeout/reconnect через новое соединение каждого polling cycle.
- Writable — configuration metadata.

## Следующий шаг

S09 добавит Web-редактор:

- список устройств;
- CRUD устройств и тегов;
- сохранение в SQLite;
- применение изменённой configuration;
- editor layout: слева выбор, центр работа, справа свойства.

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
