# CORE-004 — Project Manager

## Статус

**В разработке. Step 2 завершён; текущий шаг — Step 3.**

Этап: `L1-01 — Ядро платформы`.

Базовая точка перед началом спринта:

`88c5bb30f182f7d9898ad4c95b210a045060c94f`

Sprint plan зафиксирован commit:

`d36aaa5cdcbdfc0a2d95490d08fd46ab01c1db41`

Этот файл является планом спринта и после завершения будет дополнен итоговым отчётом.

## Цель

Реализовать проекты как самостоятельные плоские точки консолидации и контекста платформы.

После спринта пользователь должен иметь возможность создать проект, увидеть список проектов, открыть проект, изменить его базовые свойства и выбрать проект как текущий контекст Web Shell.

Project Manager должен быть самостоятельным backend-сервисом и использовать уже существующий Service Hub как внешнюю request/response boundary.

## Продуктовые исходные решения

Концепция проектов уже фиксирует:

- проект является самостоятельной точкой консолидации;
- проект чаще соответствует локации/объекту, но это не обязательное правило;
- проект используется для логического объединения, навигации, представления информации и разграничения доступа;
- проект **не является владельцем** Dashboard, мнемосхем, устройств, автоматизаций и других ресурсов;
- проекты не вкладываются друг в друга;
- один Dashboard в будущем может использоваться в нескольких проектах;
- проект в будущем может передавать Dashboard параметры/контекст;
- Web UI для проектов использует общий паттерн «список + редактор»;
- project context по возможности сохраняется при переходах между сервисами, при этом должен существовать глобальный режим без выбранного проекта.

`CORE-004` не должен переопределять эти решения.

## Архитектурная граница спринта

Project Manager отвечает за:

- собственную сущность проекта;
- стабильную идентичность проекта;
- хранение и получение проектов;
- изменение базовых свойств проекта;
- durable persistence проектов;
- внешний language-independent contract через Service Hub;
- Web-интерфейс списка/редактора;
- общий frontend project context.

Project Manager **не** становится владельцем будущих ресурсов других сервисов.

Поэтому в `CORE-004` не создаются заранее:

- Dashboard records или Dashboard editor;
- Device records;
- Mimics;
- Event Manager/Event Hub integrations;
- Package Manager integrations;
- automation bindings;
- универсальный resource-membership registry;
- вложенные проекты;
- пользовательские роли/ACL;
- authentication/tokens;
- новый Service Hub envelope field `project`;
- второй backend transport.

Если будущему сервису потребуется project ID, его собственный service contract должен явно определить, где project context нужен. Service Hub остаётся нейтральным транспортом и не начинает интерпретировать project context.

## Users & Access boundary

Проект является важной будущей областью разграничения доступа, но `CORE-005 — Users & Access` ещё не реализован.

Поэтому `CORE-004`:

- не моделирует временные фиктивные роли;
- не добавляет auth fields в Project Manager contract;
- не считает отсутствие ACL окончательной моделью безопасности;
- строит Project Manager так, чтобы `CORE-005` мог добавить user/access checks поверх стабильной service boundary.

## Dashboard boundary

Концепция допускает связь проекта с Dashboard и стартовый Dashboard, но реальный Dashboard появляется только в `CORE-014`.

В `CORE-004` не создаём фиктивные Dashboard IDs, коллекции связей или стартовый Dashboard только ради будущего API.

Связь проектов с Dashboard будет добавлена тогда, когда появится реальный Dashboard contract.

## Persistence boundary

Project Manager — первый конфигурационный/metadata-сервис, для которого потеря данных после перезапуска делает результат спринта практически непригодным.

Поэтому durable persistence входит в критерий `CORE-004`.

Конкретная технология хранения **не выбирается в этом плане заранее**. Она выбирается отдельным шагом после фиксации минимальной Project model и фактических требований.

Требования к выбору:

- проекты переживают штатный restart Project Manager;
- хранилище остаётся локальным к ответственности Project Manager;
- формат/DB не должен становиться скрытым общим контрактом между сервисами;
- не создаём универсальную platform database abstraction без доказанной необходимости;
- решение должно быть простым для локальной разработки/тестов и не закрывать путь к дальнейшему развитию.

## Минимальная Project model

Точный внешний schema фиксируется на шаге контракта.

До этого план закрепляет только необходимые свойства модели:

- у проекта есть стабильный opaque identifier, не зависящий от изменяемого отображаемого имени;
- у проекта есть минимальные человекочитаемые свойства, необходимые списку и редактору;
- проект не имеет parent project;
- проект не содержит встроенные копии ресурсов будущих сервисов.

Не добавляем расширенную metadata «на будущее», пока для неё нет реального требования.

