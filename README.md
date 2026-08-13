# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Проект развивается небольшими проверяемыми шагами. Первый вертикальный срез Modbus → Web завершён; далее добавляются постоянная конфигурация, редактор устройств, SNMP и мнемосхемы.

## Первый вертикальный срез

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

Текущая Phase 1 умеет:

1. Запускать polling одного настроенного Modbus TCP устройства.
2. Читать несколько Holding Register `UInt16` через FC03.
3. Хранить текущие значения и Online/Offline state.
4. Показывать значения в Web через REST + SignalR.
5. Разрешать запись только явно помеченных writable-тегов.
6. Записывать `UInt16` в Holding Register через FC06 из Web.

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- Blazor/SignalR packages: 10.0.10.
- Modbus: NModbus 3.0.83.
- Первый протокол: Modbus TCP.
- Второй протокол: SNMP.
- Постоянное хранение конфигурации добавляется на этапе S08.

Target framework проекта централизованно зафиксирован как `net10.0` в `Directory.Build.props`.

`global.json` ограничивает SDK линией .NET 10:

- минимальная версия SDK: `10.0.100`;
- `rollForward: latestFeature`;
- prerelease SDK разрешены для установленного preview feature band SDK .NET 10.

## Структура solution

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
GET  /health
GET  /api/tags
GET  /api/devices
POST /api/tags/{tagId}/write

SignalR /hubs/runtime
```

`TagValueDto` содержит:

```text
TagId
Value
Timestamp
Writable
```

Web сначала получает snapshot через REST, затем применяет SignalR updates. После reconnect выполняется новый REST snapshot.

## Hosted Modbus runtime

Временная конфигурация находится в:

```text
src/Dispatcher.Server/appsettings.json
```

По умолчанию:

```text
Modbus:Enabled = false
```

Конфигурация одного устройства:

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
  Writable
```

`Writable` по умолчанию `false`. Только явно writable-точка принимает команду из Web.

Путь чтения:

```text
Modbus TCP
    ↓ FC03
ModbusPollingService
    ↓
TagService + DeviceStateService
    ↓
REST / SignalR
    ↓
Web
```

Путь записи:

```text
Web
  ↓ TagId + value
POST /api/tags/{tagId}/write
  ↓ server-side validation
TagId → configured Modbus point
  ↓
ModbusWriteService
  ↓ FC06
Modbus TCP device
  ↓ success
TagService.Set(...)
  ↓
SignalR / Web
```

Web не передаёт register address, Unit ID или другие Modbus-specific параметры.

Текущая запись ограничена одним Holding Register `UInt16`, значение должно быть целым числом `0…65535`.

Файловая конфигурация Phase 1 — bootstrap-механизм. В S08 она заменяется persistent configuration.

## Web UI

Первый экран — `Мониторинг`.

- `☰` открывает глобальную навигацию поверх рабочей области.
- локальная навигация находится слева;
- таблица текущих значений занимает основную площадь;
- Online/Offline и SignalR state видимы постоянно;
- writable-теги получают компактный input + `Записать` прямо в таблице;
- read-only теги явно помечены;
- во время команды кнопка блокируется, ошибка отображается в строке.

## Основные принципы

- Репозиторий — единственный источник истины.
- Разработка идёт маленькими законченными шагами.
- Web работает с логическими `TagId`, а не с Modbus-адресами.
- Протокольные детали изолированы от Core и Web.
- Configuration и runtime state разделены.
- Не вводим преждевременно alarms, historian, roles, brokers или distributed services.
- Web проектируется как плотный инженерный интерфейс.

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
