# Архитектура Dispatcher

## 1. Назначение

Dispatcher строится как расширяемая система диспетчеризации, способная:

- получать текущие значения от устройств;
- хранить актуальное runtime-состояние;
- передавать изменения заинтересованным компонентам;
- выполнять команды управления;
- подключать дополнительные протоколы без переделки Web и основной логики;
- в дальнейшем отображать данные на мнемосхемах.

На ранних этапах система должна оставаться простой. Архитектурные границы нужны для развития, но каждый отдельный компонент не обязан сразу становиться отдельным процессом или сетевым микросервисом.

## 2. Логическая схема

```text
                         ┌─────────────────────┐
                         │ Blazor WebAssembly  │
                         │        Web          │
                         └──────────┬──────────┘
                                    │
                              REST / SignalR
                                    │
                         ┌──────────▼──────────┐
                         │   ASP.NET Core      │
                         │      Server         │
                         └──────────┬──────────┘
                                    │
                    Dispatcher.Contracts
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
          ┌─────────▼─────────┐           ┌────────▼────────┐
          │    TagService     │           │ DeviceState     │
          │ current tag data  │           │    Service      │
          └─────────▲─────────┘           └────────▲────────┘
                    │                               │
                    └───────────────┬───────────────┘
                                    │
                         ┌──────────┴──────────┐
                         │                     │
              ┌──────────▼─────────┐  ┌───────▼────────┐
              │  Modbus service   │  │  SNMP service  │
              │   first stage     │  │     later      │
              └──────────┬─────────┘  └───────┬────────┘
                         │                     │
                      Devices               Devices
```

## 3. Центральная модель: Tag

Верхние уровни системы работают с логическим идентификатором тега.

Примеры:

```text
pump01.running
pump01.pressure
boiler01.temperature
switch01.port01.status
```

Верхний уровень не должен использовать:

```text
Holding Register 40001
Coil 17
OID 1.3.6.1....
```

Эти данные принадлежат конкретному протокольному адаптеру и конфигурации устройства.

### Runtime-значение тега

Текущее значение внутри Core представлено `TagValue`:

```text
TagId
Value
Timestamp
```

Connection state устройства хранится отдельно от значения тега. `Quality` конкретного тега пока не добавляется: последнее успешно прочитанное значение остаётся в `TagService`, а доступность источника определяется через `DeviceStateService`.

## 4. Runtime services Core

### TagService

`TagService` — владелец текущих значений тегов.

Он предоставляет:

```text
Set(tagId, value)
Set(tagId, value, timestamp)
Get(tagId)
GetAll()
```

### DeviceStateService

Connection state хранится в protocol-neutral `DeviceStateService`.

Минимальное состояние:

```text
DeviceId
Status = Unknown | Online | Offline
UpdatedAt
LastSuccessfulPollAt
Error
```

Core не хранит Modbus address, Unit ID или другие protocol-specific данные состояния.

## 5. Modbus

Modbus расположен в отдельном проекте `Dispatcher.Modbus`.

### Конфигурация S04

```text
ModbusTcpDevice
├── DeviceId
├── Host
├── Port
└── UnitId

ModbusPollingPlan
├── Device
├── Points
├── PollInterval
└── RequestTimeout

ModbusHoldingRegisterPoint
├── TagId
└── Address
```

`Address` — raw 0-based protocol address, передаваемый в Function Code 03.

### Poll cycle

Один poll-cycle:

1. открывает одно TCP-соединение;
2. последовательно читает все Holding Register points;
3. только после успешного чтения всего набора обновляет `TagService`;
4. переводит устройство в `Online`;
5. закрывает соединение.

При ошибке устройство переводится в `Offline`, а следующий cycle снова устанавливает соединение.

## 6. Server и API

Начиная с S05 `Dispatcher.Server` регистрирует:

```text
TagService          Singleton
DeviceStateService  Singleton
```

REST snapshot endpoints:

```text
GET /health
GET /api/tags
GET /api/devices
```

Server не отдаёт Core-типы напрямую наружу. На HTTP-границе они преобразуются в DTO из dependency-free проекта `Dispatcher.Contracts`.

```text
Core runtime model
       ↓
Dispatcher.Server
       ↓ mapping
Dispatcher.Contracts
       ↓ JSON
Web / external client
```

`Dispatcher.Contracts` не зависит от Core, Modbus или Server. На S06 этот же проект может использовать Blazor-клиент.

На S05 Server ещё не владеет конфигурацией Modbus и не запускает polling worker автоматически. Его задача на этом шаге — предоставить корректную REST-границу над runtime state.

## 7. Web

Web реализуется на Blazor WebAssembly.

Web не должен ссылаться на Core или Modbus. Он работает через HTTP/SignalR и DTO из `Dispatcher.Contracts`.

Для связи:

- REST — получение snapshot и выполнение будущих команд;
- SignalR — realtime-изменения, начиная с S06.

### Базовая компоновка Web

Web проектируется как инженерный рабочий инструмент с высокой информационной плотностью.

Основная схема экранов сервисов:

```text
┌──────────────────────────────────────────────────────────────────┐
│ ☰  Global header                                                │
├──────────────┬──────────────────────────────────┬────────────────┤
│ Local        │ Optional contextual toolbar      │ Properties     │
│ navigation   ├──────────────────────────────────┤                │
│              │                                  │                │
│              │          Work area               │                │
│              │                                  │                │
└──────────────┴──────────────────────────────────┴────────────────┘
```

Полные правила находятся в `docs/WEB_UI.md`.

## 8. Configuration и runtime

Постоянная конфигурация и runtime state разделены.

### Configuration

Примеры:

- имя устройства;
- IP;
- port;
- protocol;
- Unit ID;
- Tag ID;
- register/OID;
- data type;
- polling interval;
- writable.

Постоянное хранилище конфигурации добавляется на этапе редактора устройств.

### Runtime

Примеры:

- текущее значение;
- timestamp;
- connection state;
- quality.

Runtime-данные на первом этапе живут в памяти.

## 9. Развёртывание на раннем этапе

Логические компоненты не обязаны быть отдельными процессами.

Допустимо:

```text
Browser
   │
   ▼
ASP.NET Core process
   ├── Server
   ├── Core runtime services
   └── protocol workers
```

Это уменьшает инфраструктурную сложность.

## 10. Не входит в первые этапы

До отдельного решения не добавляем:

- alarms;
- event journal;
- historian;
- authentication;
- roles/permissions;
- reports;
- scripts;
- redundancy;
- distributed message brokers;
- Kubernetes;
- преждевременное разделение на множество сетевых сервисов.

Эти функции рассматриваются после появления рабочего контура Modbus → Web, редактора устройств, SNMP и простой мнемосхемы.