## Ожидаемый результат

К завершению `CORE-004` должен существовать Project Manager, который:

- является отдельным C++20 backend-сервисом;
- отдельно собирается и тестируется в Linux/WSL;
- имеет понятный lifecycle и clean shutdown;
- хранит проекты durable между штатными restart;
- предоставляет versioned language-independent operations через Service Hub;
- позволяет создать проект;
- позволяет получить список проектов;
- позволяет получить один проект;
- позволяет изменить базовые свойства проекта;
- возвращает предсказуемые domain/service errors;
- регистрируется как provider в существующем Service Hub;
- не меняет Service Hub v1 envelope;
- имеет Web-раздел «Проекты» в существующем Web Shell;
- имеет рабочий список проектов и редактор;
- использует общий `ServiceHubClient`, а не открывает отдельный WebSocket;
- позволяет выбрать проект как текущий frontend context;
- позволяет вернуться в глобальный контекст без проекта;
- сохраняет выбранный context при обычных переходах внутри Web Shell в рамках текущей browser session;
- не превращает проект в владельца будущих ресурсов;
- подтверждён реальным browser → Service Hub → Project Manager integration path.

Удаление проекта не является обязательным критерием roadmap для первого спринта Project Manager и не добавляется автоматически только ради формального CRUD. Если во время реализации обнаружится реальная необходимость deletion lifecycle, она фиксируется отдельным решением в рамках спринта.

## Критерий завершения

Спринт завершён, если подтверждается сценарий:

1. Project Manager отдельно конфигурируется/собирается как C++20 service target.
2. Unit/service tests проходят локально к Project Manager.
3. Service имеет Linux lifecycle и завершается предсказуемо.
4. Созданный project получает стабильный identifier.
5. Проект можно создать, получить списком, получить по ID и изменить через Project Manager application boundary.
6. Project data переживает штатный restart сервиса.
7. Внешний Project Manager contract является versioned и language-independent.
8. Реальный Project Manager provider регистрируется в Service Hub и обслуживает project operations.
9. Service Hub envelope не расширяется project-specific полями.
10. Web Shell показывает реальный раздел проектов, а не mock/static data.
11. Web использует общий Service Hub client из `CORE-003`.
12. Пользователь может создать проект и изменить его через browser UI.
13. Пользователь может выбрать существующий project как текущий frontend context и очистить context до глобального режима.
14. Project context не создаёт ownership будущих Dashboard/Device/resources.
15. После restart Project Manager browser снова получает сохранённые проекты.
16. Недоступный Project Manager/Service Hub не разрушает Web Shell; ошибка отображается как локальная проблема сервиса/connection.
17. Users/roles/authentication не имитируются до `CORE-005`.
18. Выполнен реальный browser → Service Hub → Project Manager acceptance path.
19. Проведён финальный targeted documentation audit.

# Шаги

## Step 1 — Project domain и backend service skeleton

### Что делаем

Создаём самостоятельное дерево:

`services/project-manager/`

Фиксируем минимальную Project domain model и application boundary.

Нужно определить только реально необходимые базовые поля проекта и validation rules. Identifier должен быть стабильным и не зависеть от mutable display properties.

Создаём:

- отдельный CMake target Project Manager;
- application/domain слой;
- repository/storage port;
- временную test implementation storage port для unit tests;
- unit tests create/list/get/update;
- базовый executable lifecycle.

На этом шаге не выбираем durable DB/file technology и не подключаем Service Hub.

### Результат

Project Manager существует как отдельно тестируемый backend component с минимальной domain model и чистой storage boundary.

### Реализация Step 1

Создаётся самостоятельный `services/project-manager/` без внешних runtime-зависимостей и без Service Hub integration.

Минимальная внутренняя Project model Step 1 содержит:

- стабильный opaque `id`, который генерируется независимо от изменяемого имени;
- обязательный человекочитаемый `name`;
- необязательный `description`;
- никаких parent/resource ownership полей.

Application boundary `ProjectManager` поддерживает `create`, `list`, `get` и `update`. Storage скрыт за `ProjectRepository`; production persistence adapter намеренно отсутствует до Step 2. Unit tests используют локальный in-memory repository и управляемый ID generator.

Validation Step 1:

- `name` должен содержать хотя бы один non-whitespace символ;
- `name` ограничен 256 байтами UTF-8 payload;
- `description` ограничен 4096 байтами UTF-8 payload.

Ошибки application boundary различают invalid input, missing project, storage failure и невозможность получить уникальный generated ID. Duplicate display names разрешены: имя не является identity проекта.

Executable `dispatcher-project-manager` на этом шаге реализует только самостоятельный Linux lifecycle skeleton: явно сообщает, что Service Hub provider ещё не настроен, ждёт `SIGINT`/`SIGTERM` и завершается чисто. Сетевой endpoint и provider registration появятся только в Step 3.

