# Engineering & Configuration — Functional Specification

**ID namespace:** `ENG-*`  
**Статус:** `IN PROGRESS` — приняты решения `ENG-Q001–ENG-Q110`; первый смысловой checkpoint подготовлен.  
**Зависит от:** `PRD-Q001–PRD-Q803`.  
**Reference scenarios:** `RS-001`, `RS-002`, `RS-003`, `RS-004`, `RS-005`, `RS-012`, `RS-014`, `RS-019`.

## 1. Назначение

Спецификация описывает, как инженер создаёт, изменяет, проверяет, публикует и сопровождает конфигурацию Dispatcher от одного Compact-контроллера до распределённой Full/Edge установки.

Она должна дать однозначный ответ на практический вопрос:

> Инженер открыл Dispatcher и должен превратить пустую/существующую установку в корректно настроенную работающую систему. Какие рабочие области, сущности, действия, проверки и состояния он проходит?

## 2. Граница спецификации

Входит:

- Engineering workspace;
- locations и object registries;
- managed/observed objects;
- types, device profiles, templates;
- connections/adapters/endpoints;
- parameters и semantic commands;
- relationships;
- discovery;
- import;
- Create copies;
- draft/change sets;
- collaborative editing;
- validation;
- impact analysis;
- approval;
- publish;
- deploy/activate;
- desired/active state Edge;
- version/recovery views;
- engineering diagnostics;
- engineering permissions;
- simplified setup для Compact поверх той же модели.

Не входит как подробная предметная спецификация:

- ежедневная operator workflow — `OPS-*`;
- общие UI primitives/shell — `WEB-*`;
- внутренний protocol/API implementation;
- конкретная БД;
- реализация distributed configuration transport;
- детальный редактор Dashboard/Mimic/VMS/ACS/ТОиР, кроме точки интеграции с Engineering.

## 3. Неподвижные инварианты из Product Concept

Спецификация обязана сохранять:

- одна core identity объекта;
- type / device profile / object template разделены;
- configuration discovery/proposal ≠ authoritative runtime observation;
- import создаёт обычную новую конфигурацию в editor draft и не является sync/update механизмом;
- managed configuration проходит Draft → Validate → Impact → Approval(policy) → Publish;
- Publish ≠ Deploy ≠ Activate;
- Full задаёт desired operational Edge configuration, actual/active state сообщается отдельно;
- managed actuator action всегда проходит Command Model;
- package/service editor не получает параллельный Save/Apply lifecycle;
- normal configuration change не выполняется скрытой administrative transaction;
- Compact использует тот же foundation и может быть включён как Edge без пересоздания проекта.

## 4. Структура спецификации

### ENG-01 — Engineering entry point и workspace

Engineering рассматривается как один специализированный service/workspace общего Web Shell. Внутри него находятся устойчивые рабочие области, а packages могут добавлять разрешённые специализированные области/редакторы без замены общей навигационной модели.

### ENG-02 — Locations и object registry

Нужно довести до полного функционального контракта создание/редактирование locations, heterogeneous Object Registry, physical placement, функциональные отношения, managed/observed representation и lifecycle/presence indicators.

### ENG-03 — Object creation model

Базовые решения по созданию, identity, duplicate/Create Copies, observed promotion, physical/functional split и deletion уже приняты в `ENG-Q051–ENG-Q110`.

### ENG-04 — Types, profiles, templates

Следующий активный тематический блок: отдельные UX и semantics для object/equipment type, device profile, object template, наследования, управляемых обновлений и package-provided/organization-defined definitions.

### ENG-05 — Connections и adapters

Открыт для последующей проработки.

### ENG-06 — Parameters

Открыт для последующей проработки.

### ENG-07 — Semantic commands

Открыт для последующей проработки.

### ENG-08 — Relationships

Базовая typed-relation модель принята в `ENG-Q081–ENG-Q088`; специализированные редакторы, ограничения и масштабные представления требуют дальнейшей детализации.

### ENG-09 — Discovery и observed objects

Базовая Engineering-семантика observed объектов принята в `ENG-Q096–ENG-Q102`; discovery proposal flow будет дополнен отдельным блоком.

### ENG-10 — Import и Create copies

