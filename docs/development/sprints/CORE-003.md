# CORE-003 — Web Shell

## Статус

**Завершён.**

Этап: `L1-01 — Ядро платформы`.

Базовая точка перед началом спринта:

`7df47f234f6e0638e0f41ef81706d05244d7d2ea`

Этот файл содержит план и итоговый отчёт завершённого спринта.

## Цель

Создать самостоятельную базовую Web-оболочку платформы на React + TypeScript.

Web Shell должен дать будущим сервисам единый пользовательский каркас:

- компактный глобальный Header;
- глобальную навигацию;
- максимальную рабочую область;
- общую frontend-основу взаимодействия с backend через Service Hub.

Спринт не создаёт предметные Web-интерфейсы будущих сервисов. Его задача — подготовить устойчивую оболочку, в которую они будут добавляться следующими спринтами.

## Исходные решения

В репозитории уже зафиксированы:

- Web frontend: React + TypeScript;
- Node.js используется только как frontend toolchain;
- Service Hub v1 доступен браузеру напрямую по WebSocket;
- endpoint: `/v1/ws`;
- subprotocol: `dispatcher.service-hub.v1`;
- application messages: UTF-8 JSON;
- отдельный browser gateway для Service Hub не требуется.

`CORE-003` не меняет Service Hub contract без реальной найденной необходимости.

## Архитектурная граница спринта

Web Shell отвечает за общую оболочку Web-приложения и базовую frontend-инфраструктуру.

Он не должен становиться:

- Project Manager;
- Device Manager;
- Users & Access;
- Event Manager;
- Dashboard;
- Mimics;
- Package Manager;
- отдельным backend gateway;
- системой уведомлений;
- системой аутентификации.

Глобальный Header и навигация должны соответствовать согласованной концепции Web UI, но элементы будущих функций не должны изображаться работающими раньше соответствующих сервисов.

Например, наличие концептуального места под события, сообщения или пользовательское меню не означает, что CORE-003 должен реализовать Event Manager, пользовательские сообщения или authentication.

## Frontend technology boundary

React + TypeScript уже утверждены.

В этом спринте выбирается только минимальный toolchain, необходимый для реального frontend development/build/test.

Frontend state manager, большая UI-библиотека и design system не выбираются заранее без необходимости.

Для первого shell предпочтительны стандартные возможности React, Web Platform и простой локальный styling. Если на конкретном шаге выяснится реальная необходимость дополнительной библиотеки, решение фиксируется тогда.

## Ожидаемый результат

К завершению `CORE-003` должен существовать самостоятельный Web Shell, который:

- располагается в отдельном frontend-дереве репозитория;
- имеет воспроизводимый Node-based install/build/test workflow;
- собирается как production frontend;
- открывается как самостоятельное Web-приложение;
- показывает компактный глобальный Header;
- имеет работающий механизм глобального меню/навигации;
- оставляет основную площадь экрана рабочей области;
- не показывает предметные функции сервисов, которых ещё нет;
- содержит TypeScript client для Service Hub v1;
- умеет подключаться к настраиваемому Service Hub URL;
- умеет отправить request, сопоставить response и отменить request;
- корректно отражает состояние backend connection;
- не падает, если Service Hub временно недоступен;
- подтверждает реальный request/response через Service Hub test provider;
- имеет автоматические frontend-проверки и browser smoke/integration проверку;
- не вводит authentication или второй backend transport.

## Критерий завершения

Спринт завершён, если подтверждается сценарий:

1. Frontend dependencies устанавливаются воспроизводимо из зафиксированного lockfile.
2. TypeScript/frontend checks проходят.
3. Production build Web Shell завершается успешно.
4. Собранное приложение открывается в browser test environment.
5. На странице присутствуют глобальный Header, глобальная навигация и рабочая область.
6. Глобальное меню открывается и закрывается предсказуемо.
7. Будущие сервисы не представлены ложными рабочими экранами или действиями.
8. Web Shell создаёт прямое WebSocket connection к Service Hub с subprotocol `dispatcher.service-hub.v1`.
9. Frontend Service Hub client выполняет реальный request/response через test provider.
10. Несколько frontend requests корректно сопоставляются по client request ID.
11. Cancel завершает соответствующий frontend request предсказуемо.
12. Недоступный или закрытый Service Hub переводит frontend client в понятное connection state и не разрушает React application.
13. После явного нового подключения frontend client снова способен выполнить request; автоматическая production reconnect policy в этом спринте не требуется.
14. Выполнены обязательные тесты и sprint acceptance.
15. Проведена финальная целевая ревизия документации.

