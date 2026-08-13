# Архитектурные решения

Этот файл фиксирует принятые решения, чтобы следующие изменения не основывались на догадках.

Если решение меняется, старое не удаляется молча: его статус меняется на `Superseded`, а ниже добавляется новое решение с причиной.

---

## D-001 — C#/.NET для Core и Server

**Status:** Accepted

Core и Server реализуются на C#/.NET.

---

## D-002 — Blazor WebAssembly для Web

**Status:** Accepted

Web-клиент реализуется на Blazor WebAssembly.

Для сложной графики в будущем допускается JavaScript interop без отказа от Blazor в остальной части приложения.

---

## D-003 — TagService как центр runtime-состояния

**Status:** Accepted

Центральной runtime-абстракцией измеряемых/управляемых данных является логический Tag.

`TagService` хранит текущие значения тегов.

Connection state устройства хранится отдельно в protocol-neutral `DeviceStateService`.

---

## D-004 — Протокольные детали изолированы

**Status:** Accepted

Web, мнемосхемы и общая предметная логика обращаются к логическим `TagId` и `DeviceId`.

Прямые зависимости верхних уровней от Modbus-register, coil, SNMP OID и других protocol-specific адресов запрещены.

---

## D-005 — Начинать просто

**Status:** Accepted

Не строим заранее полноценную SCADA.

До отдельной необходимости не вводим alarms, events, roles, historian, message broker, distributed services и сложный plugin runtime.

---

## D-006 — Логическая модульность раньше физического разделения

**Status:** Accepted

Core, Server и протокольные компоненты проектируются с явными границами, но в ранней версии могут выполняться в одном серверном процессе.

---

## D-007 — REST + SignalR

**Status:** Accepted

Для Web:

- REST используется для получения snapshot и будущих команд;
- SignalR используется для realtime-изменений.

После SignalR reconnect клиент повторно получает REST snapshot.

---

## D-008 — Configuration и Runtime разделены

**Status:** Accepted

Постоянная конфигурация устройства/тега не смешивается с текущими runtime-значениями.

---

## D-009 — Репозиторий является источником истины

**Status:** Accepted

Перед каждым шагом разработки агент обязан читать актуальный `master` репозитория и необходимые файлы.

Правила взаимодействия подробно описаны в `AGENTS.md`.

---

## D-010 — Web как плотный инженерный интерфейс

**Status:** Accepted

Web-интерфейс проектируется с приоритетом рабочей области и информационной плотности.

Подробные UI-правила зафиксированы в `docs/WEB_UI.md`.

---

## D-011 — NModbus для первой реализации Modbus

**Status:** Accepted

Первая реализация Modbus TCP использует NModbus 3.x, начиная с `3.0.83`.

Зависимость изолирована внутри `Dispatcher.Modbus`.

---

## D-012 — Device connection state отделён от TagService

**Status:** Accepted

Состояние соединения устройства хранится в `DeviceStateService`, а не кодируется специальным тегом.

---

## D-013 — Reconnect S04 через новое соединение каждого poll-cycle

**Status:** Accepted

Каждый Modbus poll-cycle открывает соединение, читает набор точек и закрывает его.

Persistent connection и сложный reconnect вводятся только при необходимости.

---

## D-014 — Public API contracts отделены от Core

**Status:** Accepted

Публичные DTO находятся в `Dispatcher.Contracts`.

```text
Dispatcher.Contracts
    └── не зависит от Core / Modbus / Server / Web

Dispatcher.Server
    ├── Dispatcher.Contracts
    └── Dispatcher.Core

Dispatcher.Web
    └── Dispatcher.Contracts
```

---

## D-015 — Blazor WebAssembly раздаётся тем же ASP.NET Core host

**Status:** Accepted

`Dispatcher.Web` остаётся client-side Blazor WebAssembly.

Статические assets раздаёт `Dispatcher.Server`, поэтому Web, REST и SignalR работают с одного origin.

Для .NET 10 Server использует `MapStaticAssets()`, а `index.html` использует fingerprinted Blazor bootstrap asset.

---

## D-016 — Core change-events являются минимальной realtime-границей

**Status:** Accepted

`TagService` и `DeviceStateService` публикуют простые in-process `Changed` events.

`Dispatcher.Server` преобразует их в SignalR через `RuntimeHubPublisher`.

Core не зависит от SignalR.

---

## D-017 — До persistent configuration Modbus host использует стандартную ASP.NET Core configuration

**Status:** Accepted

S07 разделён на два подшага:

```text
S07A hosted read runtime
S07B write path
```

В S07A один Modbus TCP device конфигурируется через strongly typed секцию `Modbus` стандартной ASP.NET Core configuration system.

По умолчанию polling выключен:

```text
Modbus:Enabled = false
```

Причина:

- Server уже должен уметь реально запускать protocol worker;
- persistent configuration относится к следующему этапу S08;
- временный config не должен требовать SQLite раньше дорожной карты;
- стандартная configuration system позволяет использовать `appsettings.json` и environment variables без собственного loader.

Поддержка нескольких persistent devices, CRUD и применение изменённой конфигурации остаются S08/S09.