Базовый Create Copies contract принят в `ENG-Q103–ENG-Q107`; import UX и schemas требуют отдельной детализации.

### ENG-11 — Draft и change sets

Основной пользовательский фундамент принят в `ENG-Q006–ENG-Q027` и `ENG-Q038–ENG-Q046`.

### ENG-12 — Validation

Базовый lifecycle validation принят в `ENG-Q047–ENG-Q050`; детальные уровни и subject validations требуют отдельного раунда.

### ENG-13 — Impact analysis

Базовое требование для deletion принято; полный impact contract остаётся открытым.

### ENG-14 — Approval и Publish

Открыт для отдельного раунда.

### ENG-15 — Deploy / Activate / Edge

Открыт для отдельного раунда.

### ENG-16 — Versions, correction и recovery

Открыт для отдельного раунда.

### ENG-17 — Engineering diagnostics

Открыт для отдельного раунда.

### ENG-18 — Compact simple setup

Открыт для отдельного раунда; обязан использовать ту же underlying model.

## 5. Принятые решения — Engineering foundation (`ENG-Q001–ENG-Q050`)

### 5.1 Engineering service и рабочий контекст

- **ENG-Q001 — B.** Engineering — один service с внутренними рабочими областями, а не набор глобально разрозненных сервисов Objects/Connections/Types.
- **ENG-Q002 — C.** При входе открывается последний доступный рабочий контекст; если его нет — Engineering overview.
- **ENG-Q003 — B.** Engineering имеет собственную постоянную service-local navigation.
- **ENG-Q004 — B.** Верхняя навигация строится вокруг нескольких устойчивых рабочих областей, внутри которых находятся реестры/редакторы; не выводить каждую сущность отдельным глобальным пунктом и не сводить Engineering к одному дереву.
- **ENG-Q005 — B.** Установленные packages/services могут добавлять разрешённые специализированные рабочие области/редакторы Engineering.
- **ENG-Q006 — C.** Основной рабочий контекст = change set + текущая предметная область/объект/location.
- **ENG-Q007 — B.** Текущий change set должен быть постоянно и однозначно видим в Engineering UI.
- **ENG-Q008 — B.** Published configuration можно просматривать без active change set в read-only режиме.
- **ENG-Q009 — B.** Команда Edit из published view предлагает выбрать существующий change set или создать новый; Compact UX может упростить этот шаг, не меняя модель.
- **ENG-Q010 — B.** Один пользователь может работать в нескольких change sets; конкретный browser context/tab имеет один текущий change set.

### 5.2 Change set как пользовательская сущность

- **ENG-Q011 — C.** Change set всегда имеет устойчивое отображаемое название; система может автоматически предложить имя.
- **ENG-Q012 — C.** Обязательная metadata change set: identity, name, creator, created time, last activity, state, scope summary; описание требуется по policy/risk, а не всегда.
- **ENG-Q013 — B.** Один change set может включать изменения разных типов managed-configuration entities.
- **ENG-Q014 — B.** Change set может охватывать managed configuration нескольких специализированных сервисов при наличии прав.
- **ENG-Q015 — B.** Change set может жить длительно — дни/недели — и не привязан к одной browser session.
- **ENG-Q016 — B.** Owner change set может отличаться от creator; поддерживается передача ответственности.
- **ENG-Q017 — B.** Помимо owner поддерживаются collaborators/reviewers/approvers согласно rights/workflow.
- **ENG-Q018 — C.** Видимость чужих change sets определяется permissions/scopes.

### 5.3 Autosave, undo, revisions и checkpoints

- **ENG-Q019 — B.** Изменения внутри change set автоматически сохраняются как draft revisions; обычная кнопка Save не является главным механизмом и не означает Publish.
- **ENG-Q020 — B.** UI явно показывает состояние autosave: Saved / Saving / Failed / Offline или эквивалентное.
- **ENG-Q021 — A.** Пользователь может закрыть вкладку после успешного autosave; при незавершённом/неудачном сохранении требуется предупреждение.
- **ENG-Q022 — C.** Поддерживается локальный undo в разумных редакторах и возврат change set к revision/checkpoint без переписывания истории.
- **ENG-Q023 — B.** Autosave может коалесцировать технические изменения; каждое нажатие клавиши не обязано быть отдельной пользовательской revision.
- **ENG-Q024 — B.** Инженер может вручную создавать именованный checkpoint change set.
- **ENG-Q025 — B.** Возврат к checkpoint создаёт новое состояние draft и не уничтожает последующую историю revisions.
- **ENG-Q026 — B.** Поддерживается структурный compare состояний change set.
- **ENG-Q027 — B.** Поддерживается compare change set с текущей active/published configuration как основной review mode.

