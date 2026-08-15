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

V2-S01…V2-S03 завершают storage/ingestion/policy/retention и read API foundation Historian.

Следующий шаг новой дорожной карты:

```text
V2-S04 — Historian Web / Trends
```

Подробный порядок и зависимости определены в `docs/ROADMAP_V2.md`.
