# Открытые предметные области

**Назначение:** постоянный компактный индекс областей, для которых после общей концепции требуется отдельная функциональная или предметная детализация.

Текущий порядок работы и статусы **не дублируются здесь** — они находятся во временном `../functional/ROADMAP.md`.

Engineering, Operations и Web Platform уже переведены в functional layer:

- `../functional/engineering/ENGINEERING_CONFIGURATION.md`
- `../functional/operations/OPERATIONS.md`
- `../functional/web-platform/WEB_PLATFORM.md`

Оставшиеся крупные области:

| ID | Область | Что требуется подробно определить |
|---|---|---|
| `ALM-Q001` | Events / Alarms / Incidents / Notifications | Подробные operator flows, alarm list/details, episodes, ACK, correlation, storm handling, suppression, escalation, Edge delivery и reconnect UX. |
| `HIS-Q001` | Historian / Trends / Reports | Trend UX, history query flows, aggregates, corrections, counters, comparison, export/report integration. |
| `AUT-Q001` | Automation | SCADA API для Rules, runtime lifecycle, libraries/modules, debugging, profiling, test/shadow execution, resource policies и developer workflow. |
| `VMS-Q001` | VMS | Камеры/каналы/потоки, recording/storage topology, archive timeline, Edge, external VMS/NVR, PTZ, analytics, privacy, evidence/export, права и UX. |
| `MAINT-Q001` | ТОиР | Defects/work orders, maintenance strategies, materials, labor, regulations/checklists, physical-unit replacement history, mobile work и ERP/EAM boundary. |
| `ACS-Q001` | СКУД | Точки доступа, controllers/readers, Person/subjects/credentials, access policies, zones, schedules, antipassback, emergency modes и VMS links. |
| `SPAT-Q001` | Maps / Plans / BIM | Geographic/floor spatial model, layers, coordinates, BIM revisions, element↔object mapping, 2D/3D workspaces и service topology. |
| `IT-Q001` | IT / Networks / Virtualization | Servers, network devices, interfaces, topology, VM/containers/cloud resources, discovery/observation, metrics, dependencies, commands и diagnostics. |
| `DSH-Q001` | Dashboard Editor | Page/container hierarchy, responsive layout, properties, templates, links, context, permissions и performance. |
| `MIM-Q001` | Mimic Editor | Primitives, pipes/connections, symbols, layers, bindings, templates, scaling, collaboration и performance. |
| `HVAC-Q001` | HVAC | Вентиляция, heating/cooling, valves, pumps, supervisory PID/schedules, alarms, mimics и ТОиР как предметная проверка общей модели. |
| `ENERGY-Q001` | Energy | Power distribution, ATS, generators, UPS, meters, power quality, interval data, balances, demand/tariff analysis и граница energy service. |
| `FIRE-Q001` | Fire | Panels/zones/detectors, suppression, smoke control, evacuation context, rights и допустимые команды при сохранении локального certified safety contour. |
| `UTIL-Q001` | Water / utilities | Reservoirs, pressure/flow, pump groups, valves, leakage, sewerage, meters, rules, alarms и ТОиР. |
| `VERTICAL-Q001` | Other domains | Lighting, lifts/escalators, refrigeration, agriculture, telecom, transport, environmental monitoring и другие application domains как проверки foundation. |
| `PERF-Q001` | Quantitative performance | Численные performance contracts и аппаратные профили после появления измеряемых прототипов/load generators; состав метрик уже принят. |
| `IMP-Q001` | Import schemas | Конкретные schemas/columns/delimiters/versioning внутри subject editors; общий строгий import contract уже принят. |

## Принцип

Отдельная область не становится отдельным продуктовым сервисом автоматически. Детальная функциональная спецификация должна сначала использовать общие objects/parameters/commands/alarms/history/automation/dashboards/ТОиР и вводить специализированную модель только там, где реально существует собственный устойчивый workflow.
