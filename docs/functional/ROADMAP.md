# Дорожная карта функциональной спецификации

**Тип документа:** временный рабочий навигатор.  
**Не является:** Product Concept, release roadmap или календарным планом реализации.  
**Удаляется:** после завершения functional stage и переноса итогового состояния в канонические документы следующего этапа.

## 1. Статусы

- `NOT STARTED` — область ещё не разбиралась подробно.
- `IN PROGRESS` — идёт функциональная проработка.
- `REVIEW` — основная спецификация собрана и проходит сквозную проверку.
- `DONE` — функциональная граница зафиксирована достаточно для следующего этапа.
- `BLOCKED` — продолжение требует решения зависимости.

## 2. Текущее положение

| Этап | Статус | Результат |
|---|---|---|
| Общая продуктовая концепция | `DONE` | `PRD-Q001–PRD-Q803` |
| Functional foundation | `DONE` | структура functional docs, roadmap, reference scenarios |
| Engineering / Configuration | `IN PROGRESS` | `ENG-Q001–ENG-Q790`, `ENG-FR001–ENG-FR150`; `ENG-CP03` подготовлен |
| Operations / Dispatcher Workspace | `NOT STARTED` | вторая центральная functional specification |
| Web Platform | `NOT STARTED` | общий поведенческий контракт Web UI |
| Architecture Readiness Review #1 | `NOT STARTED` | проверка центрального сквозного контура |

## 3. FS-10 — Engineering / Configuration

**Статус:** `IN PROGRESS`

### Сделано

1. Engineering workspace и service navigation foundation.
2. Change sets, autosave/revisions/checkpoints/compare.
3. Registry/tree/inspector/editor behaviour.
4. Collaborative editing, conflicts и validation foundation.
5. Locations, Object Registry, creation, stable identity.
6. Typed relationships foundation.
7. Functional position / physical unit foundation.
8. Observed-object Engineering semantics.
9. Duplicate / Create Copies / deletion foundation.
10. Object Type semantic contract, inheritance/composition/capabilities/versioning.
11. Device Profile semantics, mapping, compatibility, local profile workflow.
12. Object Template semantics, typed inputs, nested templates, linked instances.
13. Template update/propagation, selective rollout, local overrides, conflicts.
14. Detach/adopt/reattach, template deletion resolution.
15. Device Profile replacement и Object Type migration с traceability.
16. Connections / Adapters / Endpoints / Credentials и typed connection schemas.
17. Connection diagnostics, durable/protective disable, desired-vs-actual execution placement и authority handover.
18. Полный Parameter foundation: identity/types/quantities/units/source bindings/acquisition.
19. Quality / time quality / freshness / provenance / normalization / calibration / limits.
20. Historization / late data / gaps / deadbands / retention / semantic history versions.
21. Multiple sources / source selection / manual substitution / calculated/aggregate/counter semantics.
22. Bulk Parameter Engineering, observed Parameters, commissioning/diagnostics, rights/scale/validation/impact.

**Принято:** `ENG-Q001–ENG-Q790`, `ENG-FR001–ENG-FR150`.

### Следующий блок

`ENG-Q791...` — **Semantic Commands**: definitions, arguments, risk, safety/preconditions, execution lifecycle, feedback/results, uncertainty, concurrency, Edge/offline и diagnostics.

После него продолжаются Discovery/Import details, full Validation/Impact, Approval/Publish, Deploy/Activate/Edge, versions/recovery, Engineering diagnostics/permissions и Compact setup.

### Remaining крупными блоками

- Semantic Commands;
- углубление Relationships там, где потребуется редактор/scale behaviour;
- Discovery proposal / Observed / Promotion details;
- Import UX/schemas на уровне Engineering;
- Validation / Impact — полная модель;
- Approval / Publish;
- Deploy / Activate / Edge desired-vs-active;
- Versions / corrective publication / recovery divergence;
- Engineering diagnostics / permissions;
- Compact/simple setup;
- полный reference-scenario coverage review.

**Критерий DONE:** инженер может пройти сквозной путь от пустой установки/готового template до работающего объекта без неизвестных функциональных переходов.

---

## 4. Ближайшая последовательность functional stage

1. `FS-10` Engineering / Configuration.
2. `FS-20` Operations / Dispatcher Workspace.
3. `FS-30` Web Platform.
4. `GATE-A` Architecture Readiness Review #1.
5. System Architecture при PASS, без ожидания детализации каждого специализированного сервиса.
6. Остальные functional subject specs продолжаются по dependencies на устойчивом foundation.

Это порядок **проработки требований**, а не очередность зрелости функций и не MVP roadmap.