CTest Step 1 проверяет domain/application behavior и clean shutdown по SIGINT/SIGTERM. Durable DB/file technology в Step 1 не выбрана.

Step 1 завершён commit:

`172e40887fde3b5b963264904e0c4fa73225a34a`

## Step 2 — Durable persistence baseline

### Что делаем

На основании модели Step 1 выбираем минимальную технологию durable storage.

Решение должно быть записано в документацию вместе с причиной выбора.

Реализуем production storage adapter и тесты как минимум для:

- create/read/list/update;
- restart/reopen;
- сохранения stable IDs;
- предсказуемого поведения при отсутствующем/новом storage;
- ошибок чтения/записи, которые реально можно корректно обработать на этом этапе.

Не создаём общий DB-service или универсальную repository framework только потому, что будущим сервисам тоже понадобится persistence.

### Результат

Project records переживают restart Project Manager, а persistence остаётся внутренней ответственностью сервиса.

### Решение и реализация Step 2

Для production persistence Project Manager выбирается локальный **SQLite** adapter. Это решение относится только к Project Manager и не является выбором общей БД платформы.

Причины выбора:

- текущая Project model — небольшой локальный metadata-набор (`id`, `name`, `description`);
- SQLite embedded и transactional, поэтому не появляется отдельный database service/process;
- database file остаётся внутренней деталью Project Manager и не становится межсервисным контрактом;
- create/read/list/update и restart/reopen проверяются напрямую;
- решение не ограничивает выбор persistence для других сервисов, Event Hub или будущей истории.

Production adapter `SqliteProjectRepository` реализует существующий `ProjectRepository` port без изменения domain/application API.

Внутренний schema baseline:

- SQLite `PRAGMA user_version = 1`;
- таблица `projects`;
- `id TEXT PRIMARY KEY NOT NULL`;
- `name TEXT NOT NULL`;
- `description TEXT NOT NULL`.

Новая database создаётся и инициализируется автоматически. Database с `user_version`, который новее поддерживаемого executable, отвергается вместо неявного downgrade. Empty/unopenable storage path приводит к startup/storage error.

Executable получает optional `database-path`; default — `dispatcher-project-manager.db`. Storage открывается до входа в signal lifecycle, поэтому сервис не запускается в volatile режиме при ошибке persistence.

Persistence test проверяет create/update, primary-key conflict, missing update, reopen сохранённой database, stable project ID после reopen и ошибки инициализации storage. Signal lifecycle tests используют временную SQLite database и подтверждают её создание перед clean SIGINT/SIGTERM shutdown.

Новая WSL development dependency Step 2 — `libsqlite3-dev`. Отдельный SQLite server не требуется. Service Hub contract/provider по-прежнему остаётся Step 3.

Step 2 завершён commit:

`1d90091ee7804d1bb8c49618a1780a226488c671`

## Step 3 — Project Manager contract и Service Hub provider

### Что делаем

Фиксируем внешний versioned Project Manager contract поверх существующего Service Hub v1.

Точные service/operation names, request/response payload schemas и error codes определяются на этом шаге и документируются в репозитории.

Минимальные операции должны покрыть:

- create project;
- list projects;
- get project;
- update project.

Project-specific payload не изменяет общий Service Hub envelope.

Реализуем provider connection/lifecycle:

- подключение Project Manager к Service Hub;
- registration service route;
- обработку параллельных requests;
- correlation через существующий Hub;
- корректное отображение domain/service errors;
- provider disconnect/reconnect behavior, необходимое нормальному lifecycle.

Authentication не добавляем.

### Результат

Другой процесс или Web client может работать с Project Manager через стабильную language-independent Service Hub boundary.

### Реализация Step 3

Project Manager публикует versioned provider address `project-manager.v1` через существующий Service Hub v1 без изменения общего envelope.

Контракт фиксируется в `docs/architecture/project-manager-contract.md` и machine-readable payload definitions `services/project-manager/protocol/dispatcher/project_manager/v1/project_manager.schema.json`.

Operations v1:

- `create-project`;
- `list-projects`;
- `get-project`;
- `update-project`.

Successful payload содержит Project (`id`, `name`, `description`) либо список проектов. Provider-specific errors используют prefix `project.`; prefix `hub.` остаётся зарезервирован Service Hub.

Production adapter `ServiceHubProvider` использует Boost.Beast + `json-c`, запрашивает subprotocol `dispatcher.service-hub.v1`, регистрирует `project-manager.v1`, принимает `request`/`cancel` и отправляет обычные Service Hub `response`.

