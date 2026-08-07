# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA перед этим пакетом:** `688392edb17ddce6e4d3874ff54344aacc2033b0`.

## 1. Завершённый Product Concept

Общая горизонтальная Product Concept завершена и остаётся фундаментом требований:

- `PRD-Q001–PRD-Q803`;
- Full / Compact / Edge;
- общая object/configuration/command/alarm/history/package/security модель;
- application scope от одного Compact-контроллера до крупной распределённой Full/Edge SCADA.

Product Concept не расширяется по инерции. Возврат туда происходит только при обнаружении нового фундаментального product-level решения или противоречия.

## 2. Текущий этап — Functional Specification

Functional Specification определяет наблюдаемое поведение продукта, user flows, lifecycle, validations, permissions и Full/Compact/Edge semantics достаточно подробно для последующего UX/API/System Architecture design.

Рабочий навигатор: `../functional/ROADMAP.md`.

## 3. Engineering / Configuration — текущее состояние

**Статус:** `IN PROGRESS`.

Принято и подготовлено к `ENG-CP02`:

- `ENG-Q001–ENG-Q250` без намеренных пропусков;
- `ENG-FR001–ENG-FR058`;
- Engineering workspace/change-set/object foundation;
- Types / Device Profiles / Object Templates;
- type/profile/template versioning и compatibility;
- linked template instances, local overrides, selective propagation;
- nested template dependency semantics;
- detach/adopt/reattach;
- profile replacement и type migration with provenance.

`ENG-CP01` уже соответствует commit `688392edb17ddce6e4d3874ff54344aacc2033b0`.

`ENG-CP02` добавляет `ENG-Q111–ENG-Q250` и `ENG-FR026–ENG-FR058` и ожидает commit/push.

## 4. Следующая точка продолжения

После подтверждения SHA `ENG-CP02` продолжить с:

> `ENG-Q251...` — Connections / Adapters / Endpoints / Credentials / execution placement.

Затем перейти к Parameters и Semantic Commands.

## 5. Checkpoint discipline

Пользователю не требуется вручную отслеживать момент фиксации. Правила находятся в `../functional/ROADMAP.md`:

- смысловой блок имеет приоритет над фиксированным количеством вопросов;
- ориентир — `100–200` новых Q между checkpoints;
- перед ZIP обязательны перенос всех Q/FR, coverage-check, roadmap и PROJECT_STATE update;
- диапазон следующего checkpoint заранее не фиксируется.

## 6. Architecture readiness

До `GATE-A` не выбирать по инерции frontend/backend frameworks, конкретную DB, message broker, internal transport, fencing/lease/consensus implementation или исходный код.

`GATE-A` выполняется после достаточной детализации Engineering + Operations + Web Platform и проверяет центральный сквозной contour от configuration до runtime/command/audit.
