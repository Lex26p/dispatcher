# Архитектура Dispatcher

## 1. Состояние S10A

В системе впервые работают два protocol runtime:

```text
Modbus TCP ──→ Dispatcher.Modbus ─┐
                                  ├──→ TagService
SNMP v2c  ──→ Dispatcher.Snmp ────┘        ↓
                                     REST / SignalR
                                           ↓
                                      Monitoring
```

`TagService` и `DeviceStateService` остаются protocol-neutral.

## 2. Dispatcher.Snmp

Новый проект:

```text
Dispatcher.Snmp
├── SnmpGetClient
├── SnmpPollingService
├── SnmpValueConverter
└── Configuration
    ├── SnmpV2cDevice
    ├── SnmpPoint
    ├── SnmpPollingPlan
    └── SnmpOidValidator
```

Зависимости:

```text
Dispatcher.Snmp
├── Dispatcher.Core
└── Lextm.SharpSnmpLib
```

Он не зависит от Server, Web или Modbus.

## 3. SNMP scope

S10A реализует:

```text
SNMP version: v2c
operation:    GET
transport:    UDP
configuration: Host / Port / Community
points:       TagId / OID
```

Один poll-cycle формирует GET request с настроенным набором varbind OID.

Успешный ответ:

```text
SNMP varbinds
     ↓
SnmpValueConverter
     ↓
TagService.Set(...)
     ↓
DeviceStateService.Online
```

Ошибка DNS, timeout, protocol error, missing OID или malformed response:

```text
DeviceStateService.Offline
```

External cancellation не превращается в Offline.

## 4. SNMP values

На текущем этапе явно сохраняется тип для распространённых SMI values:

```text
Integer32   → Int32
Counter32   → UInt32
Gauge32     → UInt32
TimeTicks   → UInt32
Counter64   → UInt64
OctetString → String
Null        → null
```

Другие valid library values временно переходят в `string`.

`NoSuchObject`, `NoSuchInstance`, `EndOfMibView` считаются ошибкой poll-cycle.

## 5. Persistent configuration

SQLite schema version:

```text
2
```

Старые таблицы:

```text
modbus_devices
modbus_tags
```

Новые:

```text
snmp_devices
snmp_tags
```

### snmp_devices

```text
device_id
name
enabled
host
port
community
poll_interval_ms
request_timeout_ms
```

### snmp_tags

```text
tag_id
device_id
name
oid
```

## 6. Schema migration

Если Server открывает schema version `1`:

```text
existing Modbus data
       ↓ preserve
CREATE snmp_devices
CREATE snmp_tags
       ↓
PRAGMA user_version = 2
```

Новая empty database сразу создаётся как version `2`.

Неизвестная schema version по-прежнему приводит к startup error.

## 7. ConfigurationCatalog

`ConfigurationCatalog` теперь содержит:

```text
ModbusDevices
SnmpDevices
```

и общий индекс:

```text
DeviceId
TagId
```

`DeviceId` и `TagId` глобально уникальны между протоколами.

Причина:

```text
TagService[tagId]
DeviceStateService[deviceId]
```

имеют общие protocol-neutral keys. Два protocol owners не могут публиковать разные сущности под одним ID.

Modbus write routing остаётся отдельным `FindTag`, потому что S10A SNMP tags read-only.

## 8. Protocol runtime coordination

До второго протокола `ModbusRuntimeHostedService` самостоятельно очищал весь runtime state.

С двумя протоколами это неверно.

Теперь individual hosted services отвечают только за собственные polling loops:

```text
ModbusRuntimeHostedService
SnmpRuntimeHostedService
```

а configuration live apply координирует:

```text
RuntimeConfigurationCoordinator
```

Алгоритм:

```text
stop Modbus
stop SNMP
    ↓
clear TagService
clear DeviceStateService
    ↓
start Modbus from ConfigurationCatalog
start SNMP from ConfigurationCatalog
```

Таким образом изменение Modbus configuration не оставляет SNMP runtime остановленным или stale.

## 9. Startup

Порядок регистрации hosted services:

```text
ConfigurationInitializationHostedService
        ↓ loads Modbus + SNMP into catalog

ModbusRuntimeHostedService
        ↓

SnmpRuntimeHostedService
        ↓

RuntimeHubPublisher
```

Каждый protocol worker читает уже готовый active catalog.

## 10. Existing Device Editor

S10A намеренно не меняет UI/API редактора.

Текущий `/api/configuration/modbus/...` продолжает редактировать только Modbus.

При его mutation:

```text
replace Modbus records in SQLite
        ↓
ConfigurationCatalog.ReplaceModbus
        ↓
RuntimeConfigurationCoordinator
        ↓
restart both active protocols
```

SNMP records в SQLite не удаляются.

S10B добавит SNMP CRUD API и UI.

## 11. Runtime application boundary

Monitoring по-прежнему не знает protocol:

```text
TagId
Value
Timestamp
Writable
```

SNMP tag автоматически получает:

```text
Writable = false
```

и появляется через те же REST/SignalR endpoints, что и Modbus.

## 12. S10B boundary

S10B должен добавить configuration/editor boundary:

```text
Device Editor
├── Modbus TCP
└── SNMP v2c
```

но не создавать отдельный SNMP monitoring screen.

Операторский runtime остаётся единым.