# Шаги

## Step 1 — Frontend-каркас и build/test toolchain

### Что делаем

Создаём первое отдельное frontend-дерево проекта.

Базовая точка размещения:

`web/`

На шаге фиксируем минимальный Node-based toolchain для:

- React;
- TypeScript;
- development server;
- production build;
- type checking;
- unit/component tests;
- browser smoke/integration test.

Должны появиться:

- package manifest;
- lockfile;
- минимальное React application entry point;
- команды install/build/test;
- базовая документация запуска.

Точные frontend packages и их версии выбираются по актуальному состоянию ecosystem на этом шаге, а не по памяти или старым шаблонам.

Не добавляем state manager или крупную UI-библиотеку только ради каркаса.

### Результат

Web Shell отдельно устанавливается, собирается и запускает минимальную React + TypeScript страницу.

### Решение Step 1

Frontend создаётся в `web/`.

Зафиксирован минимальный toolchain:

- Node.js 24.20.0 LTS;
- npm 11.19.0;
- React 19.2.8;
- Vite 8.1.x;
- TypeScript 6.0.3;
- Vitest 4.1.10;
- React Testing Library;
- Playwright 1.62.1 с Chromium для browser smoke.

Direct dependency versions фиксируются точно в `package.json`. Первый bootstrap создаёт `package-lock.json`, после чего чистые установки выполняются через `npm ci`.

Step 1 включает:

- минимальный React entry point;
- TypeScript project references;
- Vite development/production build;
- jsdom component test;
- production-preview browser smoke через Playwright;
- README с командами frontend-разработки.

State manager, router, UI kit и Service Hub client в Step 1 не добавляются.

Step 1 завершён commit:

`f974baa578ad310cbcd3403836d47fc2a32ec7d8`

## Step 2 — Базовый App Shell layout

### Что делаем

Создаём общий визуальный каркас согласно `docs/concept/10-web-ui.md`:

- компактный глобальный Header;
- trigger глобального меню слева;
- центральная рабочая область;
- структурное место для будущих глобальных действий справа;
- layout, который не забирает лишнее пространство у рабочей области.

Будущие Event Manager/messages/user функции не реализуем и не выдаём за работающие.

Shell должен нормально вести себя как минимум на обычном desktop viewport и при уменьшении ширины окна.

Не создаём Dashboard или service-specific editor layout.

### Результат

Существует узнаваемая базовая оболочка «Диспетчера», пригодная для размещения следующих Web-сервисов.

### Реализация Step 2

Bootstrap-screen Step 1 заменяется базовым App Shell:

- компактный global Header высотой 48 px;
- слева находится узнаваемый menu trigger и название «Диспетчер»;
- menu trigger намеренно disabled до Step 3, чтобы не изображать несуществующую навигацию работающей;
- справа резервируется пустая структурная область будущих глобальных действий без Event Manager/messages/user functionality;
- основную площадь занимает самостоятельная рабочая область;
- layout не создаёт Footer, service-specific panels, Dashboard или editor chrome;
- узкий viewport проверяется browser smoke test и не должен создавать горизонтальный overflow.

Component test подтверждает структуру Header/workspace и отсутствие дополнительных action buttons. Playwright проверяет production build на desktop и узком viewport.

Дополнительно `*.tsbuildinfo` фиксируется как generated build artifact и исключается из дальнейшего version control.

Step 2 завершён commit:

`a8f7b91a8b0c23385ce349c55ca6e6a70e9685c8`

## Step 3 — Глобальная навигация

### Что делаем

Реализуем минимальный механизм глобального меню и frontend navigation.

Нужно проверить:

- открытие/закрытие меню;
- закрытие предсказуемым пользовательским действием;
- текущую shell route/workspace;
- fallback для неизвестного frontend route;
- базовое keyboard-friendly поведение там, где оно необходимо меню.

