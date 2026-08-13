# Архитектура Dispatcher

## 1. Назначение

Dispatcher строится как расширяемая система диспетчеризации, способная получать текущие значения, хранить runtime-состояние, передавать изменения в Web и выполнять команды управления.

На ранних этапах система остаётся простой и выполняется в одном ASP.NET Core процессе.

## 2. Логическая схема S07A

```text
┌──────────────────────────────────────────────────────┐
│ Browser                                               │
│ Dispatcher.Web — Blazor WebAssembly                  │
│ REST snapshot + SignalR updates                      │
└───────────────────────┬──────────────────────────────┘
                        │ same origin
┌───────────────────────▼──────────────────────────────┐
│ Dispatcher.Server — ASP.NET Core                     │
│                                                      │
│ REST / SignalR                                       │
│       ▲                                              │
│       │                                              │
│ TagService + DeviceStateService                      │
│       ▲                                              │
│       │                                              │
│ ModbusRuntimeHostedService                           │
│       │                                              │
│ ModbusPollingService                                 │
└───────┼──────────────────────────────────────────────┘
        │
        ▼
 Modbus TCP device
```

`Dispatcher.Contracts` задаёт DTO и имена SignalR-событий между Server и Web.

## 3. Runtime services Core

### TagService

Хранит текущие значения:

```text
TagId
Value
Timestamp
```

После `Set` публикуется in-process `Changed`.

### DeviceStateService

Хранит protocol-neutral состояние:

```text
DeviceId
Status = Unknown | Online | Offline
UpdatedAt
LastSuccessfulPollAt
Error
```

После изменения публикуется in-process `Changed`.

Core не зависит от ASP.NET, SignalR или Modbus-конфигурации.

## 4. Modbus

`Dispatcher.Modbus` отвечает за protocol-specific работу.

Текущий scope:

- Modbus TCP;
- Function Code 03;
- несколько Holding Register `UInt16`;
- polling interval;
- request/connect timeout;
- reconnect через новое соединение каждого poll-cycle;
- обновление `TagService` и `DeviceStateService`.

Один poll-cycle открывает соединение, читает весь набор точек, публикует значения только после успешного чтения полного набора и закрывает соединение.

## 5. Hosted runtime S07A

`Dispatcher.Server` с S07A ссылается на `Dispatcher.Modbus`.

Запуск polling выполняет:

```text
ModbusRuntimeHostedService : BackgroundService
```

Зависимости:

```text
ModbusRuntimeHostedService
        ↓
ModbusPollingService
        ↓
TagService + DeviceStateService
```

Hosted service не содержит Modbus protocol implementation. Он только преобразует Server-конфигурацию в `ModbusPollingPlan` и запускает уже существующий polling service.

### Временная конфигурация

До S08 конфигурация одного устройства задаётся strongly typed секцией:

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
        └── Address
```

Источник — стандартная ASP.NET Core configuration system (`appsettings.json`, environment variables и другие стандартные providers).

По умолчанию `Enabled = false`, поэтому чистый запуск проекта не делает сетевых подключений.

На S08 этот bootstrap-механизм заменяется постоянной конфигурацией.

## 6. Dispatcher.Contracts

Проект не зависит от Core, Server, Modbus или Web.

Публичный runtime-контракт:

```text
TagValueDto
DeviceStateDto
DeviceConnectionStatusDto
RuntimeHubContract
```

## 7. Server/API/realtime

Server предоставляет:

```text
GET /health
GET /api/tags
GET /api/devices
SignalR /hubs/runtime
```

`RuntimeHubPublisher` преобразует Core `Changed` events в SignalR DTO.

С S07A данные для этих endpoints/events могут поступать от реально запущенного hosted Modbus polling.

## 8. Web

`Dispatcher.Web` — Blazor WebAssembly и не ссылается на Core или Modbus.

Синхронизация:

```text
REST snapshot
    ↓
SignalR changes
```

После reconnect выполняется повторный REST snapshot.

Экран мониторинга автоматически показывает теги и состояние устройства, полученные от hosted Modbus runtime. Дополнительной protocol-specific логики в Web не требуется.

## 9. Configuration и runtime

Configuration и runtime остаются разными слоями.

S07A:

```text
configuration → appsettings/environment
runtime       → in-memory Core services
```

S08:

```text
configuration → persistent storage
runtime       → in-memory Core services
```

## 10. Не входит в S07A

Не добавляются:

- Modbus write;
- writable metadata;
- device editor;
- SQLite;
- alarms/events/history;
- users/roles;
- message broker.

Write path вынесен в S07B.
