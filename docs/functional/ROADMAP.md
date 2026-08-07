# Дорожная карта функциональной спецификации

**Тип документа:** временный рабочий навигатор.  
**Не является:** продуктовой концепцией, release roadmap или календарным планом разработки.  
**Удаляется:** после завершения функционального этапа и переноса итогового состояния в канонические документы следующего этапа.

## 1. Статусы

- `NOT STARTED` — область ещё не разбиралась подробно.
- `IN PROGRESS` — идёт функциональная проработка.
- `REVIEW` — основная спецификация собрана и проверяется на сквозную согласованность.
- `DONE` — функциональная граница области зафиксирована достаточно для следующего этапа.
- `BLOCKED` — продолжение невозможно без решения зависимости.

## 2. Текущее положение

| Этап | Статус | Результат |
|---|---|---|
| Общая продуктовая концепция | `DONE` | `PRD-Q001–PRD-Q803`, три независимые проверки, application scope |
| Functional foundation | `DONE` | структура functional docs, roadmap, reference scenarios зафиксированы в Git |
| Engineering / Configuration | `IN PROGRESS` | приняты `ENG-Q001–ENG-Q110`, `ENG-FR001–ENG-FR025`; первый смысловой checkpoint |
| Operations / Dispatcher Workspace | `NOT STARTED` | вторая центральная функциональная спецификация |
| Web Platform | `NOT STARTED` | общий поведенческий контракт Web UI |
| Architecture Readiness Review #1 | `NOT STARTED` | проверка готовности центрального сквозного контура к архитектуре |

## 3. Ближайший маршрут

### FS-00 — Functional foundation

**Статус:** `DONE`

**Сделано:**

- отделён слой functional specification от Product Concept;
- определена система идентификаторов;
- создан временный roadmap;
- создан набор reference scenarios;
- созданы каркасы Engineering, Operations и Web Platform;
- foundation зафиксирован в Git.

---

### FS-10 — Engineering / Configuration

**Статус:** `IN PROGRESS`

**Цель:** полностью описать пользовательский и функциональный путь от создания/обнаружения сущности до публикации и фактической активации конфигурации.

**Сделано:**

1. Engineering service/workspace foundation.
2. Рабочий контекст и change sets.
3. Autosave/revisions/checkpoints/compare.
4. Registry / tree / inspector / editor behaviour.
5. Collaborative editing и conflict semantics.
6. Validation foundation.
7. Locations и Object Registry foundation.
8. Object creation и stable identity semantics.
9. Typed relationships foundation.
10. Functional position / physical unit foundation.
11. Observed-object Engineering semantics.
12. Duplicate / Create Copies foundation.
13. Move/delete и historical identity continuity.

**Принятые диапазоны:** `ENG-Q001–ENG-Q110`, `ENG-FR001–ENG-FR025`.

**Следующий блок:** `ENG-Q111...` — Types, Device Profiles, Object Templates, inheritance/composition и managed update semantics.

**Осталось крупными блоками:**

- Types / Profiles / Templates;
- Connections / Adapters / Endpoints;
- Parameters;
- Semantic Commands;
- углубление Relationships;
- Discovery proposal / Observed / Promotion;
- Import;
- Draft/Validation/Impact — детальная завершённая модель;
- Approval / Publish;
- Deploy / Activate / Edge desired-vs-active;
- Versions / corrective publication / recovery divergence;
- Engineering diagnostics / permissions;
- Compact/simple setup;
- полный coverage review по reference scenarios.

**Reference scenarios:** `RS-001`, `RS-002`, `RS-003`, `RS-004`, `RS-005`, `RS-014`, `RS-015`, `RS-019`.

**Критерий DONE:** инженер может пройти сквозной путь от пустой установки/готового template до работающего объекта без неизвестных функциональных переходов.

**Открывает:** `FS-20`, существенную часть `FS-30`, подготовку API/domain contracts.

---

### FS-20 — Operations / Dispatcher Workspace

**Статус:** `NOT STARTED`

**Зависит от:** основных сущностей и переходов `FS-10`.

