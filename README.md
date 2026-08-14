# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Phase 1 Modbus → Web, Phase 2 Device Editor и Phase 3 SNMP завершены.

## Рабочая цепочка

```text
Modbus TCP ─→ Dispatcher.Modbus ─┐
                                 ├─→ TagService / DeviceStateService
SNMP v2c  ─→ Dispatcher.Snmp ────┘             ↓
                                         REST / SignalR
                                               ↓
                                      Blazor WebAssembly
```

Система умеет:

1. Читать Modbus Holding Register `UInt16` через FC03.
2. Записывать разрешённые Modbus Holding Register через FC06.
3. Опросить SNMP v2c OID через GET.
4. Публиковать Modbus и SNMP через общие logical `TagId`/`DeviceId`.
5. Хранить configuration обоих протоколов в SQLite.
6. Редактировать Modbus TCP и SNMP v2c devices/tags через общий Web Device Editor.
7. Live-применять configuration без перезапуска Server.
8. Одновременно запускать Modbus и SNMP polling workers.
9. Показывать current values и Online/Offline в общем Monitoring.

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- SQLite: Microsoft.Data.Sqlite 10.0.10.
- Modbus: NModbus 3.0.83.
- SNMP: Lextm.SharpSnmpLib 12.5.7.
- SNMP scope: v2c GET.

## Runtime

```text
ModbusRuntimeHostedService ─┐
                            ├─→ TagService
SnmpRuntimeHostedService ───┘       ↓
                              REST / SignalR
```

`TagService` остаётся protocol-neutral:

```text
TagId
Value
Timestamp
```

`DeviceStateService` хранит общий Online/Offline state.

SNMP values нормализуются до обычных CLR values до публикации в `TagService`.

## Configuration

SQLite schema version:

```text
2
```

Таблицы:

```text
modbus_devices
modbus_tags
snmp_devices
snmp_tags
```

`DeviceId` и `TagId` уникальны между всеми protocol configurations.

При live apply:

```text
stop Modbus
stop SNMP
    ↓
clear current runtime state
    ↓
start Modbus
start SNMP
```

За это отвечает `RuntimeConfigurationCoordinator`.

## Configuration API

### Modbus TCP

```text
GET    /api/configuration/modbus/devices
POST   /api/configuration/modbus/devices
PUT    /api/configuration/modbus/devices/{deviceId}
DELETE /api/configuration/modbus/devices/{deviceId}

POST   /api/configuration/modbus/devices/{deviceId}/tags
PUT    /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
DELETE /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
```

### SNMP v2c

```text
GET    /api/configuration/snmp/devices
POST   /api/configuration/snmp/devices
PUT    /api/configuration/snmp/devices/{deviceId}
DELETE /api/configuration/snmp/devices/{deviceId}

POST   /api/configuration/snmp/devices/{deviceId}/tags
PUT    /api/configuration/snmp/devices/{deviceId}/tags/{tagId}
DELETE /api/configuration/snmp/devices/{deviceId}/tags/{tagId}
```

Оба API используют один `ConfigurationEditorService`, поэтому Modbus/SNMP mutations сериализуются одним mutation lock.

## Device Editor

URL:

```text
/devices
```

Компоновка не меняется:

```text
слева  → единое дерево Modbus/SNMP devices и tags
центр  → tags выбранного устройства
справа → properties выбранного device/tag
сверху → create/save/delete/refresh
```

При создании устройства выбирается:

```text
Modbus TCP
SNMP v2c
```

Для существующего устройства protocol selector read-only. Чтобы заменить протокол, устройство нужно удалить и создать заново — скрытой конвертации protocol-specific configuration нет.

### Modbus properties

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

### SNMP v2c properties

```text
DeviceId
Name
Enabled
Host
UDP Port
Community
PollIntervalMilliseconds
RequestTimeoutMilliseconds
```

Tag:

```text
TagId
Name
OID
```

SNMP tags текущего scope read-only и опрашиваются через GET.

Редактирование по-прежнему использует client-side draft + explicit `Сохранить`.

## Runtime API

```text
GET  /health
GET  /api/tags
GET  /api/devices
POST /api/tags/{tagId}/write

SignalR /hubs/runtime
```

Monitoring не знает протокол: Modbus и SNMP tags отображаются в одной runtime-модели.

## Следующий этап

**S11 — Runtime простой мнемосхемы.**

Первый scope:

```text
Text
Rectangle
Value
Indicator
Button
TagId binding
realtime
```

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