Не добавляем ссылки на функциональность, которой ещё нет.

Project context не реализуем до `CORE-004 — Project Manager`.

Не создаём заранее универсальный plugin UI registry.

### Результат

Web Shell имеет рабочую навигационную основу, не зависящую от ещё не реализованных предметных сервисов.

### Реализация Step 3

Навигация остаётся частью Web Shell и не требует отдельной router-библиотеки:

- menu trigger из Step 2 становится рабочим и управляет глобальной navigation panel;
- единственный реальный destination текущего shell — `Рабочая область` по пути `/`;
- будущие сервисы не добавляются в меню до появления их реальных Web-интерфейсов;
- текущий route определяется через browser `location.pathname`, переходы shell выполняются через History API;
- `popstate` синхронизирует shell при browser history navigation;
- неизвестный pathname показывает shell-level fallback `Страница не найдена` с возвратом в рабочую область;
- trigger использует `aria-expanded`/`aria-controls`, активный пункт — `aria-current`;
- после открытия focus переходит на первый navigation item, `Escape` закрывает меню и возвращает focus на trigger;
- menu также закрывается повторным нажатием trigger, выбором пункта и кликом по backdrop;
- component и Playwright tests проверяют route/fallback, keyboard behavior, desktop/narrow viewport и отсутствие ложных future-service links.

Project context, service-specific routes, plugin UI registry и Service Hub client в Step 3 не добавляются.

Step 3 завершён commit:

`81264746c0948b664c30b91418b8cc477b6b2f82`

## Step 4 — TypeScript client Service Hub

### Что делаем

Создаём browser-oriented TypeScript client для уже подтверждённого Service Hub v1.

Client должен:

- принимать Service Hub URL через конфигурацию;
- открывать стандартный browser WebSocket;
- запрашивать subprotocol `dispatcher.service-hub.v1`;
- отслеживать connection state;
- генерировать client request IDs;
- отправлять `request`;
- хранить pending frontend requests;
- сопоставлять `response` с исходным request;
- поддерживать несколько активных requests;
- отправлять `cancel`;
- корректно завершать pending requests при закрытии connection;
- различать Hub error response и transport failure.

Не добавляем auth headers/tokens или новый envelope.

Автоматическая бесконечная reconnect policy и собственный heartbeat protocol не вводятся.

### Результат

Frontend имеет переиспользуемый client Service Hub, не связанный с конкретным React screen или предметным backend service.

### Реализация Step 4

Добавляется самостоятельный browser-oriented client в `web/src/service-hub/ServiceHubClient.ts`.

Client следует существующему Service Hub v1 contract без нового envelope или transport:

- принимает явный WebSocket URL через constructor options;
- открывает стандартный browser `WebSocket` с subprotocol `dispatcher.service-hub.v1`;
- имеет состояния `disconnected`, `connecting`, `connected`, `disconnecting` и подписку на их изменения;
- `connect()` и `disconnect()` не реализуют автоматическую reconnect policy;
- `request()` генерирует client request ID, отправляет `request` и возвращает handle с `id`, `response` Promise и `cancel()`;
- несколько pending requests хранятся независимо и завершаются по response `id`, включая ответы вне порядка отправки;
- optional `timeoutMs` передаётся как contract field `timeout_ms`; фактический request deadline остаётся ответственностью Hub;
- `cancel()` отправляет contract message `cancel` и ожидает нормальный Hub response `hub.cancelled`;
- `response { ok: false }` отклоняет request как `ServiceHubRequestError`;
- WebSocket close отклоняет все pending requests как `ServiceHubTransportError`;
- connection-level `protocol_error`, invalid JSON, unexpected message type и unknown response ID считаются protocol failure и закрывают socket с code `1002`;
- injectable WebSocket factory используется только как test seam; production default остаётся browser `WebSocket`.

Unit tests с fake WebSocket проверяют handshake/subprotocol, connection state, parallel correlation, Hub error, cancellation, disconnect и protocol failure. React lifecycle/context и отображение connection state остаются Step 5.

Новых frontend dependencies в Step 4 не требуется.

