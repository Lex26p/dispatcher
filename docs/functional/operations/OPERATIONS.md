# Operations / Dispatcher Workspace — Functional Specification

**ID namespace:** `OPS-*`  
**Статус:** `NOT STARTED` — зафиксированы только границы и план проработки.  
**Зависит от:** ключевой object/config semantics из `ENG-*`.  
**Reference scenarios:** `RS-006–RS-013`, `RS-017–RS-020`.

## 1. Назначение

Спецификация описывает ежедневную работу диспетчера и других эксплуатационных ролей с уже работающей системой.

Главный вопрос:

> Что видит и делает оператор от момента появления проблемы или необходимости действия до понятного результата, не переходя между несогласованными экранами и не теряя operational context?

## 2. Входит

- стартовый operational workspace;
- location/object context;
- current state;
- live parameters;
- quality/freshness/connection/degradation;
- alarms;
- semantic commands;
- command progress/result/uncertainty;
- trends/history in context;
- incidents;
- My Work/tasks;
- shift/handover;
- manual control/substitution/suppression/maintenance context;
- contextual VMS/ACS/ТОиР links;
- Full/Edge/offline representation;
- operator rights and action explanations;
- role/density/workspace preferences.

## 3. Не входит

- инженерное создание конфигурации — `ENG-*`;
- общий reusable Web shell/components contract — `WEB-*`;
- подробная VMS/ACS/ТОиР subject model;
- frontend/backend implementation.

## 4. План разделов

1. `OPS-01` Entry workspace and current responsibility.
2. `OPS-02` Location/object navigation.
3. `OPS-03` Effective operational context.
4. `OPS-04` Live state/parameters/quality.
5. `OPS-05` Alarm handling.
6. `OPS-06` Semantic commands.
7. `OPS-07` Trends/history context.
8. `OPS-08` Incident coordination.
9. `OPS-09` My Work/tasks/assignments.
10. `OPS-10` Shift and handover.
11. `OPS-11` Operational exceptions.
12. `OPS-12` VMS/ACS/ТОиР contextual transitions.
13. `OPS-13` Edge/offline/degraded operation.
14. `OPS-14` Rights, explanations and break-glass entry points.
15. `OPS-15` Specialized fullscreen/kiosk/mobile behaviour.

## 5. Критерий завершения

Dispatcher operator должен без неизвестных функциональных переходов пройти сценарии:

- alarm → diagnose → command → result;
- uncertain command;
- Edge offline + reconnect;
- maintenance/manual/substitution context;
- incident + VMS/ACS evidence;
- emergency degraded conditions.

## 6. Принятые functional requirements

Пока отсутствуют. Содержательная работа начинается после основной структуры `ENG-*`.
