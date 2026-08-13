# Архитектурные решения

Если решение меняется, старое не удаляется молча: его статус меняется на `Superseded`, а ниже добавляется новое решение с причиной.

---

## D-001 — C#/.NET для Core и Server

**Status:** Accepted

Core и Server реализуются на C#/.NET.

---

## D-002 — Blazor WebAssembly для Web

**Status:** Accepted

Web-клиент реализуется на Blazor WebAssembly.

---

## D-003 — TagService как центр runtime-состояния

**Status:** Accepted

`TagService` хранит текущие значения логических Tag. Connection state хранится отдельно в `DeviceStateService`.

---

## D-004 — Протокольные детали изолированы

**Status:** Accepted

Web работает с `TagId`/`DeviceId`, а не protocol address.

---

## D-005 — Начинать просто

**Status:** Accepted

Не вводим преждевременно alarms, historian, roles, brokers, distributed services и generic plugin framework.

---

## D-006 — Логическая модульность раньше физического разделения

**Status:** Accepted

Компоненты имеют явные границы, но ранняя версия выполняется в одном host.

---

## D-007 — REST + SignalR

**Status:** Accepted

REST используется для snapshot/commands, SignalR — для realtime changes.

---

## D-008 — Configuration и Runtime разделены

**Status:** Accepted

Persistent configuration не смешивается с runtime current values.

---

## D-009 — Репозиторий является источником истины

**Status:** Accepted

Перед каждым шагом читается актуальный `master`.

---

## D-010 — Web как плотный инженерный интерфейс

**Status:** Accepted

UI проектируется с приоритетом рабочей области и информационной плотности.

---

## D-011 — NModbus для первой реализации Modbus

**Status:** Accepted

Modbus TCP использует NModbus 3.x (`3.0.83`).

---

## D-012 — Device connection state отделён от TagService

**Status:** Accepted

Connection state хранится в `DeviceStateService`.

---

## D-013 — Reconnect через новое соединение каждого poll-cycle

**Status:** Accepted

Каждый cycle открывает новое TCP-соединение.

---

## D-014 — Public API contracts отделены от Core

**Status:** Accepted

`Dispatcher.Contracts` не зависит от Core/Modbus/Server/Web.

---

## D-015 — Blazor WebAssembly раздаётся тем же ASP.NET Core host

**Status:** Accepted

WASM, REST и SignalR работают с одного origin.

---

## D-016 — Core change-events являются минимальной realtime-границей

**Status:** Accepted

Core `Changed` events преобразуются Server в SignalR.

---

## D-017 — До persistent configuration Modbus host использует стандартную ASP.NET Core configuration

**Status:** Superseded by D-021

S07A/S07B использовали `appsettings` как временный источник device/tag configuration.

---

## D-018 — Write routing выполняется по логическому TagId

**Status:** Accepted

Server разрешает `TagId` в текущей configuration и только затем получает Modbus target.

---

## D-019 — Writable является configuration metadata, а не частью TagService

**Status:** Accepted

`TagService` хранит `TagId/Value/Timestamp`; `Writable` принадлежит configuration.

---

## D-020 — Phase 1 write ограничен UInt16 Holding Register FC06

**Status:** Accepted

Write поддерживает `UInt16` `0..65535` через FC06.

---

## D-021 — Persistent configuration хранится в SQLite

**Status:** Accepted

Начиная с S08 device/tag configuration хранится в SQLite.

Используется `Microsoft.Data.Sqlite`, без EF Core.

Причина:

- текущая схема мала и хорошо выражается двумя таблицами;
- ADO.NET provider даёт явное управление schema/load/save;
- не требуется отдельный migration CLI/tooling на текущем этапе;
- при необходимости schema evolution можно реализовать по `PRAGMA user_version`.

Текущая schema version:

```text
1
```

---

## D-022 — Активная configuration загружается в ConfigurationCatalog

**Status:** Accepted

SQLite является durable source of truth для конфигурации, но protocol runtime не читает БД на каждом poll/write.

При startup:

```text
SQLite
  ↓
validate
  ↓
ConfigurationCatalog
```

Polling, Writable metadata и write routing используют один in-memory snapshot.

S09 будет обновлять SQLite и затем заменять catalog snapshot.

---

## D-023 — Новая configuration database начинается пустой

**Status:** Accepted

S08 не создаёт скрытые sample devices/tags.

Причина:

- sample device не должен выглядеть как реальная configuration;
- disabled localhost-запись не несёт продуктовой ценности;
- следующий S09 предоставляет штатный Web CRUD.

Пустая configuration означает отсутствие protocol network connections.

---

## D-024 — Data type не становится фиктивно настраиваемым до реализации второго типа

**Status:** Accepted

Persistent tag model S08 соответствует реально работающему scope:

```text
Holding Register UInt16
```

Поэтому UI/configuration пока не хранит выбираемый `DataType`.

Когда добавляется следующий поддерживаемый тип (`Int16`, `Int32`, `Float32` и т.д.), model/schema расширяются вместе с фактическим conversion path.