### 5.4 Реестры и editor behaviour

- **ENG-Q028 — C.** Основной registry view — таблица; для структурных/пространственных задач доступен tree view.
- **ENG-Q029 — B.** Где обе формы осмысленны, table/tree переключаются без потери предметного контекста.
- **ENG-Q030 — C.** Обычный выбор строки открывает inspector/preview; явное действие открывает полноценный editor/context.
- **ENG-Q031 — B.** Double-click может быть desktop convenience, но не единственным/главным способом открытия editor.
- **ENG-Q032 — B.** Multi-selection открывает общий selection inspector.
- **ENG-Q033 — B.** Bulk edit разнородных объектов разрешён только для свойств/действий, семантически общих всей selection.
- **ENG-Q034 — B.** Полноценное редактирование обычной сущности выполняется в рабочей области/editor, а не в большом modal dialog.
- **ENG-Q035 — B.** Настройки сущности делятся на понятные semantic sections/tabs/workspaces; не создавать одну бесконечную форму.
- **ENG-Q036 — B.** Object editor может показывать runtime/actual context, но он визуально и семантически отделён от editable configuration.
- **ENG-Q037 — B.** Operational values не редактируются как config properties; operational/diagnostic actions используют соответствующую семантику и Command Model.

### 5.5 Draft representation и collaboration

- **ENG-Q038 — B.** Draft changes имеют явные состояния/метки Added / Modified / Removed / Conflict и аналоги; цвет не является единственным сигналом.
- **ENG-Q039 — B.** Новый draft object виден в tree/registry текущего change set с явным признаком New/Draft и не становится active до publication.
- **ENG-Q040 — B.** Delete существующего объекта в change set означает planned removal; active object продолжает существовать до публикации.
- **ENG-Q041 — B.** Planned-for-removal object продолжает показывать live runtime state до фактической публикации удаления.
- **ENG-Q042 — B.** Несколько инженеров могут параллельно редактировать разные объекты одного change set.
- **ENG-Q043 — C.** По умолчанию используется optimistic concurrency с обнаружением конфликтов; сложные специализированные editor могут применять временные editing claims/locks.
- **ENG-Q044 — B.** UI показывает presence коллеги, работающего с тем же объектом, но presence сам по себе не обязательно блокирует редактирование.
- **ENG-Q045 — C.** Настоящий конфликт показывается явно и разрешается на уровне затронутых semantic properties/structures, где это возможно; last-write-wins не допускается как незаметная политика.
- **ENG-Q046 — B.** Неразрешённый conflict может оставаться explicit issue в change set; публикация затронутого consistency/configuration scope блокируется до разрешения.

### 5.6 Validation foundation

- **ENG-Q047 — C.** Лёгкие проверки выполняются постоянно; full validation запускается явно и обязательно выполняется для состояния, которое публикуется.
- **ENG-Q048 — B.** Базовые UX severity: Error, Warning, Info.
- **ENG-Q049 — B.** Validation issues можно группировать/фильтровать по severity, object, problem type, location/consistency domain и другим полезным срезам.
- **ENG-Q050 — B.** Нельзя Publish состояние, отличающееся от последнего полного validation snapshot; после изменений full validation считается устаревшим.

## 6. Принятые решения — Objects & Structure (`ENG-Q051–ENG-Q110`)

### 6.1 Installation root и locations

