# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA до этого пакета:** `d4f4b4ef696662c9ef5eec965b21b42e3ac0b61a`.

## 1. Завершённый этап

Общая горизонтальная Product Concept завершена.

- реестр: `PRD-Q001–PRD-Q803`;
- три независимые сквозные проверки завершены и их принятые результаты встроены в канон;
- application scope зафиксирован от одного Compact-контроллера Residential & Small-Site Automation до крупной распределённой Full/Edge SCADA;
- рабочее дерево очищено от промежуточных audit/notes документов; история остаётся в Git.

## 2. Текущий этап

Идёт **Functional Specification** — слой требований между Product Concept и System Architecture.

Functional foundation зафиксирован в Git. Активная спецификация: `docs/functional/engineering/ENGINEERING_CONFIGURATION.md`.

## 3. Engineering — текущее состояние

Приняты и подготовлены к первому смысловому checkpoint:

- `ENG-Q001–ENG-Q050` — Engineering foundation: service/workspace, change sets, autosave, revisions/checkpoints, registries/editors, collaboration, validation foundation;
- `ENG-Q051–ENG-Q110` — Objects & Structure: locations, object registry, creation, identity, typed relations, functional/physical split, observed objects, Duplicate/Create Copies, move/delete;
- `ENG-FR001–ENG-FR025` — нормативные functional requirements, включая автоматически зафиксированные очевидные решения.

Первый Engineering checkpoint: `ENG-CP01`.

## 4. Правило Git-checkpoints

Контрольные точки больше не отслеживаются вручную по сообщениям.

`docs/functional/ROADMAP.md` содержит постоянный checkpoint protocol:

- checkpoint после самостоятельного смыслового блока;
- ориентир `100–200` новых Q между точками;
- связный блок не режется искусственно ради номера;
- перед каждым checkpoint обязательны Q/FR transfer, Decision Register, roadmap update, PROJECT_STATE update и coverage-check;
- диапазоны следующих Q заранее не фиксируются.

## 5. Следующая точка продолжения

Начать `ENG-Q111...`:

> **Types, Device Profiles, Object Templates, inheritance/composition и managed update semantics.**

После завершения этого смыслового блока применяется `ENG-CP02` согласно roadmap; отдельное напоминание пользователя о checkpoint не требуется.

## 6. Ближайшая последовательность функционального этапа

1. Завершить Engineering / Configuration (`ENG-*`).
2. Operations / Dispatcher Workspace (`OPS-*`).
3. Web Platform (`WEB-*`).
4. Architecture Readiness Review #1.

Это порядок проработки требований, а не release/MVP roadmap.

## 7. Первый architecture-readiness gate

После ENG + OPS + WEB должен быть определён центральный сквозной контур:

> инженер создаёт/изменяет конфигурацию → Validate/Impact/Publish → Deploy/Activate на runtime/Edge → диспетчер видит live state/alarm → выполняет semantic command → получает result/uncertain state → audit/history сохраняют факты.

Если этот контур не требует архитектурных догадок о пользовательском смысле действий, можно переходить к первой системной архитектуре, не дожидаясь полной детализации всех специализированных сервисов.

## 8. Пока не фиксируем автоматически

До Architecture Readiness Review #1 не переходить по инерции к:

- выбору frontend/backend frameworks;
- схеме БД;
- внутренним API/transport protocols;
- message broker;
- fencing/lease/consensus implementation;
- исходному коду.

Архитектурный вопрос можно отметить как dependency, но не подменять им functional requirement.
