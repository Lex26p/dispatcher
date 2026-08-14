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

Persistent Modbus tag model соответствует реально работающему `Holding Register UInt16`.

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

**Status:** Superseded by D-031

В S09A один Modbus runtime мог самостоятельно очистить global runtime state. После появления SNMP очистка координируется между протоколами.

---

## D-027 — Configuration API может быть protocol-specific

**Status:** Accepted

Runtime application API остаётся protocol-neutral, но Device Editor должен редактировать реальные настройки протокола.

Поэтому configuration API может иметь protocol-specific contracts/endpoints, тогда как Monitoring и Mimics продолжают работать с logical tags.

---

## D-028 — Device Editor использует explicit Save поверх client-side draft

**Status:** Accepted

Редактирование свойств не вызывает server mutation автоматически.

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

---

## D-029 — Первый SNMP scope — v2c GET

**Status:** Accepted

S10A использует `Lextm.SharpSnmpLib 12.5.7`.

Поддерживается:

```text
SNMP v2c
GET
UDP
Community
OID polling
```

Не добавляются пока:

```text
SNMP SET
SNMP v3
TRAP/INFORM receiver
WALK discovery
MIB browser
```

Причина: первый use case — polling конкретных OID в общие logical tags.

---

## D-030 — DeviceId и TagId глобально уникальны между протоколами

**Status:** Accepted

Modbus и SNMP не могут использовать одинаковые logical IDs.

```text
Modbus DeviceId ─┐
SNMP DeviceId   ─┴─ unique

Modbus TagId ────┐
SNMP TagId ──────┴─ unique
```

Причина: `TagService` и `DeviceStateService` являются общими runtime stores и индексируются этими ID.

---

## D-031 — Global runtime state очищает только RuntimeConfigurationCoordinator

**Status:** Accepted

Individual protocol hosted services:

```text
ModbusRuntimeHostedService
SnmpRuntimeHostedService
```

управляют только собственными polling loops.

При configuration live apply общий coordinator выполняет:

```text
stop all protocol polling
        ↓
clear TagService / DeviceStateService
        ↓
start all protocol polling
```

Это предотвращает ситуацию, когда изменение одного протокола уничтожает current state другого и не запускает его заново.

---

## D-032 — SQLite schema v2 добавляет SNMP с миграцией v1 → v2

**Status:** Accepted

Schema version `2` добавляет:

```text
snmp_devices
snmp_tags
```

При upgrade с version `1` Modbus tables и records сохраняются.

Не допускается требование удалить существующую user database ради добавления второго протокола.

---

## D-033 — SNMP values нормализуются до обычных CLR values до TagService

**Status:** Accepted

`TagService` не хранит SharpSnmpLib-specific `ISnmpData`.

Перед публикацией выполняется conversion:

```text
Integer32 / counters / gauge / timeticks / string
        ↓
CLR primitive / string
        ↓
TagService
```

Таким образом Server/Web/mimics не зависят от SNMP library types.

---

## D-034 — Modbus и SNMP configuration mutations сериализуются одним lock

**Status:** Accepted

После появления второго protocol editor Modbus и SNMP mutations выполняет один singleton `ConfigurationEditorService` с одним `SemaphoreSlim`.

Причина:

- оба протокола разделяют `ConfigurationCatalog`;
- `DeviceId` и `TagId` имеют cross-protocol uniqueness;
- после каждой mutation перезапускается общий protocol runtime;
- независимые locks позволили бы двум параллельным mutation работать от разных snapshots и применять runtime в недетерминированном порядке.

Storage остаётся protocol-specific:

```text
ReplaceAsync(modbus)
ReplaceSnmpAsync(snmp)
```

но sequencing configuration changes является общим.

---

## D-035 — Protocol существующего устройства не конвертируется field update-ом

**Status:** Accepted

В Device Editor protocol выбирается при создании:

```text
Modbus TCP
SNMP v2c
```

Для persisted устройства protocol selector read-only.

Причина: Modbus и SNMP имеют разные protocol-specific свойства и tag schemas. Изменение одного enum-поля не определяет, как преобразовать:

```text
UnitId / Address / Writable
        ↕
Community / OID
```

Текущий явный workflow смены протокола:

```text
delete device
create device with another protocol
```

Если в будущем потребуется migration/conversion wizard, это будет отдельная операция, а не обычный update.

---

## D-036 — Runtime мнемосхемы хранится как persistent definition в SQLite

**Status:** Accepted

S11 добавляет `mimics` в общую configuration database и повышает schema version до `3`.

Definition содержит canvas metadata и список элементов.

Причина:

- S12 editor должен сохранять тот же definition, который исполняет runtime;
- отдельный файл/временная hard-coded схема создали бы второй источник истины;
- SQLite уже является durable configuration storage приложения.

Новая БД остаётся пустой: sample-мнемосхема автоматически не создаётся.

---

## D-037 — Первый mimic renderer использует SVG в Blazor WebAssembly

**Status:** Accepted

Runtime elements рендерятся через SVG `viewBox`.

Причина:

- S11 нужны absolute coordinates;
- Text/Rectangle/Indicator естественно выражаются SVG primitives;
- canvas масштабируется без собственной JavaScript rendering loop;
- Blazor event handling достаточно для простого Button;
- coordinate model можно повторно использовать в S12 editor.

JavaScript canvas/WebGL не вводится до появления реальной необходимости.

---

## D-038 — Mimic binding хранит только TagId и использует существующий RuntimeStateClient

**Status:** Accepted

`Value`, `Indicator`, `Button` связываются с runtime только через logical `TagId`.

Mimic definition не содержит:

```text
Modbus address
UnitId
SNMP OID
Community
protocol type
```

Realtime отдельного hub/service не создаёт.

Mimic page использует существующий `RuntimeStateClient`, который уже объединяет REST snapshot и SignalR `TagChanged`.

---

## D-039 — Mimic Button использует существующий tag write path

**Status:** Accepted

Первый Button хранит:

```text
TagId
CommandValue UInt16
Text
```

Кнопка доступна только при `TagValueDto.Writable == true`.

Command выполняется через:

```text
RuntimeStateClient.WriteTagAsync
        ↓
POST /api/tags/{tagId}/write
        ↓
existing write routing
```

Отдельный command bus или protocol-specific command в mimic definition не создаётся.

SNMP-bound Button read-only/disabled в текущем scope.

---

## D-040 — Mimic Editor использует client-side draft и explicit Save

**Status:** Accepted

S12 повторяет уже проверенный Device Editor interaction model:

```text
server MimicDefinitionDto
        ↓
client-side draft
        ↓
local property changes
        ↓
explicit Save
        ↓
PUT whole definition
```

Причина:

- координаты и свойства меняются часто во время редактирования;
- auto-save на каждый input создавал бы лишние HTTP writes;
- пользователь должен явно видеть границу сохранённой и несохранённой схемы;
- S11 persistence уже атомарно сохраняет whole definition.

При смене схемы или refresh dirty draft требует подтверждения потери изменений.

---

## D-041 — Минимальный S12 редактирует position/size через properties panel без drag-and-drop

**Status:** Accepted

Выбор элемента выполняется кликом на SVG canvas.

Position и size редактируются численно:

```text
X
Y
Width
Height
```

в правой properties panel.

Причина:

- это полностью покрывает текущий S12 scope;
- сохраняется одна coordinate model с S11 SVG runtime;
- не требуется JavaScript pointer/drag layer;
- drag handles, snapping, zoom/pan и multi-select можно добавлять только при подтверждённой необходимости.

Отсутствие drag-and-drop не меняет persistent/runtime contracts.
