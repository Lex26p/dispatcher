# Архитектура Dispatcher

## 1. Состояние после S09B

Phase 2 завершает полный configuration path:

```text
Blazor Device Editor
        ↓
Configuration REST API
        ↓
validate
        ↓
SQLite
        ↓
ConfigurationCatalog
        ↓
Modbus runtime live apply
```

Runtime current state остаётся отдельным:

```text
Protocol workers
      ↓
TagService / DeviceStateService
      ↓
REST / SignalR
      ↓
Monitoring / future mimics
```

## 2. Web services

Глобальная навигация:

```text
Monitoring
Device Editor
```

Monitoring является runtime application UI.

Device Editor является configuration UI и поэтому может видеть protocol-specific Modbus fields.

## 3. Device Editor layout

Редактор реализует общий пространственный контракт:

```text
слева  → devices/tags selection tree
центр  → tags work table
справа → selected object properties
сверху → create/save/delete/refresh
```

Правая панель всегда относится к выбранному object.

Выбор device показывает его tags в центральной таблице. Выбор tag сохраняет ту же рабочую таблицу и выделяет строку, а справа показывает tag properties.

## 4. Client-side draft

Device Editor не отправляет mutation на каждый `input`.

```text
server snapshot
    ↓
local editable draft
    ↓
explicit Save
    ↓
configuration API
```

Причина: каждая server mutation live-применяет configuration и перезапускает polling loops. Auto-save на каждом символе был бы неправильным runtime behaviour.

При переходе к другому object или refresh с dirty draft пользователь получает browser confirmation.

## 5. ConfigurationClient

`Dispatcher.Web.Services.ConfigurationClient` инкапсулирует HTTP-вызовы:

```text
GET devices
POST/PUT/DELETE device
POST/PUT/DELETE tag
```

Он работает только с `Dispatcher.Contracts.Configuration`.

Web по-прежнему не ссылается на Server, Core или Modbus assemblies.

Server validation errors извлекаются из Problem Details `detail/title` и показываются в editor.

## 6. Configuration API

S09A endpoints не меняются:

```text
GET    /api/configuration/modbus/devices

POST   /api/configuration/modbus/devices
PUT    /api/configuration/modbus/devices/{deviceId}
DELETE /api/configuration/modbus/devices/{deviceId}

POST   /api/configuration/modbus/devices/{deviceId}/tags
PUT    /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
DELETE /api/configuration/modbus/devices/{deviceId}/tags/{tagId}
```

## 7. Runtime reconfiguration

После Save/Delete:

```text
SQLite ReplaceAsync
      ↓
ConfigurationCatalog.Replace
      ↓
cancel old polling
      ↓
clear old runtime current state
      ↓
start new polling
      ↓
ConfigurationChanged
```

Monitoring reloads current runtime snapshot.

Device Editor после собственной mutation повторно читает configuration snapshot, поэтому отображает server-confirmed state.

## 8. Protocol boundary

Monitoring и будущие Mimics работают с logical tags.

Device Editor редактирует:

```text
Host
Port
UnitId
Holding Register Address
Writable
```

Это protocol-specific configuration boundary и не переносит Modbus details в runtime application model.

## 9. Current tag model

На S09B поддержан только:

```text
Holding Register UInt16
```

Поэтому тип в редакторе показывается read-only.

Появление второго data type должно расширять configuration schema и protocol conversion вместе, а не добавлять фиктивный UI selector.

## 10. Следующий этап

S10 добавляет второй protocol component — SNMP.

Ключевая архитектурная проверка S10: Device Editor должен уметь редактировать Modbus и SNMP configuration, а Monitoring должен продолжать работать с общими `TagId` и `DeviceId`.
