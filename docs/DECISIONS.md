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

`TagService` хранит текущие значения логических Tag.

Connection state хранится отдельно в `DeviceStateService`.

---

## D-004 — Протокольные детали изолированы

**Status:** Accepted

Web и общая логика работают с `TagId`/`DeviceId`, а не Modbus register/OID.

---

## D-005 — Начинать просто

**Status:** Accepted

Не вводим преждевременно alarms, historian, roles, brokers, distributed services и generic plugin framework.

---

## D-006 — Логическая модульность раньше физического разделения

**Status:** Accepted

Core, Server и protocol components имеют явные границы, но ранняя версия выполняется в одном host.

---

## D-007 — REST + SignalR

**Status:** Accepted

REST используется для snapshot и команд, SignalR — для realtime changes.

---

## D-008 — Configuration и Runtime разделены

**Status:** Accepted

Persistent configuration не смешивается с текущими runtime values.

---

## D-009 — Репозиторий является источником истины

**Status:** Accepted

Перед каждым шагом читается актуальный `master` и необходимые файлы.

---

## D-010 — Web как плотный инженерный интерфейс

**Status:** Accepted

UI проектируется с приоритетом рабочей области и информационной плотности.

---

## D-011 — NModbus для первой реализации Modbus

**Status:** Accepted

Первая Modbus TCP реализация использует NModbus 3.x (`3.0.83`).

---

## D-012 — Device connection state отделён от TagService

**Status:** Accepted

Connection state хранится в protocol-neutral `DeviceStateService`.

---

## D-013 — Reconnect через новое соединение каждого poll-cycle

**Status:** Accepted

Каждый cycle открывает новое TCP-соединение. Persistent connection вводится только при необходимости.

---

## D-014 — Public API contracts отделены от Core

**Status:** Accepted

`Dispatcher.Contracts` не зависит от Core/Modbus/Server/Web. Web ссылается на Contracts, но не Core.

---

## D-015 — Blazor WebAssembly раздаётся тем же ASP.NET Core host

**Status:** Accepted

Client-side WASM, REST и SignalR работают с одного origin.

---

## D-016 — Core change-events являются минимальной realtime-границей

**Status:** Accepted

Core публикует in-process `Changed`; Server преобразует их в SignalR.

---

## D-017 — До persistent configuration Modbus host использует стандартную ASP.NET Core configuration

**Status:** Accepted

S07A использует strongly typed секцию `Modbus`; persistent configuration относится к S08.

---

## D-018 — Write routing выполняется по логическому TagId

**Status:** Accepted

Публичная команда:

```text
POST /api/tags/{tagId}/write
```

содержит только логический `TagId` и новое значение.

Server разрешает `TagId` в текущей configuration и только затем получает Modbus-specific:

```text
Device
UnitId
Address
Writable
```

Web не может передать произвольный Modbus address.

Причина:

- сохраняется граница `Protocol → Tag → Application`;
- server-side configuration остаётся authority для write target;
- read-only tags нельзя сделать writable подменой HTTP payload;
- будущий Web не зависит от protocol-specific addressing.

---

## D-019 — Writable является configuration metadata, а не частью TagService

**Status:** Accepted

`TagService` продолжает хранить только:

```text
TagId
Value
Timestamp
```

`Writable` берётся из текущей configuration и добавляется только в public `TagValueDto`.

Причина:

- configuration/runtime остаются разделены;
- текущее значение не становится владельцем access policy;
- S08 сможет перенести metadata в persistent model без миграции runtime store.

---

## D-020 — Phase 1 write ограничен UInt16 Holding Register FC06

**Status:** Accepted

S07B пишет только один Holding Register `UInt16` через Modbus Function Code 06.

Допустимый диапазон:

```text
0..65535
```

После успешного Modbus response значение сразу записывается в `TagService`, поэтому Web получает подтверждённое новое состояние через существующий SignalR path.

Другие data types, coils и multi-register write добавляются только вместе с соответствующей configuration model.