Локальные Web-проверки выполняются нативно в Windows на зафиксированных Node.js 24.20.0 / npm 11.19.0; C++ backend workflow остаётся Linux/WSL.

Step 4 завершён commit:

`02002f48a08cae6697b28be5e06b73864c2d9384`

## Step 5 — React-интеграция Service Hub

### Что делаем

Связываем Service Hub client с lifecycle Web Shell.

Нужно дать React application минимальную общую точку доступа к:

- connection state;
- Service Hub client/request API;
- clean connect/disconnect lifecycle.

Web Shell должен запускаться даже если backend недоступен.

Пользовательский интерфейс должен ненавязчиво показывать текущее состояние backend connection, чтобы отсутствие Service Hub не выглядело как сломанная пустая страница.

Не создаём глобальный production state manager только ради этой интеграции.

### Результат

Будущие Web-сервисы смогут использовать общую Service Hub connection вместо создания собственных WebSocket connections в каждом экране.

### Реализация Step 5

React application получает одну общую connection boundary поверх client Step 4:

- `web/src/service-hub/ServiceHubProvider.tsx` хранит общий React context с client и текущим connection state;
- `useServiceHub()` даёт будущим Web-экранам общий доступ к тому же client/request API без создания собственных WebSocket connections;
- Provider подписывается на connection state, выполняет `connect()` при mount и `disconnect()` при unmount;
- ошибка первоначального подключения не разрушает React tree: состояние остаётся/возвращается в `disconnected`, а shell продолжает работать;
- общий `ServiceHubClient` создаётся один раз в `main.tsx`, а Provider размещается выше `StrictMode`, чтобы dev StrictMode не выполнял двойной lifecycle реального WebSocket;
- `web/src/service-hub/serviceHubConfig.ts` использует `VITE_SERVICE_HUB_URL`, если он задан, иначе выводит same-origin URL `/v1/ws` с `ws://` для HTTP и `wss://` для HTTPS;
- правая область global Header показывает компактный status `Service Hub` для `disconnected`, `connecting`, `connected`, `disconnecting` без кнопок и ложной пользовательской функциональности;
- автоматическая reconnect policy, authentication и application heartbeat по-прежнему не добавляются.

Unit/component tests проверяют URL resolution, lifecycle Provider, доступ shared client/state через context и отображение state в App Shell. Playwright без запущенного backend подтверждает, что production shell остаётся рабочим и явно показывает недоступный Service Hub.

Реальный Service Hub, test provider и browser request/response path остаются Step 6.

Новых frontend dependencies в Step 5 не требуется.

Step 5 завершён commit:

`d612dcfec40c6447c35bc59993514fbb05e20e73`

## Step 6 — Реальная browser/backend интеграция

### Что делаем

Проверяем Web Shell против реального `dispatcher-service-hub`.

Используем test provider, предназначенный только для автоматической проверки.

Acceptance/integration path должен подтвердить:

`Web Shell client → Service Hub → test provider → Service Hub → Web Shell client`

Проверяем:

- реальное WebSocket connection;
- negotiated subprotocol;
- успешный request/response;
- несколько активных requests;
- cancel;
- состояние при недоступном/закрытом Service Hub;
- возможность выполнить новый request после явного нового подключения;
- production frontend build;
- открытие собранного Web Shell в browser test environment;
- базовое отображение Header/navigation/workspace.

Тестовый provider не становится production-сервисом платформы.

### Результат

Web Shell использует реальную Service Hub boundary, а не mock-only frontend contract.

### Реализация Step 6

Реальная browser/backend проверка оформляется отдельным automation path и не подменяет обычный backend-independent browser smoke.

