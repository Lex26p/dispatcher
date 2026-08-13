# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Проект развивается небольшими проверяемыми шагами. Сначала создаётся минимальный рабочий контур от устройства до Web, после чего добавляются редактор устройств, новые протоколы и мнемосхемы.

## Цель первой версии

Первый законченный вертикальный срез:

```text
Modbus TCP device
        ↓
   Modbus service
        ↓
     Tag service
        ↓
 ASP.NET Core server
        ↓
 REST + SignalR
        ↓
 Blazor WebAssembly
```

Пользователь должен иметь возможность:

1. Подключить Modbus TCP устройство.
2. Читать заданные точки.
3. Видеть текущие значения в Web.
4. Записывать разрешённые значения из Web обратно в устройство.

После этого развитие идёт последовательно:

1. Редактор устройств и тегов.
2. SNMP.
3. Простая мнемосхема.
4. Дальнейшее расширение по фактическим требованиям.

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- Blazor/SignalR packages: 10.0.10.
- Modbus: NModbus 3.0.83.
- Первый протокол: Modbus TCP.
- Второй протокол: SNMP.
- Постоянное хранение конфигурации будет добавлено на этапе редактора устройств.

Target framework проекта централизованно зафиксирован как `net10.0` в `Directory.Build.props`.

`global.json` ограничивает SDK линией .NET 10:

- минимальная версия SDK: `10.0.100`;
- `rollForward: latestFeature`;
- prerelease SDK разрешены, чтобы проект мог собираться установленными preview feature band SDK .NET 10.

## Текущая структура solution

```text
Dispatcher.slnx
├── src/
│   ├── Dispatcher.Contracts/
│   ├── Dispatcher.Core/
│   ├── Dispatcher.Modbus/
│   ├── Dispatcher.Server/
│   └── Dispatcher.Web/
└── tests/
    ├── Dispatcher.Core.Tests/
    ├── Dispatcher.Modbus.Tests/
    └── Dispatcher.Server.Tests/
```

`Dispatcher.Web` зависит только от `Dispatcher.Contracts` и platform packages Blazor/SignalR. Он не ссылается на Core или Modbus.

## Runtime API и realtime

ASP.NET Core публикует:

```text
GET /health
GET /api/tags
GET /api/devices

SignalR /hubs/runtime
```

Web сначала получает snapshot через REST, затем применяет realtime-изменения SignalR.

После восстановления SignalR-соединения Web повторно загружает REST snapshot.

## Hosted Modbus runtime

С S07A `Dispatcher.Server` может запускать polling одного Modbus TCP устройства как `BackgroundService`.

Временная конфигурация находится в:

```text
src/Dispatcher.Server/appsettings.json
```

По умолчанию:

```text
Modbus:Enabled = false
```

При включении конфигурация задаёт:

```text
DeviceId
Host
Port
UnitId
PollIntervalMilliseconds
RequestTimeoutMilliseconds
Points[]
  TagId
  Address
```

Путь выполнения:

```text
appsettings / environment
        ↓
ModbusRuntimeHostedService
        ↓
ModbusPollingService
        ↓
Modbus TCP device
        ↓
TagService + DeviceStateService
        ↓
REST + SignalR
        ↓
Blazor WebAssembly
```

`Address` пока остаётся raw 0-based Modbus address.

Файловая конфигурация S07A является временным bootstrap-механизмом. Постоянная конфигурация и редактор устройств появятся в S08/S09.

## Web UI

Первый экран — `Мониторинг`.

Глобальная навигация открывается поверх рабочей области по `☰`, локальная навигация постоянно находится слева, а центральная таблица занимает основную рабочую площадь.

Панель свойств справа на мониторинге не показывается, потому что это не редактор.

Blazor WebAssembly исполняется в браузере. Его static assets, REST и SignalR раздаёт тот же `Dispatcher.Server`.

Для .NET 10 bootstrap-файл Blazor WebAssembly fingerprinted; `index.html` содержит import map и placeholder `blazor.webassembly#[.{fingerprint}].js`, а Server использует `MapStaticAssets()`.

## Текущий Modbus scope

Поддерживается:

- Modbus TCP;
- Function Code 03;
- несколько Holding Register `UInt16`;
- polling interval;
- timeout;
- reconnect через новое соединение каждого cycle;
- protocol-neutral `Online/Offline`;
- запуск polling из ASP.NET Core host для одного настроенного устройства.

Write path ещё не реализован. Он составляет S07B и завершит Phase 1.

## Основные принципы

- Репозиторий — единственный источник истины.
- Разработка идёт маленькими законченными шагами.
- Не реализуем функции «на будущее», пока они не нужны текущему этапу.
- Web и мнемосхемы работают с логическими тегами, а не с адресами конкретных протоколов.
- Протокольные детали изолируются от Core и Web.
- Текущее runtime-состояние и постоянная конфигурация — разные виды данных.
- Архитектурные границы закладываются сразу, но преждевременная микросервисная инфраструктура не вводится.
- Web проектируется как плотный инженерный интерфейс с приоритетом рабочей области.
- Перед каждым изменением сначала читается актуальное состояние репозитория.

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