При transport disconnect Project Manager сохраняет process/SQLite state, выполняет bounded retry loop, создаёт новое WebSocket connection и повторно регистрирует provider route. SIGINT/SIGTERM останавливает retry loop через общий application lifecycle.

CTest `project-manager.service-hub-integration` запускает реальные процессы Service Hub и Project Manager, проверяет create/list/get/update, invalid payload/domain error, несколько одновременно активных requests, затем перезапускает Service Hub на том же порту и подтверждает повторную provider registration и доступ к сохранённому проекту. Production Project Manager не линкуется с внутренним C++ API Service Hub.

## Step 4 — Web Project Manager: список и редактор

### Что делаем

Добавляем в существующий Web Shell первый реальный service UI.

Глобальная навигация получает destination «Проекты», потому что функция теперь существует реально.

Web Project Manager использует shared `ServiceHubClient` из `CORE-003`.

Реализуем минимальный пользовательский сценарий:

- открыть список проектов;
- увидеть loading/empty/error states;
- создать проект;
- открыть существующий проект;
- изменить базовые свойства;
- сохранить;
- вернуться к списку.

Используем уже согласованный UI pattern «список + редактор».

Router/state/UI library не добавляется автоматически. Если текущий shell navigation перестаёт быть разумно поддерживаемым без router, необходимость оценивается на этом шаге и решение фиксируется отдельно.

### Результат

Проектами можно реально управлять из browser через Project Manager backend.

## Step 5 — Project context в Web Shell

### Что делаем

Добавляем минимальный общий frontend project context, который смогут использовать следующие service UIs.

Контекст должен:

- содержать selected project identity/минимально необходимое отображение;
- иметь явный глобальный режим `no project`;
- позволять выбрать проект из реальных данных Project Manager;
- сохраняться при обычной навигации между shell destinations в текущей browser session;
- очищаться, если выбранный project больше недоступен;
- быть доступным будущим экранам через общую React boundary без отдельного WebSocket.

Не добавляем user-specific saved preferences до `CORE-005`.

Не добавляем project field в каждый Service Hub message.

### Результат

Web Shell имеет реальный project context, но остаётся способен работать в глобальном режиме.

## Step 6 — Реальная integration и restart recovery

### Что делаем

Проверяем полный путь:

`Web Shell → Service Hub → Project Manager → durable storage`

и обратно.

Automation должен использовать реальные процессы Service Hub и Project Manager.

Проверяем как минимум:

- создание project из browser;
- список/get/update;
- выбор project context;
- несколько последовательных/параллельных project requests там, где это уместно;
- restart Project Manager;
- повторную provider registration;
- сохранность project data после restart;
- повторное получение project из Web после restart;
- локальное понятное состояние при временно недоступном Project Manager/Hub;
- сохранение базового Header/navigation/workspace behavior.

### Результат

Project Manager подтверждён не только unit tests, но и реальной межпроцессной/browser boundary.

## Step 7 — Sprint acceptance, итоговый отчёт и documentation audit

### Что делаем

Новых функций не добавляем, кроме исправлений, необходимых критериям завершения.

Выполняем финальный targeted acceptance:

- Project Manager build/tests;
- persistence/restart;
- Service Hub provider integration;
- Web typecheck/unit tests;
- production Web build/browser smoke;
- real browser/Service Hub/Project Manager integration;
- project context;
- отсутствие ложной ownership/auth functionality.

После этого:

- заполняем итоговый отчёт этого файла;
- синхронизируем architecture/project contract docs;
- обновляем roadmap/root/docs/service READMEs/context;
- проверяем, не устарела ли концепция проектов или Web UI;
- closure commit фиксируется отдельно и его SHA проверяется перед `CORE-005`.

### Результат

`CORE-004 — Project Manager` завершён и даёт реальную основу project context для следующих сервисов.

# Что сознательно не входит в CORE-004

- Users & Access;
- authentication;
- roles/permissions/ACL;
- control mode;
- вложенные проекты;
- Dashboard implementation;
- связи project ↔ Dashboard до реального Dashboard contract;
- Device Manager;
- project-owned devices/metrics;
- Event Hub/Event Manager;
- automation;
- Package Manager;
- универсальный resource-membership registry;
- новый Service Hub envelope;
- второй backend transport;
- global database service;
- history/audit trail;
- production deployment/TLS/origin policy;
- production-final design system.

# Итоговый отчёт

Заполняется после завершения спринта.

## Фактически реализовано

Пока не заполнено.

## Выполненные проверки

Пока не заполнено.

## Отклонения от плана

Пока не заполнено.

## Известные ограничения

Пока не заполнено.

## Проверка актуальности документации

Пока не выполнена.

## Итоговый baseline

Пока не определён.
