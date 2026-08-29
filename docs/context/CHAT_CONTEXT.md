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

Для Data Hub и Service Hub transport/serialization уже выбраны.

Пока не утверждены:

- технология Event Hub;
- БД и постоянное хранение;
- deployment;
- внешний Driver Runtime API и межпроцессный путь write-provider;
- механизм восстановления runtime state;
- точный state enum;
- аутентификация/токены;
- production TLS/origin policy Service Hub;
- frontend state manager и UI-библиотеки.

## Текущая точка разработки

Завершены:

- `CORE-001 — Data Hub`;
- `CORE-002 — Service Hub`.

Финальный baseline CORE-002:

`7df47f234f6e0638e0f41ef81706d05244d7d2ea`

Текущий спринт — `CORE-003 — Web Shell`.

Его подробный план и критерии завершения зафиксированы в `docs/development/sprints/CORE-003.md`.

Web Shell должен дать React + TypeScript shell, глобальный Header/navigation/workspace и общую browser-side Service Hub connection. Он не реализует предметные Web-интерфейсы следующих сервисов.

Frontend state manager и UI-библиотека не выбираются заранее в плане.

Plan `CORE-003` зафиксирован commit:

`12f0fd374e515d47aa8289f476ff233cc69d201c`

`CORE-003 / Step 1` завершён commit:

`f974baa578ad310cbcd3403836d47fc2a32ec7d8`

Step 1 создал `web/` с React + TypeScript, Node 24 LTS, Vite, Vitest/Testing Library, reproducible `package-lock.json` и Playwright browser smoke.

`CORE-003 / Step 2` завершён commit:

`a8f7b91a8b0c23385ce349c55ca6e6a70e9685c8`

Step 2 создал базовый App Shell: компактный global Header, menu trigger, резерв будущих global actions и рабочую область. Generated `*.tsbuildinfo` исключены из version control.

`CORE-003 / Step 3` завершён commit:

`81264746c0948b664c30b91418b8cc477b6b2f82`

Step 3 добавил global menu и shell-level navigation: текущий `/` workspace, unknown-route fallback и keyboard-friendly open/close behavior без router dependency.

`CORE-003 / Step 4` завершён commit:

`02002f48a08cae6697b28be5e06b73864c2d9384`

Step 4 создал самостоятельный TypeScript client Service Hub v1: configurable WebSocket URL, subprotocol, connection state, request correlation, cancel и разделение Hub request errors / transport failures.

`CORE-003 / Step 5` завершён commit:

`d612dcfec40c6447c35bc59993514fbb05e20e73`

Step 5 связал client с React lifecycle через shared Provider/context, добавил общую URL-конфигурацию и ненавязчивый connection status в global Header.

`CORE-003 / Step 6` завершён commit:

`f128c55748a5e2957151ba29b0c1d872614ccadf`

Step 6 подтвердил реальный browser path через существующий C++ Service Hub и automation-only test provider: success, parallel correlation, cancel, Hub close/disconnected state и явный reconnect.

`CORE-003 — Web Shell` окончательно закрыт commit:

`88c5bb30f182f7d9898ad4c95b210a045060c94f`

Текущий спринт:

`CORE-004 — Project Manager`.

Sprint plan CORE-004 зафиксирован commit:

`d36aaa5cdcbdfc0a2d95490d08fd46ab01c1db41`

Границы CORE-004: отдельный C++ Project Manager, durable project persistence, Service Hub contract/provider, Web список/редактор и общий frontend project context. Проект остаётся плоской точкой консолидации и не владеет Dashboard/Device/другими будущими ресурсами. Users & Access/authentication остаётся CORE-005.

`CORE-004 / Step 1` завершён commit:

`172e40887fde3b5b963264904e0c4fa73225a34a`

Step 1 зафиксировал opaque stable `id`, `name`, `description`, application boundary create/list/get/update, storage port, in-memory unit tests и Linux SIGINT/SIGTERM lifecycle.

Текущий шаг — `CORE-004 / Step 2 — Durable persistence baseline`. Для Project Manager выбирается локальный SQLite adapter с внутренним schema version 1 и restart/reopen tests. SQLite является только storage detail Project Manager, не общей БД платформы. Service Hub contract/provider остаётся Step 3.

## Рабочий процесс

Изменения ChatGPT отдаёт архивом с готовыми файлами проекта. Пользователь распаковывает его поверх `C:\Projects\dispatcher`, выполняет необходимые проверки, коммитит и отправляет SHA. Новый SHA становится базовой точкой истины.

Web frontend проверяется нативно в Windows/PowerShell через `npm.cmd` / `npx.cmd` на зафиксированном Node.js/npm toolchain. C++ backend собирается и тестируется в Linux/WSL.

В конце каждого спринта обязательна целевая проверка актуальности связанной документации. Устаревшие статусы, архитектурные решения, команды, roadmap/current point и README должны быть синхронизированы до перехода к следующему спринту.

В пользовательском Git-workflow не используются команды `git diff`.

Репозиторий: `https://github.com/Lex26p/dispatcher`.
