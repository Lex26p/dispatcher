# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA до этого пакета:** `1cf035d6fb9dfdde2a94ecc54acdfa106962a3b3`.

## 1. Завершённый этап

Общая горизонтальная продуктовая концепция завершена.

- реестр: `PRD-Q001–PRD-Q803`;
- три независимые сквозные проверки завершены и их принятые результаты встроены в канон;
- application scope зафиксирован от одного Compact-контроллера Residential & Small-Site Automation до крупной распределённой Full/Edge SCADA;
- рабочее дерево очищено от промежуточных audit/notes документов; история остаётся в Git.

## 2. Текущий этап

Начат этап **Functional Specification** — слой требований между Product Concept и System Architecture.

Он должен определить наблюдаемое поведение продукта, пользовательские flows, рабочие области, функциональные lifecycle, validations, errors, permissions и Full/Compact/Edge semantics достаточно подробно для последующего проектирования UX/API/архитектуры.

Правила этапа: `../functional/README.md`.

## 3. Текущая дорожная карта

Временный рабочий навигатор: `../functional/ROADMAP.md`.

Ближайшая последовательность:

1. Functional foundation — структура, сценарии, правила.
2. Engineering / Configuration (`ENG-*`).
3. Operations / Dispatcher Workspace (`OPS-*`).
4. Web Platform (`WEB-*`).
5. Architecture Readiness Review #1.

Это порядок проработки требований, а не release/MVP roadmap.

## 4. Первый architecture-readiness gate

После ENG + OPS + WEB должен быть определён центральный сквозной контур:

> инженер создаёт/изменяет конфигурацию → Validate/Impact/Publish → Deploy/Activate на runtime/Edge → диспетчер видит live state/alarm → выполняет semantic command → получает result/uncertain state → audit/history сохраняют факты.

Если этот контур не требует архитектурных догадок о пользовательском смысле действий, можно переходить к первой системной архитектуре, не дожидаясь полной детализации всех специализированных сервисов.

## 5. Активные functional-файлы

- `docs/functional/README.md` — правила слоя.
- `docs/functional/ROADMAP.md` — временная дорожная карта.
- `docs/functional/REFERENCE_SCENARIOS.md` — сквозные сценарии.
- `docs/functional/engineering/ENGINEERING_CONFIGURATION.md` — первый активный functional spec.
- `docs/functional/operations/OPERATIONS.md` — подготовленная граница второй спецификации.
- `docs/functional/web-platform/WEB_PLATFORM.md` — подготовленная граница Web Platform specification.

## 6. Точка продолжения

После фиксации этого пакета начать `ENG-Q001` и первый раунд Engineering UX:

- верхнеуровневые рабочие области Engineering;
- entry point;
- location/context;
- registry vs editor;
- текущий change set;
- переходы object ↔ connection ↔ type/profile/template;
- Compact simple setup vs professional Engineering.

## 7. Пока не фиксируем автоматически

До Architecture Readiness Review #1 не переходить по инерции к:

- выбору frontend/backend frameworks;
- схеме БД;
- внутренним API/transport protocols;
- message broker;
- fencing/lease/consensus implementation;
- исходному коду.

Архитектурный вопрос можно отметить как dependency, но не подменять им functional requirement.
