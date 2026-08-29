# CORE-003 — Web Shell

## Статус

**В разработке.**

Этап: `L1-01 — Ядро платформы`.

Базовая точка перед началом спринта:

`7df47f234f6e0638e0f41ef81706d05244d7d2ea`

Этот файл является планом спринта и после завершения будет дополнен итоговым отчётом.

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
