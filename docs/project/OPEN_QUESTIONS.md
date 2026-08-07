# Открытые предметные концепции

**Назначение:** компактный индекс тем, которые требуют отдельной подробной продуктовой проработки после завершения общей горизонтальной концепции.

Закрытые аудиты и решённые вопросы здесь не хранятся.

| ID | Область | Что требуется подробно определить |
|---|---|---|
| `AUT-Q001` | Automation | SCADA API для Rules, runtime lifecycle, libraries/modules, debugging, profiling, test/shadow execution, resource policies и developer workflow. |
| `VMS-Q001` | VMS | Камеры/каналы/потоки, запись и storage topology, archive timeline, Edge, external VMS/NVR, PTZ, analytics, privacy, evidence/export, права и UX. |
| `MAINT-Q001` | ТОиР | Defects/work orders, maintenance strategies, materials, labor, regulations/checklists, physical-unit replacement history, mobile work и ERP/EAM integration boundary. |
| `ACS-Q001` | СКУД | Точки доступа, controllers/readers, Person/subjects/credentials, access policies, zones, schedules, antipassback, emergency modes и VMS links. |
| `BIM-Q001` | BIM | Model connection/import, revisions, element↔object mapping, 3D workspace, rights, performance и final service/workspace topology без BIM authoring. |
| `MAP-Q001` | Maps / plans | Geographic maps, sites/buildings/floors, layers, coordinates, object bindings, editors и spatial navigation без универсального GIS. |
| `HVAC-Q001` | HVAC | Вентиляция, heating/cooling, valves, pumps, cascades, PID/schedules, alarms, mimics и TOиR как предметная проверка общей модели. |
| `ENERGY-Q001` | Energy | Power distribution, ATS, generators, UPS, meters, power quality, interval data, balances, demand/tariff analysis и граница отдельного energy service. |
| `FIRE-Q001` | Fire | Panels/zones/detectors, suppression, smoke control, evacuation context, rights и допустимые команды при сохранении локального certified safety contour. |
| `IT-Q001` | IT / networks | Servers, network devices, interfaces, topology, VM/containers/cloud resources, discovery/observation, metrics, dependencies, commands и diagnostics без обязательного NMS. |
| `UTIL-Q001` | Water / utilities | Reservoirs, pressure/flow, pump groups, valves, leakage, sewerage, meters, rules, alarms и ТОиР. |
| `VERTICAL-Q001` | Other domains | Lighting, lifts/escalators, intrusion/security systems, specialized industrial equipment и другие реальные вертикали. |
| `UX-Q001` | Dashboard Editor | Page/container hierarchy, responsive layout, properties, collaboration, templates, links and performance. |
| `UX-Q002` | Mimic Editor | Primitives, pipes/connections, symbols, layers, bindings, templates, scaling, collaboration and performance. |
| `PERF-Q001` | Quantitative performance | Численные performance contracts и аппаратные профили после появления измеряемых прототипов/load generators; состав метрик уже принят. |
| `IMP-Q001` | Import schemas | Конкретные текстовые schemas/columns/delimiters/versioning определяются внутри соответствующих subject editors; общий строгий import contract уже принят. |

## Принцип

Отдельная предметная концепция может уточнить общую модель и service boundary, если реальные сценарии этого требуют. Отсутствие подробностей конкретной вертикали в общей концепции не является дефектом горизонтального фундамента.