- `web/e2e/run-service-hub-integration.mjs` инкрементально конфигурирует/собирает существующий C++ target `dispatcher_service_hub` через WSL и запускает его на временном loopback port;
- automation-only Node.js provider `web/e2e/support/service-hub-test-provider.mjs` подключается тем же WebSocket v1 protocol, регистрирует `test.web-shell` и существует только во время e2e-проверки;
- test provider не является production backend service и не добавляет новый transport;
- production frontend собирается с явным `VITE_SERVICE_HUB_URL` на реальный Hub;
- `VITE_SERVICE_HUB_E2E=1` включает только test seam, который отдаёт Playwright ссылку на уже существующий shared `ServiceHubClient` из `main.tsx`; обычный production build этот seam не включает;
- `web/e2e/service-hub.integration.spec.ts` выполняет request/response через реальный Hub и test provider;
- два одновременно активных `parallel-echo` requests provider возвращает в обратном порядке, подтверждая client-side correlation;
- unknown service подтверждает реальный Hub error `hub.unknown_service`;
- `wait-for-cancel` + `cancel-count` подтверждают client cancel, Hub response `hub.cancelled` и доставку provider cancel;
- integration control останавливает реальный Hub во время открытого browser application, после чего React status переходит в `disconnected`;
- Hub и provider запускаются снова, но автоматического reconnect нет; тест явно вызывает `client.connect()` и подтверждает новый успешный request;
- Header/navigation/workspace остаются рабочими после reconnect;
- обычный `npm.cmd run test:e2e` по-прежнему проверяет production shell при недоступном backend.

Новых frontend dependencies и production backend компонентов Step 6 не добавляет.

Step 6 завершён commit:

`f128c55748a5e2957151ba29b0c1d872614ccadf`

## Step 7 — Sprint acceptance, итоговый отчёт и documentation audit

### Что делаем

Новых функций не добавляем, кроме исправлений, необходимых для критериев завершения.

Выполняем финальную проверку:

1. чистая установка frontend dependencies;
2. type checks/tests;
3. production build;
4. browser smoke;
5. глобальный shell layout/navigation;
6. реальная Service Hub integration;
7. request correlation/cancel;
8. backend unavailable/reconnect behavior.

После этого:

- исправляем найденные проблемы строго в рамках `CORE-003`;
- заполняем итоговый отчёт в этом файле;
- проводим обязательную целевую ревизию документации;
- синхронизируем roadmap, README, frontend README, architecture/context при необходимости;
- пользователь фиксирует финальный commit и возвращает SHA;
- SHA проверяется в репозитории и становится baseline следующего спринта.

### Результат

`CORE-003` завершён, Web Shell является рабочей frontend-основой для `CORE-004 — Project Manager`.

# Что сознательно не входит в CORE-003

- Project Manager UI и project data;
- Users & Access UI;
- login/authentication;
- roles/permissions;
- control mode;
- Event Hub client;
- Event Manager UI;
- реальные notifications;
- пользовательские подписанные сообщения;
- рабочая логика user menu;
- Device Manager UI;
- Package Manager UI;
- Dashboard editor/runtime;
- Mimics;
- frontend Data Hub client;
- отдельный HTTP/gRPC-Web backend gateway;
- SSR;
- PWA/offline mode;
- mobile application;
- production hosting/reverse-proxy configuration;
- production TLS/Origin/auth policy;
- production-final reconnect/heartbeat strategy;
- универсальная plugin UI registration system;
- production-final design system;
- обязательный внешний state manager;
- обязательная большая UI component library.

Если новая задача не нужна для критерия завершения Web Shell, она не добавляется в спринт автоматически.

# Итоговый отчёт

## Фактически реализовано

`CORE-003 — Web Shell` создал самостоятельную frontend-основу платформы:

- отдельный `web/` с React + TypeScript, Vite, Vitest/Testing Library и Playwright;
- воспроизводимый npm workflow с зафиксированным lockfile и Node.js/npm baseline;
- компактный global Header, рабочую область и responsive layout без лишнего application chrome;
- глобальное меню с единственным реально существующим shell destination `/`, unknown-route fallback и keyboard-friendly focus/Escape behavior;
- самостоятельный browser-oriented `ServiceHubClient` для существующего Service Hub v1;
- configurable Service Hub URL, subprotocol `dispatcher.service-hub.v1`, connection state, client request IDs, parallel correlation, cancel и различение request/protocol/transport errors;
- shared React `ServiceHubProvider`/`useServiceHub()` с единым connect/disconnect lifecycle;
- ненавязчивый Service Hub connection status в Header при сохранении работоспособности shell без backend;
- реальную browser/backend integration через существующий C++ `dispatcher-service-hub` и automation-only test provider;
- проверку success, reverse-order parallel correlation, `hub.unknown_service`, cancel propagation, Hub shutdown/disconnected state и явного reconnect;
- отсутствие ложных экранов будущих сервисов, authentication, второго backend transport, обязательного router/state manager/UI kit.

