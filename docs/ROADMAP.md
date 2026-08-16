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
[ ] V2-S09B — Users/Roles Web admin service
[ ] V2-S09C — Actor-aware security audit wiring
```

Следующий шаг после принятия V2-S09A:

```text
V2-S09B — Users/Roles Web admin service
```

Подробный порядок и зависимости определены в `docs/ROADMAP_V2.md`.
