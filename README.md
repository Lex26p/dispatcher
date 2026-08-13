# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Phase 1 Modbus → Web завершена. Phase 2 завершена первым полноценным Device Editor: Modbus-устройства и теги сохраняются в SQLite, применяются к работающему polling runtime и редактируются через Blazor WebAssembly.

## Рабочая цепочка

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
8. Создавать, редактировать и удалять Modbus devices/tags через Web Device Editor.

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

## Configuration и Runtime

```text
SQLite
  ↓
ConfigurationCatalog
  ↓
Modbus runtime

TagService + DeviceStateService
  ↓
REST / SignalR
  ↓
Monitoring
```

Configuration и runtime current state остаются разными слоями.

## Web

Глобальная навигация:

```text
☰
├── Мониторинг
└── Редактор устройств
```

### Мониторинг

Показывает current tag values, Online/Offline, SignalR state и write controls для writable tags.

### Редактор устройств

URL:

```text
/devices
```

Компоновка:

```text
┌─────────────────────────────────────────────────────────────────────┐
│ ☰ Dispatcher   Редактор устройств                                  │
├──────────────┬──────────────────────────────────────┬───────────────┤
│ Devices      │ +Device +Tag Save Delete Refresh   │ Свойства      │
│ ├─ PLC-01    ├──────────────────────────────────────┤ выбранного    │
│ │  └─ Tags   │                                      │ объекта       │
│ └─ PLC-02    │      таблица тегов устройства        │               │
└──────────────┴──────────────────────────────────────┴───────────────┘
```

Редактор использует S09A configuration API.

Изменения редактируются как локальный draft и применяются только по `Сохранить`. Это важно: ввод одного символа не должен перезапускать Modbus polling. При наличии несохранённого draft UI предупреждает перед сменой объекта или refresh.

Server-side validation остаётся окончательной. Ошибка отображается непосредственно в редакторе.

Текущие свойства device:

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

Tag:

```text
TagId
Name
Holding Register UInt16
Raw Address
Writable
```

## Configuration API

```text
GET    /api/configuration/modbus/devices

POST   /api/configuration/modbus/devices
PUT    /api/configuration/modbus/devices/{deviceId}
DELETE /api/configuration/modbus/devices/{deviceId}

POST   /api/configuration/modbus/devices/{deviceId}/tags
PUT    /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
DELETE /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
```

Каждая успешная mutation сохраняется в SQLite и live-применяется к polling runtime.

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
- несколько tags/devices.
- live restart polling после configuration changes.
- Writable — configuration metadata.

## Следующий этап

S10 добавляет SNMP как второй protocol service и интегрирует его configuration в общий Device Editor без изменения runtime Web-модели logical tags.

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
