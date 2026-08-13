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

Monitoring, мнемосхемы и общая runtime-логика работают с `TagId`/`DeviceId`, а не protocol address.

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

Начиная с S08 device/tag configuration хранится в SQLite через `Microsoft.Data.Sqlite`.

Schema version = `1`.

---

## D-022 — Активная configuration загружается в ConfigurationCatalog

**Status:** Accepted

SQLite — durable source of truth, `ConfigurationCatalog` — активный in-memory snapshot для protocol runtime и write routing.

---

## D-023 — Новая configuration database начинается пустой

**Status:** Accepted

Не создаются скрытые sample devices/tags.

---

## D-024 — Data type не становится фиктивно настраиваемым до реализации второго типа

**Status:** Accepted

Текущий persistent tag model соответствует реально работающему `Holding Register UInt16`.

---

## D-025 — Configuration mutations сохраняют и применяют целый snapshot

**Status:** Accepted

S09A CRUD не вводит отдельные SQL repositories для каждой сущности.

Каждая mutation:

```text
copy current snapshot
      ↓
change one device/tag
      ↓
validate whole snapshot
      ↓
SQLite ReplaceAsync transaction
      ↓
ConfigurationCatalog.Replace
      ↓
runtime ApplyAsync
```

Причина:

- текущая configuration мала;
- `ReplaceAsync` уже существует и транзакционен;
- целый snapshot упрощает validation global uniqueness `DeviceId`/`TagId`;
- не требуется преждевременная repository/unit-of-work hierarchy.

Если объём configuration станет большим, storage mutation strategy пересматривается.

---

## D-026 — Live apply перезапускает polling loops и сбрасывает runtime current state

**Status:** Accepted

После успешного сохранения configuration Server отменяет текущие Modbus polling loops и запускает их заново из нового snapshot.

Перед новым запуском очищаются:

```text
TagService
DeviceStateService
```

Причина:

после изменения Host, UnitId, Address или состава tags старое значение нельзя считать актуальным.

После live apply Web получает `ConfigurationChanged` и перечитывает runtime snapshot.

---

## D-027 — Configuration API может быть protocol-specific

**Status:** Accepted

Runtime application API остаётся protocol-neutral, но Device Editor должен редактировать реальные настройки протокола.

Поэтому S09A использует:

```text
/api/configuration/modbus/...
```

и Modbus-specific DTO.

Это не нарушает `Protocol → logical Tag → Application`, потому что protocol details видит только configuration/editor service, а monitoring/mimic runtime продолжает работать через logical tags.

---

## D-028 — Device Editor использует explicit Save поверх client-side draft

**Status:** Accepted

Редактирование свойств в S09B не вызывает server mutation автоматически.

```text
configuration snapshot
       ↓
client-side draft
       ↓
explicit Save
       ↓
REST mutation
       ↓
live apply
```

Причина:

- каждая configuration mutation приводит к runtime reconfiguration;
- auto-save на каждом вводимом символе создавал бы лишние stop/start polling cycles;
- инженер должен явно видеть момент применения configuration;
- Server остаётся authority по validation.

Dirty draft явно обозначается, а смена выбранного объекта или refresh требует подтверждения потери несохранённых изменений.
