# Контекст проекта для продолжения в новом чате

## Назначение

Этот файл быстро восстанавливает контекст проекта «Диспетчер».

Репозиторий `Lex26p/dispatcher` является источником истины. Сначала следует читать `docs/README.md`, затем нужные concept-файлы и `docs/architecture/README.md`.

## Продукт

«Диспетчер» — универсальная модульная платформа диспетчеризации и управления физическими и цифровыми объектами.

Ядро небольшое и универсальное. Необязательные возможности выносятся в пакеты. Если пакет не установлен, его функций не должно быть в интерфейсе.

Система предназначена и для инженеров, и для программистов: типовые задачи предпочтительно решаются через UI, сложные — могут решаться кодом.

Dashboard — основной универсальный механизм построения Web UI.

## Согласованное ядро

- проекты;
- Dashboard;
- мнемосхемы;
- устройства;
- метрики, включая записываемые и state-метрики;
- диспетчер событий;
- пользователи и права;
- системные данные и базовое администрирование;
- менеджер пакетов;
- общий Web UI.

Автоматизация не входит в ядро и является первым планируемым официальным плагином.

## Ключевые продуктовые решения

### Проекты
Проект — точка консолидации, а не владелец данных. Проекты не вложены друг в друга. Пользователь с доступом к нескольким проектам может видеть объединённые события.

### Dashboard
Универсальная страница из контейнеров и компонентов. Может получать параметры от проекта и передавать их вложенным компонентам. Отвечает за страницы, контейнеры, модальные окна и навигацию.

### Мнемосхемы
Интерактивные SVG-изображения. Отображаются только внутри Dashboard. Не являются отдельными Web-страницами и не содержат другие мнемосхемы внутри себя.

### Устройства и метрики
Устройство может быть виртуальным, но это не отдельная пользовательская классификация.

Метрика — универсальная единица данных.

Отдельная универсальная сущность `Command` удалена. Управление выполняется записью в метрики, для которых разрешена запись.

У каждой рабочей метрики есть связанная state-метрика с одним текущим состоянием. Примеры: Normal, Warning, Alarm, NoData, Maintenance. Точный enum ещё не утверждён.

### Пользователи
Различаются просмотр, управление, редактирование и административные права. Опциональный режим управления защищает от случайной записи. Пользователь без права управления не может включить этот режим.

### Web UI
Компактный глобальный Header, максимум места рабочей области, левая навигация сервиса, правая панель свойств, локальный Header только при необходимости, редко используемые ссылки скрываются из постоянного интерфейса.

## Архитектура

Backend строится как набор независимых сервисов вокруг специализированных Hub-механизмов.

### Data Hub
- реализован в `CORE-001` как отдельный C++20-сервис;
- внешний контракт: gRPC + Protocol Buffers proto3, package `dispatcher.data_hub.v1`;
- принимает realtime-значения через `PublishMetric`;
- хранит последнее актуальное значение каждой метрики;
- отдаёт его через `GetCurrent`;
- поддерживает retained/live `Subscribe`;
- хранит state-метрики тем же механизмом, что и обычные метрики;
- принимает `WriteMetric` и маршрутизирует запрос к текущему внутреннему provider;
- не считает write request подтверждением фактического изменения значения;
- не знает предметную область и описательную metadata метрик.

### Device Manager
Источник истины об устройствах и описании метрик: ID, название, описание, местоположение, состав метрик, настройки и возможность записи.

Realtime-значения находятся в Data Hub.

### Диспетчер событий / аварий
Следит за настроенными метриками, проверяет условия и записывает нужный state обратно в Data Hub.

События и аварии отправляются в отдельный Event Hub.

### Event Hub
Отдельный Hub для событий и аварий. Не смешивается с Data Hub.

### Service Hub
- реализован и завершён в `CORE-002`;
- внешний transport v1: WebSocket;
- serialization: UTF-8 JSON;
- endpoint path: `/v1/ws`;
- subprotocol: `dispatcher.service-hub.v1`;
- provider регистрирует один service address на connection;
- client отправляет addressable request по `service` + `operation`;
- Hub выполняет correlation, timeout/cancel и возвращает response;
- generic payload остаётся непрозрачным JSON;
- тот же client protocol напрямую пригоден browser Web Shell;
- authentication/authorization в CORE-002 не реализуются.

Подробный контракт: `docs/architecture/service-hub-contract.md`.

### Project Manager
- реализован и завершён в `CORE-004` как отдельный C++20-сервис;
- Project v1 содержит только stable opaque `id`, `name`, `description`;
- production persistence — локальный SQLite schema v1, внутренний для Project Manager;
- внешний service address — `project-manager.v1` поверх существующего Service Hub v1;
- операции — `create-project`, `list-projects`, `get-project`, `update-project`;
- Service Hub envelope не содержит project-specific field;
- Web Shell имеет `/projects`, list/create/edit UI и общий project context с явным global mode;
- project context сохраняется только в browser `sessionStorage`, проверяется через `get-project` и не является user preference до CORE-005;
- реальная browser integration подтверждает durable restart/re-registration path.

Подробный контракт: `docs/architecture/project-manager-contract.md`.

### Независимость
Сервис не должен знать других потребителей своих данных без необходимости. Плагины работают через общие контракты.

## Языки