- **ENG-Q051 — C.** Organization существует как системная сущность установки, но не является обычным managed object в технологическом дереве.
- **ENG-Q052 — B.** Location — отдельная типизированная структурная сущность общей объектной модели, а не просто ordinary managed object.
- **ENG-Q053 — B.** Location hierarchy поддерживает произвольную разумную глубину и типизированные уровни.
- **ENG-Q054 — C.** Dispatcher предоставляет базовые location types, packages могут добавлять свои; building-oriented схема не навязывается.
- **ENG-Q055 — B.** Managed object может не иметь physical location, если его класс логический/виртуальный/системный/агрегированный или иным образом не требует физического размещения.
- **ENG-Q056 — B.** Physical object может временно не иметь assigned location; UI показывает это явно, а validation/policy определяет допустимость публикации.
- **ENG-Q057 — B.** У объекта один primary physical placement; дополнительные пространственные/функциональные отношения задаются typed relations.
- **ENG-Q058 — B.** Перемещение объекта между locations меняет placement существующей identity, если это тот же объект.
- **ENG-Q059 — B.** Physical placement и functional affiliation отображаются раздельно.
- **ENG-Q060 — B.** Locations поддерживают type-defined properties/extensions, включая при необходимости address, coordinates, timezone и domain metadata.

### 6.2 Object Registry

- **ENG-Q061 — B.** Существует общий Object Registry плюс специализированные представления/фильтры.
- **ENG-Q062 — B.** Базовая строка Object Registry представляет managed object identity, а не profile или parameter.
- **ENG-Q063 — B.** Общий registry включает physical, logical, virtual, system, aggregate и другие object classes с фильтрацией.
- **ENG-Q064 — B.** Для установок со split model существует специализированный Physical Units registry/view.
- **ENG-Q065 — B.** Observed objects имеют специализированное представление/фильтр общего object foundation, а не отдельную несвязанную сущность.
- **ENG-Q066 — B.** Большие tree/registry views обязаны поддерживать scoped browsing, filtering и incremental/lazy presentation; требование — не загружать всю установку как обязательное условие навигации.

### 6.3 Создание объекта

- **ENG-Q067 — B.** Context-sensitive Create объединяет допустимые способы: manual, type/profile/template, duplicate/Create Copies, discovery proposal, import into draft и package-provided predefined structure.
- **ENG-Q068 — C.** При создании требуется object class/type contract; полностью бесформенный untyped object не является нормой, но generic type допустим.
- **ENG-Q069 — B.** Display name объекта обязателен.
- **ENG-Q070 — C.** Internal identity всегда уникальна; display name не обязан быть уникальным глобально, а local naming uniqueness может задаваться policy/context.
- **ENG-Q071 — B.** Помимо display name поддерживается отдельный engineering code/tag; он может быть optional или policy-required.
- **ENG-Q072 — B.** Engineering code/tag можно изменить как managed configuration, не меняя stable internal identity.
- **ENG-Q073 — B.** Сложные templates/profiles показывают preview результата; простой Create не обязан проходить отдельный preview wizard.
- **ENG-Q074 — B.** Wizard применяется только там, где последовательный ввод действительно нужен; простые сущности создаются быстро.
- **ENG-Q075 — B.** После Create объект сразу появляется в текущем change set как New/Draft, а не active.

### 6.4 Identity и references

- **ENG-Q076 — B.** Internal Dispatcher ID управляется системой и не задаётся пользователем вручную.
- **ENG-Q077 — B.** Internal ID доступен в details/diagnostics/copy-reference, но не является главным визуальным именем объекта.
- **ENG-Q078 — B.** Rules, Dashboards, relations и другие устойчивые ссылки используют stable identity; UI отображает human-readable labels.
- **ENG-Q079 — B.** Rename/move не ломают ссылки, пока сохраняется identity.
- **ENG-Q080 — B.** Удалённая object identity автоматически не переиспользуется новым объектом.

### 6.5 Hierarchy и typed relations

- **ENG-Q081 — B.** Navigation hierarchy не равна универсальному ownership; semantic relationships задаются явно.
- **ENG-Q082 — B.** Объект может иметь несколько functional relationships/parents; primary navigation placement может быть один или отсутствовать.
- **ENG-Q083 — B.** Один объект в нескольких views остаётся одной identity, а не копиями.
- **ENG-Q084 — B.** Relationships имеют explicit type и direction/semantics где это применимо.
- **ENG-Q085 — B.** Package может добавлять relationship types с объявленной семантикой и compatibility constraints.
- **ENG-Q086 — B.** Relationship type может определять собственные properties, например role, priority или effective period.
- **ENG-Q087 — B.** Relationship constraints валидируют совместимость endpoints и другие semantic restrictions.
- **ENG-Q088 — B.** Object editor/inspector показывает связанные объекты с type/direction; graph view не является единственным способом просмотра.

