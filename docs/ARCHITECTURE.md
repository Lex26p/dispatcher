# Архитектура Dispatcher

## 1. Состояние после Phase 1

Первый вертикальный срез работает в обоих направлениях:

```text
                 read / write
Browser  ↔  Server  ↔  Core  ↔  Modbus  ↔  Device
```

Система остаётся одним ASP.NET Core host с логически разделёнными проектами.

## 2. Логическая схема

```text
┌──────────────────────────────────────────────────────┐
│ Browser                                               │
│ Dispatcher.Web — Blazor WebAssembly                  │
│ REST commands/snapshot + SignalR updates             │
└───────────────────────┬──────────────────────────────┘
                        │ same origin
┌───────────────────────▼──────────────────────────────┐
│ Dispatcher.Server — ASP.NET Core                     │
│                                                      │
│ GET /api/tags                                        │
│ GET /api/devices                                     │
│ POST /api/tags/{tagId}/write                         │
│ SignalR /hubs/runtime                                │
│                                                      │
│ ModbusRuntimeHostedService                           │
└───────────────┬──────────────────────────────────────┘
                │
      ┌─────────┴─────────┐
      │                   │
┌─────▼──────┐     ┌──────▼──────────┐
│ TagService │     │ DeviceState     │
│            │     │ Service         │
└─────▲──────┘     └──────▲──────────┘
      │                   │
      └─────────┬─────────┘
                │
       Dispatcher.Modbus
       ├── polling FC03
       └── write FC06
                │
                ▼
        Modbus TCP device
```

## 3. Core runtime

### TagService

Хранит текущее runtime-значение:

```text
TagId
Value
Timestamp
```

После `Set` публикует in-process `Changed`.

`Writable` не хранится в `TagService`: это configuration metadata, а не runtime value.

### DeviceStateService

Хранит:

```text
DeviceId
Status = Unknown | Online | Offline
UpdatedAt
LastSuccessfulPollAt
Error
```

Core не знает Modbus address, Unit ID, ASP.NET или SignalR.

## 4. Modbus read

Текущий read scope:

- Modbus TCP;
- FC03;
- Holding Register;
- `UInt16`;
- несколько points;
- timeout;
- новое соединение каждого poll-cycle.

Путь:

```text
ModbusPollingService
        ↓
ModbusTcpRegisterReader
        ↓ FC03
Device
        ↓
TagService + DeviceStateService
```

Новый poll публикует значения только после успешного чтения всего настроенного набора.

## 5. Modbus write

Текущий write scope:

- только настроенный `Writable = true` tag;
- Holding Register;
- `UInt16`;
- FC06 Write Single Register;
- значение `0…65535`.

Публичная команда не содержит protocol address:

```text
POST /api/tags/{tagId}/write
{
  "value": 3456
}
```

Server:

1. проверяет, что Modbus runtime включён;
2. находит `TagId` в текущей configuration;
3. проверяет `Writable`;
4. проверяет `UInt16`;
5. преобразует configuration в Modbus write target;
6. вызывает `ModbusWriteService`;
7. после успешного FC06 обновляет `TagService`.

```text
TagId + value
      ↓
Server validation / routing
      ↓
configured Device + Address
      ↓
ModbusWriteService
      ↓
ModbusTcpRegisterWriter
      ↓ FC06
Device
      ↓ success
TagService.Set
      ↓
SignalR
      ↓
Web
```

На этом этапе отдельный generic command bus не вводится.

## 6. Configuration

До S08 один Modbus device задаётся стандартной ASP.NET Core configuration:

```text
Modbus
├── Enabled
└── Device
    ├── DeviceId
    ├── Host
    ├── Port
    ├── UnitId
    ├── PollIntervalMilliseconds
    ├── RequestTimeoutMilliseconds
    └── Points
        ├── TagId
        ├── Address
        └── Writable
```

`Writable` по умолчанию `false`.

В S08 эта bootstrap-конфигурация заменяется persistent configuration.

## 7. Public contracts

`Dispatcher.Contracts` не зависит от Core/Modbus/Server/Web.

`TagValueDto`:

```text
TagId
Value
Timestamp
Writable
```

`Writable` нужен Web для отображения разрешённого действия, но адрес протокола наружу не публикуется.

## 8. REST + SignalR

REST:

```text
GET  /api/tags
GET  /api/devices
POST /api/tags/{tagId}/write
```

SignalR:

```text
/hubs/runtime
TagChanged
DeviceStateChanged
```

REST используется для snapshot и command request. SignalR — для realtime state changes.

Успешная write-команда возвращает обновлённый `TagValueDto` и одновременно приводит к `TagChanged` через `TagService`.

## 9. Web

`Dispatcher.Web` зависит только от `Dispatcher.Contracts`.

Monitoring screen остаётся плотным:

- локальная навигация слева;
- таблица тегов в центре;
- status устройства и SignalR видимы;
- writable row содержит компактный input и кнопку;
- read-only row явно маркируется;
- во время write кнопка блокируется;
- server/device error показывается в строке команды.

Web никогда не получает `Address`, `UnitId` или NModbus types.

## 10. Ограничения после Phase 1

Пока нет:

- persistent device/tag configuration;
- нескольких управляемых конфигурацией устройств;
- coils/input registers/discrete inputs;
- Int16/Int32/UInt32/Float32 conversion;
- grouped register reads;
- historian;
- alarms/events;
- users/roles;
- generic plugin runtime;
- distributed message broker.

Следующий шаг — S08, persistent configuration.