Backend: C++20.

Текущий backend toolchain baseline: CMake 3.20+, Ninja, CTest, Linux/WSL.

Можно использовать готовые C++ библиотеки и создавать собственные по реальной необходимости.

Web: React + TypeScript.

Node.js — frontend toolchain, а не обязательный backend-сервис.

Межсервисные контракты должны быть независимы от C++, чтобы внешние плагины могли при необходимости использовать другие языки.

## Пока не выбрано

Для Data Hub и Service Hub transport/serialization уже выбраны. Project Manager использует локальный SQLite schema v1; это не определяет общую persistence-технологию платформы.

Пока не утверждены:

- технология Event Hub;
- общая persistence-стратегия будущих сервисов и истории;
- deployment;
- внешний Driver Runtime API и межпроцессный путь write-provider;
- механизм восстановления runtime state;
- точный state enum;
- exact authentication/session representation для CORE-005 до contract step;
- production TLS/origin policy Service Hub;
- frontend state manager и UI-библиотеки.

## Текущая точка разработки

Завершены:

- `CORE-001 — Data Hub`;
- `CORE-002 — Service Hub`;
- `CORE-003 — Web Shell`;
- `CORE-004 — Project Manager`.

Финальный documentation baseline CORE-003:

`88c5bb30f182f7d9898ad4c95b210a045060c94f`

Sprint plan CORE-004:

`d36aaa5cdcbdfc0a2d95490d08fd46ab01c1db41`

Функциональные commits CORE-004:

- Step 1 — `172e40887fde3b5b963264904e0c4fa73225a34a`;
- Step 2 — `1d90091ee7804d1bb8c49618a1780a226488c671`;
- Step 3 — `7eb7c89e3f5d9b975dc10b71f4a1bbff8e00ed29`;
- Step 4 — `7527e2758f77e40cb6b86795e2b2a21896e55224`;
- Step 5 — `1b4017526789e32e5e4ece90a63fa287cddb57c8`;
- Step 6 — `9818254650fdf26ba8a2708dacca16433989d8fe`.

Closure CORE-004:

`29b1f0ea750633cc53cc4e023585835d2b06ad8b`

CORE-004 даёт отдельный C++ Project Manager, local durable SQLite storage, versioned `project-manager.v1` Service Hub provider, Web list/editor, shared frontend project context и реальный browser/backend restart-recovery acceptance path. Проект не владеет будущими Dashboard/Device/resources; auth/ACL не имитировались.

Текущий спринт — `CORE-005 — Users & Access`.

План: `docs/development/sprints/CORE-005.md`.

Plan commit проверен:

`d05cba25981599baaeadd9ad452d1f68dbabd834`.

CORE-005 должен дать stable user identity, durable users/access configuration, local authentication/session boundary, согласованный authenticated Service Hub request path, backend-authoritative Project Manager access, Web login/current-user/access administration и control-mode baseline. Первый sprint реально применяет global + project scope; Device/Dashboard-specific ACL, external IdP/MFA и Event Hub audit publication не имитируются.

Exact credential hashing dependency, session/token representation и Service Hub auth representation выбираются только на соответствующих шагах плана.

`CORE-005 / Step 1` завершён commit:

`e8b42e69fecf7079f7b18f5f86fe334308d2579c`

Step 1 зафиксировал stable user ID, `login`/`display_name`/`enabled`, independent capabilities `view/control/edit/admin`, named permission sets, global/project assignments и effective permissions как union matching assignments. Disabled user fail-closed; explicit deny/groups/ABAC не добавлены.

`CORE-005 / Step 2` завершён commit:

`3b140d808638bb0c14a70b2aa1df96eb377af197`

Step 2 зафиксировал локальный SQLite schema v1 только для Users & Access и OpenSSL scrypt password verifier (`N=2^17`, `r=8`, `p=1`) без plaintext storage. Explicit first-admin bootstrap читает password/confirmation через stdin и атомарно создаёт user + full explicit permission set + global assignment + credential verifier + audit record.

Текущий шаг — `CORE-005 / Step 3 — Authentication/session contract`. Service address фиксируется как `users-access.v1`; session — opaque 256-bit bearer token с server-side state, 30-minute idle timeout и 12-hour absolute lifetime. SQLite schema v2 хранит только SHA-256 token digest и durable session metadata. Step 3 публикует language-independent contract/schema и реализует session engine/tests, но ещё не меняет Service Hub envelope и не добавляет Web login; это Step 4+.

## Рабочий процесс

Изменения ChatGPT отдаёт архивом с готовыми файлами проекта. Пользователь распаковывает его поверх `C:\Projects\dispatcher`, выполняет необходимые проверки, коммитит и отправляет SHA. Новый SHA становится базовой точкой истины.

Web frontend проверяется нативно в Windows/PowerShell через `npm.cmd` / `npx.cmd` на зафиксированном Node.js/npm toolchain. C++ backend собирается и тестируется в Linux/WSL.

В конце каждого спринта обязательна целевая проверка актуальности связанной документации. Устаревшие статусы, архитектурные решения, команды, roadmap/current point и README должны быть синхронизированы до перехода к следующему спринту.

В пользовательском Git-workflow не используются команды `git diff`.

Репозиторий: `https://github.com/Lex26p/dispatcher`.
