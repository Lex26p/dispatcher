# Архитектура Dispatcher

## 1. Состояние после S10B

Phase 3 завершает второй protocol vertical slice:

```text
Modbus TCP ─→ Dispatcher.Modbus ─┐
                                 ├─→ TagService / DeviceStateService
SNMP v2c  ─→ Dispatcher.Snmp ────┘             ↓
                                         REST / SignalR
                                               ↓
                                Monitoring + Device Editor
```

Monitoring остаётся protocol-neutral.

Device Editor является configuration boundary и видит реальные protocol-specific поля.

## 2. Persistent configuration

SQLite schema version `2`:

```text
modbus_devices
modbus_tags

snmp_devices
snmp_tags
```

`ConfigurationCatalog` содержит:

```text
ModbusDevices
SnmpDevices
```

`DeviceId` и `TagId` глобально уникальны между протоколами.

## 3. Configuration REST boundary

Два protocol-specific route groups:

```text
/api/configuration/modbus/...
/api/configuration/snmp/...
```

Это намеренно configuration API, а не runtime API.

Публичные SNMP contracts:

```text
SnmpDeviceConfigurationDto
SnmpTagConfigurationDto
SnmpDeviceUpsertRequest
SnmpTagUpsertRequest
```

SNMP device:

```text
DeviceId
Name
Enabled
Host
Port
Community
PollIntervalMilliseconds
RequestTimeoutMilliseconds
Tags[]
```

SNMP tag:

```text
TagId
Name
Oid
```

## 4. Единый ConfigurationEditorService

Modbus и SNMP mutations выполняет один singleton `ConfigurationEditorService`.

Внутри него один:

```text
SemaphoreSlim _mutationLock
```

Смысл — сериализовать configuration mutation между протоколами.

Если бы Modbus и SNMP CRUD использовали независимые lock, возможен race:

```text
Modbus reads catalog A
SNMP reads catalog A
      ↓ parallel writes
catalog/storage/runtime receive incompatible ordering
```

Общий lock сохраняет последовательную модель изменения active configuration.

## 5. Modbus mutation

```text
copy Modbus snapshot
      ↓
mutation
      ↓
validate Modbus + current SNMP
      ↓
ReplaceAsync(modbus)
      ↓
ConfigurationCatalog.ReplaceModbus
      ↓
RuntimeConfigurationCoordinator.ApplyAsync
      ↓
ConfigurationChanged
```

SNMP records в SQLite не затрагиваются.

## 6. SNMP mutation

```text
copy SNMP snapshot
      ↓
mutation
      ↓
validate current Modbus + SNMP
      ↓
ReplaceSnmpAsync(snmp)
      ↓
ConfigurationCatalog.ReplaceSnmp
      ↓
RuntimeConfigurationCoordinator.ApplyAsync
      ↓
ConfigurationChanged
```

Modbus records не затрагиваются.

## 7. Runtime live apply

Общий coordinator:

```text
stop Modbus polling
stop SNMP polling
        ↓
clear TagService
clear DeviceStateService
        ↓
start Modbus
start SNMP
```

Individual protocol hosted services не очищают global runtime state.

## 8. Device Editor data model

Web получает два configuration snapshots:

```text
GET /api/configuration/modbus/devices
GET /api/configuration/snmp/devices
```

и объединяет их только на presentation layer:

```text
DeviceItem
├── Protocol
├── common properties
└── protocol-specific source DTO
```

Это не создаёт generic protocol model в Server/Core.

Причина: общий UI действительно нужен сейчас, но protocol-specific configuration schemas остаются разными.

## 9. Device Editor layout

Сохраняется общий editor contract:

```text
слева  → единое дерево devices/tags
центр  → таблица tags выбранного device
справа → properties выбранного object
сверху → create/save/delete/refresh
```

В дереве protocol виден сразу:

```text
PLC-01        MODBUS
Switch-01     SNMP
```

Зелёный/серый marker в configuration tree означает `Enabled/Disabled`, а не Online/Offline. Реальный connection status показывается в Monitoring.

## 10. Protocol selection

Protocol выбирается при создании устройства:

```text
Modbus TCP
SNMP v2c
```

Для persisted устройства protocol selector read-only.

Причина: conversion Modbus → SNMP или SNMP → Modbus требует преобразования protocol-specific properties и tags и не является обычным field update.

Текущий безопасный workflow:

```text
delete old device
create new device with required protocol
```

## 11. SNMP Device Editor properties

Device:

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

SNMP v2c GET scope read-only, поэтому `Writable` для SNMP в editor отсутствует.

## 12. Client-side draft

Правило S09B сохраняется для обоих протоколов:

```text
server snapshot
      ↓
local draft
      ↓
explicit Save
      ↓
REST mutation
      ↓
live runtime apply
```

Auto-save не используется.

## 13. Runtime application boundary

Monitoring получает:

```text
TagId
Value
Timestamp
Writable

DeviceId
Status
UpdatedAt
LastSuccessfulPollAt
Error
```

и не знает, пришёл tag из Modbus или SNMP.

SNMP tag публикуется как:

```text
Writable = false
```

## 14. Следующий этап

S11 вводит runtime-модель простой мнемосхемы.

Ключевой принцип сохраняется:

```text
Mimic binding → TagId
```

Мнемосхема не должна хранить Modbus Address или SNMP OID.
