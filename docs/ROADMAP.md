# Дорожная карта Dispatcher

Дорожная карта изменяется по мере развития проекта. Количество шагов не является жёстким: шаг можно разделить или объединить, если это уменьшает риск и сохраняет проверяемый результат.

## Обозначения

- `[x]` — содержимое шага подготовлено/реализовано в репозитории.
- `[ ]` — шаг ещё не реализован.
- Финальное принятие каждого технического шага подтверждается пользователем после локальной проверки и отправки нового Git SHA.
- Ошибка проверки означает продолжение текущего шага, а не переход к следующему.

## Phase 0 — Основа проекта

- [x] **S00 — Документация проекта**

## Phase 1 — Первый вертикальный срез: Modbus → Web

- [x] **S01 — Минимальный .NET solution**
- [x] **S02 — Core и TagService**
- [x] **S03 — Минимальный Modbus TCP read**
- [x] **S04 — Polling и состояние устройства**
- [x] **S05 — ASP.NET Core API**
- [x] **S06 — Blazor WebAssembly read UI**
- [x] **S07A — Hosted Modbus polling**
- [x] **S07B — Write path Web → Modbus**

## Phase 2 — Редактор устройств

- [x] **S08 — Постоянная конфигурация**
- [x] **S09A — Configuration CRUD API и live apply**
- [x] **S09B — Blazor Device Editor**

## Phase 3 — SNMP

- [x] **S10A — SNMP runtime и persistent configuration**
- [x] **S10B — SNMP configuration API и Device Editor**

### Результат Phase 3

Modbus TCP и SNMP v2c одновременно опрашиваются, полностью настраиваются через общий Device Editor и публикуют данные через единую runtime-модель тегов.

## Phase 4 — Простая мнемосхема

- [x] **S11 — Runtime мнемосхемы**
  - persistent mimic definitions;
  - SQLite schema v3 и migration v2 → v3;
  - `Text`;
  - `Rectangle`;
  - `Value`;
  - `Indicator`;
  - `Button`;
  - binding только по `TagId`;
  - SVG renderer в Blazor;
  - realtime через существующий `RuntimeStateClient` / SignalR;
  - простая команда UInt16 через существующий write path;
  - runtime API `GET /api/mimics...`;
  - минимальный configuration PUT/DELETE как foundation для S12;
  - без скрытой sample-мнемосхемы.

- [x] **S12 — Минимальный редактор мнемосхемы**
  - создание/удаление схем;
  - добавление/удаление `Text`, `Rectangle`, `Value`, `Indicator`, `Button`;
  - выбор элемента кликом на SVG canvas;
  - позиция и размер через properties panel;
  - свойства схемы и выбранного элемента справа;
  - смена типа элемента;
  - `TagId` picker из Modbus/SNMP configuration;
  - настройка Button command value;
  - client-side draft + explicit Save;
  - сохранение через существующий S11 full-definition PUT;
  - переход Runtime ↔ Editor;
  - без отдельного drag-and-drop/JS слоя.

### Результат Phase 4

Инженер создаёт простую мнемосхему, связывает элементы с logical tags, сохраняет её и использует тот же definition в операторском realtime runtime.

## Phase 5 — Roadmap v2

Базовый цикл S00–S12 завершён.

Дальнейшее развитие вынесено в отдельный документ:

```text
docs/ROADMAP_V2.md
```

Roadmap v2 включает:

```text
Historian
Events
Users / Roles / Audit
Alarms
Templates
Scripting
```

V2-S01…V2-S04 завершают первый полный Historian vertical slice.

V2-S05 и V2-S06 завершают Phase 6: immutable Event Journal, Events REST/paging, realtime новых events и Web Events.

Phase 7 authentication и permissions vertical slices завершены до management/audit:

```text
V2-S07A — Local users storage, password hashing и bootstrap foundation
V2-S07B — Server authentication session, login/logout/current user
V2-S07C — Web authentication integration
V2-S08A — Permission/role configuration foundation
V2-S08B — Server permission enforcement
V2-S08C — Web permission visibility/enabled state
```

V2-S08 разбит на проверяемые подшаги:

```text
[x] V2-S08A — durable roles/permissions + effective-permission catalog
[x] V2-S08B — Server permission enforcement
[x] V2-S08C — Web permission visibility/enabled state
```

V2-S08 завершён полным permission vertical slice: durable configuration → Server enforcement → Web projection.

V2-S09 разбит на проверяемые подшаги:

```text
[x] V2-S09A — Users/Roles management API foundation
[x] V2-S09B — Users/Roles Web admin service
[x] V2-S09C — Actor-aware security audit wiring
```

V2-S09 завершён: Users/Roles management API, Web administration и actor-aware audit собраны в один permission-based vertical slice.

V2-S10 разбит на проверяемые подшаги:

```text
[x] V2-S10A — Alarm definitions + Server configuration foundation
[x] V2-S10B — Alarm Editor Web
```

V2-S10 завершён: durable alarm definitions, permission-protected CRUD и permission-aware Web editor собраны в один configuration vertical slice.

V2-S11 и V2-S12 завершают Alarm runtime/operator vertical slice:

```text
[x] V2-S11 — Alarm runtime state machine
[x] V2-S12 — Alarm ACK, realtime и Web
```

Реализованы four-state lifecycle, High/Low hysteresis, continuous raise delay, durable transition events, permission-protected actor-aware ACK, Alarm SignalR projection и operator Web current/history.

Phase 8 завершена. V2-S13 разбит на проверяемые подшаги:

```text
[x] V2-S13A — Mimic template Server/storage/API foundation
[x] V2-S13B — Mimic Editor template integration
```

V2-S13 завершён: S13A добавил durable concrete Mimic templates, TagId parameters, permission-protected CRUD и instantiate-by-copy; S13B добавил permission-aware Web management и placement в Mimic Editor.

V2-S14 разделён на два проверяемых подшага:

```text
[x] V2-S14A — Device/Tag template Server/storage/API + общий Template Catalog
[ ] V2-S14B — Device Editor template integration
```

V2-S14A добавляет configuration schema v9, миграцию existing Mimic template metadata в общий `TemplateId/Name/Kind/Version/Parameters` catalog, concrete Modbus/SNMP device template payloads, versioning, permission-protected CRUD и instantiate через existing atomic device live-apply boundary. Web layout Device Editor в S14A не меняется.

Следующий шаг после принятия V2-S14A:

```text
V2-S14B — Device Editor template integration
```

Подробный порядок и зависимости определены в `docs/ROADMAP_V2.md`.