## 5. Checkpoint protocol — постоянные точки фиксации в Git

Этот протокол действует автоматически: пользователю не нужно считать вопросы или напоминать о checkpoint.

### CP-1. Когда checkpoint обязателен

Checkpoint создаётся при выполнении любого условия:

1. завершён самостоятельный смысловой блок;
2. с предыдущего checkpoint принято ориентировочно `100–200` новых `*-Q...` решений;
3. связный блок превысил ориентир и его нельзя разумно разрезать — checkpoint сразу после завершения блока;
4. subject specification переходит в `REVIEW` или `DONE`;
5. принято существенное изменение Product Concept, влияющее на уже зафиксированные requirements.

Диапазон `100–200` — ориентир, а не квота; смысловая целостность важнее номера Q.

### CP-2. Что обязательно сделать перед checkpoint

- перенести все принятые `Q` в соответствующую specification;
- перенести автоматически принятые факты как `FR`/нормативные правила;
- обновить Decision Register/traceability;
- выполнить coverage-check на пропуски и дубли ID;
- убедиться, что значимые решения не остались только в чате;
- обновить `Done / Remaining / Next` в roadmap;
- обновить `docs/project/PROJECT_STATE.md`;
- проверить затронутые Markdown links/structure;
- только затем сформировать ZIP для Git.

### CP-3. Что не создаёт checkpoint само по себе

- фиксированное число сообщений;
- 10–30 новых решений внутри незавершённого смыслового блока;
- редакционная правка без нового нормативного содержания.

### CP-4. Engineering checkpoint history

| Точка | Диапазон | Содержание | Состояние |
|---|---|---|---|
| `ENG-CP01` | `Q001–Q110`, `FR001–FR025` | Engineering foundation + Objects & Structure | `COMMITTED` — `688392edb17ddce6e4d3874ff54344aacc2033b0` |
| `ENG-CP02` | `Q111–Q250`, `FR026–FR058` | Types / Profiles / Templates + lifecycle / propagation / migrations | `COMMITTED` — `fa38f437a90f98cdb4091a25187eec67f2213e6a` |
| `ENG-CP03` | `Q251–Q790`, `FR059–FR150` | Connections / execution placement + complete Parameter/value pipeline | `READY TO COMMIT` |
| `ENG-CP04+` | фактический следующий завершённый блок или threshold CP-1 | диапазон заранее не фиксируется | `PLANNED` |
| `ENG-FINAL` | полный Engineering coverage review | все remaining закрыты/явно делегированы dependent specs | `PLANNED` |

После commit `ENG-CP03` заменить его состояние на `COMMITTED` и записать фактический SHA при следующем checkpoint/update. `ENG-CP03` сознательно крупнее обычного ориентира, потому что Parameter model фиксировался одним связным semantic block.

## 6. FS-20 — Operations / Dispatcher Workspace

**Статус:** `NOT STARTED`.  
Зависит от основных сущностей и lifecycle `FS-10`. Цель — ежедневная operator workflow: live state, alarms, semantic commands, trends, incidents/My Work, operational exceptions, contextual VMS/ACS/ТОиР, Full/Edge degraded states и rights/explainability.

## 7. FS-30 — Web Platform

**Статус:** `NOT STARTED`.  
Зависит от ключевых UX-потребностей Engineering и Operations. Определяет общий shell/navigation/routing/registries/inspectors/editors/realtime/degraded/action/permission/error/fullscreen/mobile/accessibility/performance behaviour без выбора frontend framework.

## 8. GATE-A — Architecture Readiness Review #1

**Статус:** `NOT STARTED`.

PASS означает, что можно проектировать первую системную архитектуру без необходимости угадывать базовое поведение Engineering, Operations и Web. Gate проверяет центральный contour config → validate/impact/publish → deploy/activate → live state/alarm → semantic command → result → audit/history.

## 9. Следующая волна subject specifications

После/вокруг architecture foundation по dependencies: Events/Alarms/Incidents/Notifications; Historian/Trends/Reports; Automation; Dashboards; Mimics; ТОиР; VMS; СКУД; Spatial/Maps/BIM; IT/Networks/Virtualization; предметные domain checks HVAC/Energy/Fire/Water/Residential и другие.

## 10. Правила ведения roadmap

- roadmap фиксирует порядок проработки, а не очередность функций зрелого продукта;
- не добавлять сроки/оценки без отдельного resource/implementation planning;
- не дублировать в roadmap сами functional requirements;
- после каждого checkpoint обновлять только фактические Done/Remaining/Next и checkpoint history;
- если тема становится частью другой specification, объединить её и удалить лишнее;
- после завершения functional stage удалить ROADMAP.md; Git сохранит историю.