**Цель:** описать ежедневную работу диспетчера с работающей системой.

**Основные блоки:**

1. Стартовый workspace и контекст площадки.
2. Object context и effective operational state.
3. Live parameters, quality, freshness и connection state.
4. Alarms и быстрый переход к причине.
5. Semantic commands, confirmation, uncertainty и results.
6. Trends/history в оперативном контексте.
7. Incidents, My Work, tasks и handover.
8. Manual control, substitution, suppression и maintenance context.
9. VMS/ACS/ТОиР contextual transitions.
10. Full/Edge degraded/offline states.
11. Operator permissions и explainability.

**Reference scenarios:** `RS-006`, `RS-007`, `RS-008`, `RS-009`, `RS-010`, `RS-011`, `RS-012`.

**Критерий DONE:** диспетчер однозначно понимает текущее состояние объекта, причину проблемы, доступные действия и результат своих действий.

**Открывает:** полноценный `FS-30`, дальнейшие ALM/HIS/MAINT/VMS specifications.

---

### FS-30 — Web Platform

**Статус:** `NOT STARTED`

**Зависит от:** ключевых UX-потребностей `FS-10` и `FS-20`.

**Цель:** определить единый поведенческий контракт web shell и общих UI primitives без выбора frontend technology.

**Основные блоки:**

1. Global shell.
2. Service navigation.
3. Routing, stable URLs и context preservation.
4. Registries/tables/filter/search patterns.
5. Inspector.
6. Editor layout framework.
7. Realtime subscriptions и stale/degraded representation.
8. Actions, rights, disabled/hidden states и explanations.
9. Errors, background operations и progress.
10. Notifications/personal area.
11. Fullscreen, kiosk, wallboard и responsive/mobile principles.
12. Accessibility и density.
13. Performance behaviour: virtualization, pagination, incremental loading.

**Reference scenarios:** все основные `RS-*`, особенно `RS-001`, `RS-006`, `RS-013`.

**Критерий DONE:** Engineering и Operations могут быть реализованы поверх одной согласованной Web Platform без дублирующих shell/navigation/editor patterns.

**Открывает:** Architecture Readiness Review #1.

---

### GATE-A — Architecture Readiness Review #1

**Статус:** `NOT STARTED`

**Не является:** полным завершением всего функционального ТЗ.

**Проверяем:**

- сущности и lifecycle Engineering;
- основные operator flows;
- общий Web behaviour;
- reference contour от config до runtime/command/audit;
- отсутствие противоречий с `PRD-Q001–Q803`;
- достаточность функциональных контрактов для проектирования API/application boundaries.

**Критерий PASS:** можно проектировать первую системную архитектуру без необходимости угадывать базовое поведение Engineering, Operations и Web.

---

## 4. Checkpoint protocol — постоянные точки фиксации в Git

Этот раздел нужен специально, чтобы не отслеживать вручную объём переписки и не решать каждый раз заново, пора ли делать Git-checkpoint.

### CP-1. Когда checkpoint обязателен

Checkpoint создаётся при выполнении **любого** из условий:

1. завершён самостоятельный смысловой блок функциональной спецификации;
2. с предыдущего checkpoint принято ориентировочно `100–200` новых `*-Q...` решений;
3. связный блок превысил ориентир, но его нельзя разумно разрезать — checkpoint делается сразу после завершения блока, даже если диапазон получился больше 200;
4. спецификация переходит в `REVIEW` или `DONE`;
5. обнаружено и принято существенное изменение Product Concept, влияющее на уже зафиксированные требования.

Число 100–200 — **ориентир, не квота**. Смысловая целостность важнее номера вопроса.

### CP-2. Что обязательно входит в checkpoint

Перед формированием пакета необходимо:

- перенести в subject specification все принятые `Q` решения с точными ID;
- перенести все автоматически принятые очевидные решения как `FR`/нормативные правила;
- обновить Decision Register / traceability диапазон;
- обновить `Done / Remaining / Next` в этом roadmap;
- обновить `docs/project/PROJECT_STATE.md`;
- выполнить coverage-check на пропуски номеров, дубли и решения, оставшиеся только в чате;
- проверить локальные Markdown-ссылки затронутых файлов;
- только после этого формировать ZIP для Git.

