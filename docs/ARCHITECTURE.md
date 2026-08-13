# Архитектура Dispatcher

## 1. Назначение

Dispatcher строится как расширяемая система диспетчеризации, способная:

- получать текущие значения от устройств;
- хранить актуальное runtime-состояние;
- передавать изменения заинтересованным компонентам;
- выполнять команды управления;
- подключать дополнительные протоколы без переделки Web и основной логики;
- в дальнейшем отображать данные на мнемосхемах.

На ранних этапах система должна оставаться простой.

## 2. Логическая схема S06

```text
┌──────────────────────────────────────────────────────┐
│ Browser                                               │
│ Dispatcher.Web — Blazor WebAssembly                  │
│                                                      │
│ REST snapshot + SignalR updates                      │
└───────────────────────┬──────────────────────────────┘
                        │ same origin
┌───────────────────────▼──────────────────────────────┐
│ Dispatcher.Server — ASP.NET Core                     │
│                                                      │
│ REST                    RuntimeHub                   │
│  │                         ▲                         │
│  │                         │                         │
│  └─────────────┬───────────┘                         │
│                │                                     │
│       RuntimeHubPublisher                            │
└────────────────┼─────────────────────────────────────┘
                 │
       ┌─────────┴─────────┐
       │                   │
┌──────▼──────┐     ┌──────▼──────────┐
│ TagService  │     │ DeviceState     │
│             │     │ Service         │
└──────▲──────┘     └──────▲──────────┘
       │                   │
       └─────────┬─────────┘
                 │
        protocol services
                 │
               Devices
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

API:

```text
Set
Get
GetAll
```

С S06 после `Set` публикуется in-process событие `Changed`.

### DeviceStateService

Хранит protocol-neutral состояние:

```text
DeviceId
Status = Unknown | Online | Offline
UpdatedAt
LastSuccessfulPollAt
Error
```

С S06 после изменения состояния публикуется in-process событие `Changed`.

Эти события не содержат SignalR или ASP.NET типов.

## 4. Modbus

Modbus остаётся отдельным проектом `Dispatcher.Modbus`.

Один poll-cycle:

1. открывает TCP-соединение;
2. читает настроенные Holding Register points;
3. обновляет `TagService`;
4. обновляет `DeviceStateService`;
5. закрывает соединение.

На S06 автоматический запуск polling из Server ещё не подключён. Это остаётся частью завершения Phase 1.

## 5. Dispatcher.Contracts

Проект не зависит от Core, Server, Modbus или Web.

Содержит:

```text
TagValueDto
DeviceStateDto
DeviceConnectionStatusDto
RuntimeHubContract
```

`RuntimeHubContract` фиксирует:

```text
Path = /hubs/runtime
TagChanged
DeviceStateChanged
```

Так строки realtime-протокола не дублируются между Server и Web.

## 6. Server

Runtime services зарегистрированы как singleton:

```text
TagService
DeviceStateService
```

Server предоставляет:

```text
GET /health
GET /api/tags
GET /api/devices
SignalR /hubs/runtime
```

### RuntimeHubPublisher

Server подписывается на in-process `Changed` события Core и преобразует их в публичные DTO:

```text
TagService.Changed
      ↓
RuntimeHubPublisher
      ↓
TagChanged
      ↓
SignalR clients
```

То же применяется для `DeviceStateService`.

Core не знает о SignalR.

## 7. Web

`Dispatcher.Web` — Blazor WebAssembly.

Он зависит только от:

```text
Dispatcher.Contracts
Microsoft.AspNetCore.Components.WebAssembly
Microsoft.AspNetCore.SignalR.Client
```

Web не ссылается на Core или Modbus.

### Синхронизация runtime state

При загрузке:

```text
GET /api/tags
GET /api/devices
        ↓
initial snapshot
```

Затем:

```text
SignalR
├── TagChanged
└── DeviceStateChanged
```

При восстановлении SignalR-соединения Web снова читает REST snapshot. Это закрывает окно пропущенных событий во время reconnect.

### Hosting

Blazor WebAssembly исполняется в браузере.

В текущем deployment layout статические assets `Dispatcher.Web` раздаёт `Dispatcher.Server`:

```text
Browser
   ↓
http://host/
   ├── Blazor static assets
   ├── /api/*
   └── /hubs/runtime
```

Это same-origin deployment и не требует CORS.

Для .NET 10 framework assets публикуются через endpoint-based static assets (`MapStaticAssets`). `index.html` содержит `<script type="importmap"></script>`, а bootstrap script задаётся через fingerprint placeholder:

```text
_framework/blazor.webassembly#[.{fingerprint}].js
```

Во время build placeholder заменяется фактическим fingerprinted именем файла.

## 8. UI layout S06

Экран мониторинга:

```text
┌───────────────────────────────────────────────────────────┐
│ ☰ Dispatcher   Мониторинг                                │
├─────────────────┬─────────────────────────────────────────┤
│ Local nav       │ compact workspace toolbar              │
│                 ├─────────────────────────────────────────┤
│ Все данные      │                                         │
│                 │ current tags table                      │
│ Устройства      │                                         │
│ device01        │                                         │
│ ...             │                                         │
└─────────────────┴─────────────────────────────────────────┘
```

Глобальная навигация открывается overlay-панелью через `☰`.

Правая панель свойств не показывается на мониторинге, потому что текущий экран не является редактором.

Полные постоянные правила находятся в `docs/WEB_UI.md`.

## 9. Configuration и runtime

Постоянная конфигурация и runtime state разделены.

На текущем этапе runtime живёт в памяти.

Постоянное хранение конфигурации добавляется на этапе редактора устройств.

## 10. Не входит в S06

Не добавляются:

- alarms;
- event journal;
- historian;
- users/roles;
- сложная UI-библиотека;
- charts;
- device editor;
- Modbus write;
- сложный message bus.

Следующий технический результат — завершить Phase 1 реальным hosted polling и write path.
