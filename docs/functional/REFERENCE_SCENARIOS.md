# Reference Scenarios

**Назначение:** набор сквозных сценариев, которыми проверяется согласованность функциональных спецификаций Dispatcher.

Это не acceptance tests реализации и не исчерпывающий каталог use cases. Сценарии специально пересекают несколько функциональных областей и выявляют разрывы между ними.

## Правила

Каждая значимая functional specification должна явно указывать, какие `RS-*` она покрывает.

Сценарий считается функционально покрытым, если для него определены:

- пользователь/роль и контекст;
- исходное состояние;
- доступные действия;
- права/ограничения;
- наблюдаемые состояния и ошибки;
- переходы между сервисами/workspaces;
- Full/Compact/Edge semantics, где применимо;
- результат и audit/provenance, где применимо.

## RS-001 — Новый контроллер от подключения до live data

**Контекст:** инженер добавляет поддерживаемый контроллер на площадку.

Путь:

1. создать/выбрать location;
2. создать connection;
3. выбрать protocol adapter и device profile;
4. создать или привязать managed object;
5. настроить parameters;
6. пройти validation;
7. опубликовать change set;
8. доставить/активировать конфигурацию на нужном executor;
9. увидеть live value, quality, freshness и source;
10. открыть историю первого значения.

**Проверяет:** Engineering, Configuration, Edge placement, runtime, Web, Historian.

## RS-002 — Массовое добавление одинаковых устройств

Инженер настраивает один типовой счётчик/контроллер, затем создаёт 50 экземпляров с детерминированным изменением адресов, Modbus IDs, имён и помещений.

**Проверяет:** templates, Create copies, preview, validation, heterogeneous registries, publication impact.

## RS-003 — Discovery proposal физического оборудования

Драйвер обнаруживает новый физический контроллер, который должен быть инженерно принят как managed configuration.

**Проверяет:** configuration discovery/proposal, review, draft, collision handling, no silent active-model mutation.

## RS-004 — Dynamic observed VM

Внешний источник автоматически создаёт VM; Dispatcher без индивидуального publish создаёт observed object, показывает параметры/alarms/history. VM исчезает, затем появляется сущность с reusable source ID.

**Проверяет:** observed identity, namespace/incarnation, presence, promotion to managed, license visibility baseline.

## RS-005 — Compact как автономный контроллер малого объекта

На поддерживаемый embedded/industrial controller установлен Dispatcher Compact. Инженер создаёт помещения, свет, вентиляцию, ворота, локальные I/O и Rules; объект работает без Интернета.

**Проверяет:** scale-down Engineering UX, local adapters/I/O, Compact composition, local historian, commands, dashboards, offline autonomy.

## RS-006 — Авария насоса ночью

Диспетчер получает alarm остановки насоса, открывает object context, видит current state/quality, краткий trend, maintenance/manual-control context, выполняет разрешённую semantic command и видит результат.

**Проверяет:** Operations, Alarm handling, Commands, Historian, rights, effective operational context.

## RS-007 — Неопределённый результат команды

Команда отправлена через Edge; связь оборвалась после фактического выполнения, но до подтверждения Full.

**Проверяет:** command lifecycle, uncertain result, no blind retry, HA/reconnect, operator explanation, audit.

## RS-008 — Edge offline несколько суток

Full недоступен; локальный оператор продолжает разрешённую эксплуатацию, alarms/history/audit накапливаются, notification policy использует локальные каналы. После reconnect факты синхронизируются без повторной эскалации.

**Проверяет:** Edge autonomy, offline rights, notifications, immutable facts, reconciliation.

## RS-009 — Manual control + substitution + maintenance

Насос находится в ТОиР, переведён в manual control, один датчик substitution, часть alarm policy в maintenance mode, Rule продолжает наблюдение.

**Проверяет:** effective operational context, operational exceptions, command blocking, explainability.

## RS-010 — Плохое время Edge

Часы Edge имеют degraded time quality; во время partition происходят alarms, commands и telemetry, а временный operational exception подходит к expiry.

**Проверяет:** time quality, ordering uncertainty, exception lifecycle, reconnect semantics.

## RS-011 — Incident с VMS и СКУД

Alarm оборудования связан с проходом человека и видеозаписью. Оператор создаёт incident, прикладывает evidence и формирует export при частичных правах.

**Проверяет:** cross-domain identity, VMS/ACS links, sensitivity propagation, permissions, evidence/export.

## RS-012 — Восстановление Full из старого backup

Full восстановлен из backup, где desired configuration старее фактически активной конфигурации Edge.

**Проверяет:** recovery divergence, no automatic downgrade, consistency domains, recovery decision, audit lineage.

## RS-013 — Emergency Disable API

Администратор обнаруживает компрометацию внешнего API и выполняет Emergency Disable. Listener остаётся выключенным после restart/HA и не восстанавливается обычным desired-state reconciliation до явного снятия protective override.

**Проверяет:** Security Center, administrative transaction, desired-vs-actual, protective override, Web explanations.

## RS-014 — Замена физического насоса

Изначально насос моделировался combined object. Организация включает серийный учёт, materializes functional position + physical unit, затем заменяет физический насос.

**Проверяет:** identity migration, installed-device history, ТОиР, documents, preservation of operational references.

## RS-015 — Ошибка historical scaling

Два часа температуры записаны с неверным scaling. Инженер оформляет governed correction; текущий trend использует corrected interpretation, исходный provenance сохраняется, старый immutable report не переписывается.

**Проверяет:** Historian correction, aggregates, provenance, reports/evidence.

## RS-016 — Package удалён, история должна читаться

Установленный package ранее создал extension/event data. Package удалён как runtime, но старый incident должен по-прежнему показывать смысл полей и enum без запуска старого кода.

**Проверяет:** package semantic retention, historical interpretation, removal impact.

## RS-017 — Shared camera object

Камера существует как один core object и одновременно используется Equipment, VMS, dashboard, map, incident и ТОиР. Инженер пытается удалить/архивировать её.

**Проверяет:** service extensions, cross-service impact analysis, historical references.

## RS-018 — Data center mixed infrastructure

Одна площадка содержит UPS, generators, chillers, PDUs, racks, servers, switches, VMs и cameras. Часть объектов managed, часть observed.

**Проверяет:** универсальность object foundation, physical+digital relations, Engineering registries, Operations context, domain neutrality UI.

## RS-019 — Authority handover между Edge

Edge A недоступен Full, но потенциально продолжает управлять PLC. Инженер хочет перенести executor на Edge B.

**Проверяет:** desired executor vs actual authority, blocked handover, emergency recovery semantics, operator/engineer visibility.

## RS-020 — License exceed dynamic objects

Внешняя платформа создаёт runtime objects сверх лицензируемого scale.

**Проверяет:** mandatory visibility baseline, alarms/diagnostics, коммерческие ограничения без blind spot и без нарушения safety/recovery carve-outs.

## Использование при review

Перед переводом крупной спецификации в `DONE` выполняется короткая матрица:

| Scenario | Covered | Open gap | Owning spec |
|---|---|---|---|
| `RS-...` | yes/no | ... | ... |

Матрица может находиться непосредственно в review-разделе соответствующей спецификации; отдельный постоянный файл для неё не нужен.