### CP-3. Что не является поводом для отдельного checkpoint

Не создавать Git-пакет только потому, что:

- прошло фиксированное число сообщений;
- принято 10–30 очевидных решений внутри незавершённого блока;
- изменилось только рабочее пояснение без нового нормативного решения.

### CP-4. Planned checkpoints Engineering

| Точка | Условие | Состояние |
|---|---|---|
| `ENG-CP01` | Foundation + Objects & Structure, `ENG-Q001–ENG-Q110` | `CURRENT / READY TO COMMIT` |
| `ENG-CP02` | Завершён следующий крупный смысловой блок (сейчас Types / Profiles / Templates); целевой ориентир 100–200 новых Q с предыдущей точки | `PLANNED` |
| `ENG-CP03+` | Каждый следующий завершённый блок или достижение checkpoint threshold по CP-1 | `PLANNED` |
| `ENG-FINAL` | Полный Engineering coverage review, все Remaining закрыты или явно вынесены в зависимые specs, статус `REVIEW/DONE` | `PLANNED` |

После каждого checkpoint фактический диапазон Q записывается в таблицу; следующий диапазон заранее не фиксируется.

## 5. Следующая волна функциональных спецификаций

Эти области не должны блокировать `GATE-A`, если их горизонтальный смысл уже достаточен для центрального контура. Детальная проработка идёт после/параллельно архитектурному foundation согласно зависимостям.

| Порядок | Область | Статус | Основная зависимость |
|---:|---|---|---|
| 1 | Events / Alarms / Incidents / Notifications | `NOT STARTED` | OPS + Web |
| 2 | Historian / Trends / Reports | `NOT STARTED` | OPS + Engineering parameters |
| 3 | Automation | `NOT STARTED` | Engineering + Command Model + runtime contracts |
| 4 | Dashboards | `NOT STARTED` | Web Platform + object/context contracts |
| 5 | Mimics | `NOT STARTED` | Web Platform + object/context + command contracts |
| 6 | ТОиР | `NOT STARTED` | object/physical-unit + OPS |
| 7 | VMS | `NOT STARTED` | object foundation + OPS + Web |
| 8 | СКУД | `NOT STARTED` | Person + object foundation + OPS |
| 9 | Spatial / Maps / BIM | `NOT STARTED` | object/location + Web |
| 10 | IT / Networks / Virtualization | `NOT STARTED` | observed-object + Engineering |

## 6. Предметные проверки общей модели

Эти темы не обязаны становиться самостоятельными сервисами. Их задача — проверить, что функциональные механизмы действительно работают на разных реальных доменах.

| Область | Статус | Что проверяет |
|---|---|---|
| HVAC | `NOT STARTED` | типы/профили, PID supervisory, commands, alarms, mimics, ТОиР |
| Energy | `NOT STARTED` | electrical topology, counters, interval data, quality, reports |
| Fire | `NOT STARTED` | certified local contour, alarms, plans, restricted commands |
| Water / Utilities | `NOT STARTED` | distributed Edge, pumps, pressure/flow, counters, maps |
| Lifts / Escalators | `NOT STARTED` | specialized states, diagnostics, contextual UI |
| Residential & Small-Site | `NOT STARTED` | scale-down UX, Compact, local I/O, templates, local autonomy |

## 7. Правила ведения roadmap

- Roadmap описывает **порядок проработки**, а не очередность появления функций в зрелом продукте.
- Не добавлять даты и оценки без отдельной задачи планирования ресурсов/реализации.
- После каждого принятого блока обновлять только фактический status и `Done / Remaining / Next`.
- Не дублировать сюда содержание functional requirements.
- Если тема оказалась частью другой спецификации, объединить её и удалить лишний пункт.
- Git-checkpoints выполняются по протоколу раздела 4 автоматически, без отдельного отслеживания пользователем.
- После завершения functional stage удалить этот файл; Git сохранит историю.