Функциональный Step 6 завершён commit:

`f128c55748a5e2957151ba29b0c1d872614ccadf`

## Выполненные проверки

Перед фиксацией documentation-closure commit Step 7 выполняется полный acceptance-набор на зафиксированном toolchain:

    Set-Location C:\Projects\dispatcher\web
    npm.cmd ci
    npm.cmd run typecheck
    npm.cmd run test
    npm.cmd run test:e2e
    npm.cmd run test:e2e:service-hub

Closure commit допускается только при успешном выполнении всего блока.

Набор подтверждает:

- clean dependency install из `package-lock.json`;
- TypeScript checks;
- unit/component tests;
- fresh production build;
- backend-independent Playwright smoke/navigation/unavailable-backend behavior;
- реальный browser → Service Hub → test provider → Service Hub → browser path;
- параллельную correlation и cancel;
- disconnected state после остановки Hub;
- отсутствие automatic reconnect;
- успешный новый request после явного `connect()`.

## Отклонения от плана

Цель и архитектурная граница спринта не менялись.

Практический локальный workflow Web был уточнён по результатам реального запуска: frontend install/typecheck/Vitest/Playwright выполняются нативно в Windows через `npm.cmd`/`npx.cmd`, а C++ backend остаётся в Linux/WSL. Причина — запуск Vitest workers из WSL поверх `/mnt/c` оказался нестабильным и давал worker startup timeout, тогда как тот же зафиксированный frontend toolchain стабильно работает нативно в Windows.

Router, внешний state manager и UI kit не были добавлены, поскольку текущий shell не требует этих зависимостей.

## Известные ограничения

После `CORE-003` сознательно остаются вне Web Shell:

- Project Manager и project context — следующий `CORE-004`;
- authentication/authorization и user context;
- production TLS/Origin policy Service Hub;
- production-final reconnect/heartbeat policy;
- Event Hub client и notifications;
- Device Manager/Package Manager/Dashboard/Mimics и другие предметные Web-интерфейсы;
- frontend Data Hub client;
- production hosting/reverse-proxy configuration;
- универсальный plugin UI registry;
- production-final design system.

`VITE_SERVICE_HUB_E2E=1` и automation-only provider используются только интеграционным тестом и не являются production API/сервисом.

## Проверка актуальности документации

В Step 7 проведена целевая ревизия документов, которые могли устареть из-за завершения Web Shell:

- корневой `README.md` — CORE-003 отмечен завершённым, следующий спринт изменён на CORE-004;
- `docs/README.md` — CORE-003 переведён из текущего плана в завершённый plan/report;
- `docs/development/ROADMAP.md` — завершены Step 6 и CORE-003, текущая точка переведена на подготовку CORE-004;
- `docs/context/CHAT_CONTEXT.md` — зафиксирован итог CORE-003 и следующий шаг CORE-004;
- `web/README.md` — Step 6 и весь CORE-003 описаны как установленный baseline, команды Web/real integration сохранены актуальными;
- `services/service-hub/README.md` — удалены устаревшие формулировки о «будущем Web Shell», зафиксировано реальное использование browser boundary из CORE-003;
- `docs/architecture/README.md` и `docs/architecture/service-hub-contract.md` проверены: архитектурный транспорт/контракт не изменился, поэтому содержательных изменений контракта не требуется;
- concept-документы Web UI не требуют изменения: реализованный shell не меняет согласованную продуктовую концепцию.

## Итоговый baseline

Функциональный baseline перед закрывающей документацией:

`f128c55748a5e2957151ba29b0c1d872614ccadf`

Финальным baseline `CORE-003` является documentation-closure commit, которым фиксируется этот Step 7 отчёт после успешного acceptance. Его SHA намеренно не встраивается в тот же commit, чтобы не создавать рекурсивный commit только ради собственного SHA; пользователь возвращает этот SHA после push, и он отдельно проверяется в репозитории.
