# Web Platform — Functional Specification

**ID namespace:** `WEB-*`  
**Статус:** `NOT STARTED` — зафиксированы границы и зависимости.  
**Зависит от:** реальных UX-потребностей `ENG-*` и `OPS-*`.  
**Reference scenarios:** все основные `RS-*`.

## 1. Назначение

Спецификация определяет единый пользовательский контракт web-платформы Dispatcher, на которой работают Engineering, Operations и специализированные сервисы.

Это **не frontend architecture** и не design mockup. Документ отвечает на вопросы:

- какие общие UI-паттерны обязаны быть одинаковыми;
- как сохраняется контекст и URL;
- как интерфейс показывает realtime/degraded/permission state;
- как редакторы и специализированные workspaces встраиваются в один shell;
- какие правила не позволяют каждому сервису изобретать собственную навигацию и interaction model.

## 2. Уже принятые ограничения

Web specification обязана сохранять решения общей концепции:

- один global header;
- слева hamburger с глобальным меню установленных сервисов;
- справа user menu только для персональных функций;
- внутри сервиса при необходимости одна service-local navigation слева;
- одна service-local topbar для search/filters/actions;
- центральная рабочая область;
- right inspector;
- editor layout: tools/templates слева, canvas/working area центр, properties справа;
- никаких stacked headers;
- нет platform-wide global search;
- search живёт внутри subject service/list;
- специализированный fullscreen может минимизировать shell;
- desktop/mobile/kiosk/wallboard имеют общий design system, но не обязаны быть одним layout.

## 3. План разделов

1. `WEB-01` Global shell.
2. `WEB-02` Service navigation.
3. `WEB-03` Routing, stable URLs, browser back/forward.
4. `WEB-04` Global and local context propagation.
5. `WEB-05` Registries/tables.
6. `WEB-06` Search/filter/saved views.
7. `WEB-07` Inspector.
8. `WEB-08` Editors framework.
9. `WEB-09` Realtime subscriptions and update behaviour.
10. `WEB-10` Loading/progress/background operations.
11. `WEB-11` Error/degraded/offline states.
12. `WEB-12` Permissions, disabled actions and explanations.
13. `WEB-13` Dialogs/confirmations/risk actions.
14. `WEB-14` Personal notification/account surfaces.
15. `WEB-15` Fullscreen/kiosk/wallboard.
16. `WEB-16` Responsive/mobile principles.
17. `WEB-17` Accessibility/density/localization.
18. `WEB-18` UI performance contract: virtualization, paging, incremental loading.
19. `WEB-19` Specialized service integration contract.
20. `WEB-20` Contextual links between services.

## 4. Критерий завершения

Engineering и Operations должны быть описываемы поверх одного набора Web Platform правил без:

- второго global header;
- дублирующих navigation patterns;
- service-specific reinvention common registry/inspector behaviour;
- потери context при переходах;
- неясного representation realtime/permissions/degraded state.

## 5. Принятые functional requirements

Пока отсутствуют. Содержательная работа начинается после появления первых устойчивых flows в `ENG-*` и `OPS-*`.