### 6.6 Functional position и physical unit

- **ENG-Q089 — B.** Split functional-position/physical-unit model не обязателен для каждой установки; simple combined object остаётся полноценным вариантом.
- **ENG-Q090 — B.** Type/template задаёт разумный default modelling mode; инженер применяет split там, где нужна traceability конкретных physical units.
- **ENG-Q091 — B.** Functional position сохраняет identity при замене установленной physical unit.
- **ENG-Q092 — B.** Physical unit сохраняет identity через storage → installation → removal → repair → reinstallation.
- **ENG-Q093 — B.** При split model Engineering показывает текущую installed physical unit в контексте functional position.
- **ENG-Q094 — B.** Install/remove physical unit — семантически значимая операция с датой, историей и проверками, а не простое редактирование одного поля.
- **ENG-Q095 — B.** Если relation type означает mutually-exclusive physical installation, одна physical unit не может одновременно находиться в нескольких таких positions; validation блокирует конфликт.

### 6.7 Observed objects

- **ENG-Q096 — B.** Observed objects видимы в Engineering с явным source-owned/observed status и ограничениями редактирования.
- **ENG-Q097 — B.** Source-owned observed properties показываются как facts и не редактируются как Dispatcher configuration; редактируются только Dispatcher-owned metadata/configuration.
- **ENG-Q098 — B.** Observed object может быть Promote/Manage через governed transition без смены core identity.
- **ENG-Q099 — B.** Исчезновение observed object из source не удаляет identity; отображается presence/disappearance state согласно source contract.
- **ENG-Q100 — B.** Повторное появление raw source ID сопоставляется согласно source identity/incarnation contract; reused ID не склеивается автоматически.
- **ENG-Q101 — B.** UI показывает source/namespace/external identity и при необходимости incarnation/provenance observed identity.
- **ENG-Q102 — B.** Ambiguous merge/split/rebind разрешается как governed correction с preview влияния и сохранением provenance.

### 6.8 Duplicate / Create Copies

- **ENG-Q103 — B.** Duplicate создаёт новый draft object с новой stable identity.
- **ENG-Q104 — B.** Copy policy определяется типом свойства; identity/runtime/history/physical-unit identity не копируются, а UI показывает, что именно будет перенесено.
- **ENG-Q105 — B.** Create Copies — полноценный массовый механизм, а не частный import workaround.
- **ENG-Q106 — A.** Базовый Create Copies contract включает count, deterministic numbering/naming, code/tag rules, placement rules, parameter substitutions, preview и collision detection без обязательного scripting language.
- **ENG-Q107 — B.** Create Copies не выполняет automatic Publish; результат — обычные draft entities текущего change set.

### 6.9 Move / Delete / historical continuity

- **ENG-Q108 — B.** Planned deletion проходит impact analysis по references, relations, rules, dashboards, commands, extensions, history semantics и другим зависимостям до publication.
- **ENG-Q109 — B.** Planned deletion можно отменить обычным revert внутри change set до публикации.
- **ENG-Q110 — B.** После опубликованного удаления объект перестаёт быть active, но сохраняются identity/semantic metadata, необходимые для истории и references; повторное создание формирует новую identity.

## 7. Принятые functional requirements (`ENG-FR001–ENG-FR025`)

- **ENG-FR001.** Engineering является специализированным workspace внутри общего Web Shell Dispatcher, а не отдельным приложением.
- **ENG-FR002.** Engineering работает с общей объектной и конфигурационной моделью Dispatcher; специализированные сервисы не получают параллельный механизм сохранения managed configuration.
- **ENG-FR003.** Active published configuration по умолчанию не редактируется непосредственно: изменения выполняются через draft/change set.
- **ENG-FR004.** Переход между объектами, реестрами и редакторами не должен терять незавершённые изменения текущего change set.
- **ENG-FR005.** Validation issue, связанная с конкретной сущностью, должна идентифицировать эту сущность и, где возможно, давать переход к месту исправления.
- **ENG-FR006.** Engineering должен работать на Compact и Full/Edge; composition/density могут различаться, фундаментальная модель — нет.
- **ENG-FR007.** Permissions влияют на доступность Engineering, отдельных actions, objects/scopes, configuration areas и publication operations.
- **ENG-FR008.** Published и draft states должны визуально и семантически различаться.
- **ENG-FR009.** При недоступности backend/Full/Edge UI не выдаёт stale data за current и явно показывает connection/freshness/availability state.
- **ENG-FR010.** Engineering routes/URLs должны позволять устойчиво открыть конкретный object/registry/change set при наличии прав.
- **ENG-FR011.** Engineering предоставляет общий Object Registry с фильтрацией по class, type, location, lifecycle, configuration state и другим поддерживаемым свойствам.
- **ENG-FR012.** Один managed object может отображаться в нескольких структурных/функциональных views без создания дополнительных identities.
- **ENG-FR013.** Display name, engineering code/path и internal stable identity — разные понятия; изменение первых не меняет identity.
- **ENG-FR014.** Manual create, duplicate, discovery promotion и import создают сущности в текущем change set и не обходят governed configuration lifecycle.
- **ENG-FR015.** Draft representation объекта явно отличается от active representation; пользователь может сравнить effective draft и published state.
- **ENG-FR016.** Location hierarchy не ограничивается building-oriented схемой и поддерживает domain-specific location types.
- **ENG-FR017.** Изменение location, navigation parent и relationships сохраняет object identity, если не выполняется семантическая замена объекта.
- **ENG-FR018.** Typed relations доступны как минимум из editor/inspector обоих endpoints с указанием type/direction где применимо.
- **ENG-FR019.** Source-owned properties observed objects визуально отличаются от Dispatcher-managed configuration.
- **ENG-FR020.** Disappeared observed object не удаляется автоматически вместе с accumulated operational/history identity.
- **ENG-FR021.** Split functional-position/physical-unit model и simple combined model — два допустимых способа моделирования в одной object foundation.
- **ENG-FR022.** Install/remove physical unit сохраняет history installation intervals и identities обеих сторон.
- **ENG-FR023.** Duplicate/Create Copies не копирует internal identity, runtime/history/audit history или identity конкретной physical unit.
- **ENG-FR024.** Create Copies показывает preview итоговых entities и выявляет collisions до добавления результата в draft.
- **ENG-FR025.** Published deletion configuration object не уничтожает данные, необходимые для интерпретации historical facts и persistent references.

## 8. Traceability / Decision Register

| Диапазон | Статус | Тема |
|---|---|---|
| `ENG-Q001–ENG-Q050` | `ACCEPTED` | Engineering foundation: service/workspace, change sets, autosave, registries/editors, collaboration, validation foundation |
| `ENG-Q051–ENG-Q110` | `ACCEPTED` | Objects & Structure: locations, registry, creation, identity, relations, physical units, observed objects, copies, deletion |
| `ENG-FR001–ENG-FR025` | `ACCEPTED` | Нормативные functional requirements, сформированные из первых двух раундов и Product Concept |

Правило: решение, которое не требует отдельного вопроса из-за очевидности или прямого следования Product Concept, всё равно должно быть записано как FR/нормативное правило до закрытия тематического блока.

## 9. Следующая точка проработки

Следующий тематический блок начинается с `ENG-Q111`:

> **Types, Device Profiles, Object Templates, inheritance/composition и managed update semantics.**

Диапазон заранее не ограничивается. Блок заканчивается по смысловой полноте, после чего применяется checkpoint protocol из `../ROADMAP.md`.

## 10. Критерий завершения Engineering specification

Спецификация готова к `REVIEW`, когда можно без архитектурных догадок пройти минимум:

- `RS-001` новый контроллер → live data;
- `RS-002` массовое копирование;
- `RS-003` discovery proposal;
- `RS-004` observed VM → promotion;
- `RS-005` Compact small-site setup;
- `RS-012` stale Full restore;
- `RS-014` combined object split;
- `RS-019` Edge authority handover.
