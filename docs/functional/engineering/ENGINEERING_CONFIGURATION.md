# Engineering & Configuration — Functional Specification

**ID namespace:** `ENG-*`  
**Статус:** `IN PROGRESS` — приняты `ENG-Q001–ENG-Q790` и `ENG-FR001–ENG-FR150`; checkpoint `ENG-CP03` подготовлен.  
**Основание Product Concept:** `PRD-Q001–PRD-Q803`.  
**Последний подтверждённый Git SHA перед `ENG-CP03`:** `fa38f437a90f98cdb4091a25187eec67f2213e6a`.  
**Следующая точка:** `ENG-Q791` — Semantic Commands: definitions, arguments, safety, execution, confirmation, results, uncertainty, concurrency и diagnostics.

## 1. Назначение

Спецификация определяет наблюдаемое функциональное поведение Engineering: как инженер создаёт, изменяет, сравнивает, проверяет, публикует и сопровождает конфигурацию Dispatcher от одного Compact-контроллера до распределённой Full/Edge установки.

Критерий детализации: product designer, analyst и architect не должны самостоятельно придумывать основные пользовательские правила Engineering. Архитектурные механизмы, storage, transport, frameworks и код остаются за пределами этой спецификации до соответствующего этапа.

## 2. Граница

В Engineering входят locations/object registries; managed/observed objects; types/profiles/templates; connections/adapters/endpoints; parameters; semantic commands; relationships; discovery; import/Create Copies; change sets/collaboration; validation/impact; approval/publish; deploy/activate/Edge desired-vs-active; version/recovery views; engineering diagnostics/permissions; Compact simple setup поверх той же модели.

Отдельные подробные operator workflows относятся к `OPS-*`, общий Web Shell/UI primitives — к `WEB-*`, а runtime/API/storage/distributed implementation — к System Architecture.

## 3. Неподвижные инварианты Product Concept

- одна core identity управляемого объекта;
- Object Type / Device Profile / Object Template разделены;
- configuration discovery/proposal отличается от authoritative runtime observation;
- import создаёт обычную новую конфигурацию в draft и не является sync/update механизмом;
- managed configuration проходит Draft → Validate → Impact → Approval(policy) → Publish;
- Publish, Deploy и Activate — разные состояния/действия;
- Full задаёт desired operational Edge configuration, actual/active состояние сообщается отдельно;
- managed actuator action всегда проходит Command Model;
- package/service editor не получает параллельный Save/Apply lifecycle;
- Compact использует тот же foundation и может быть включён как Edge без пересоздания проекта.

## 4. Состояние предметных блоков

| Блок | Статус | Принятые решения |
|---|---|---|
| Engineering workspace / change sets / registries / collaboration / validation foundation | `DONE FOUNDATION` | `ENG-Q001–ENG-Q050` |
| Locations / objects / identity / relations / physical units / observed / copies / deletion | `DONE FOUNDATION` | `ENG-Q051–ENG-Q110` |
| Object Types / Device Profiles / Object Templates | `DONE FUNCTIONAL BLOCK` | `ENG-Q111–ENG-Q180` |
| Template/Profile lifecycle / propagation / overrides / migrations | `DONE FUNCTIONAL BLOCK` | `ENG-Q181–ENG-Q250` |
| Connections / Adapters / Endpoints / Credentials / execution placement | `DONE FUNCTIONAL BLOCK` | `ENG-Q251–ENG-Q340` |
| Parameters / Values / Sources / Quality / Historization / Substitution | `DONE FUNCTIONAL BLOCK` | `ENG-Q341–ENG-Q790` |
| Semantic Commands | `NEXT` | с `ENG-Q791` |
| Discovery proposal / Import | `OPEN` | базовые части уже приняты |
| Validation / Impact / Approval / Publish | `OPEN DETAIL` | foundation принят |
| Deploy / Activate / Edge | `OPEN` | — |
| Versions / Recovery / Diagnostics / Permissions / Compact setup | `OPEN` | — |

## 5. Decision Register — `ENG-Q001–ENG-Q050`

- **ENG-Q001.** Engineering — один service с внутренними рабочими областями, а не набор глобально разрозненных сервисов Objects/Connections/Types.
- **ENG-Q002.** При входе открывается последний доступный рабочий контекст; если его нет — Engineering overview.
- **ENG-Q003.** Engineering имеет собственную постоянную service-local navigation.
- **ENG-Q004.** Верхняя навигация строится вокруг нескольких устойчивых рабочих областей; не выводить каждую сущность отдельным глобальным пунктом и не сводить Engineering к одному дереву.
- **ENG-Q005.** Packages/services могут добавлять разрешённые специализированные рабочие области/редакторы Engineering без замены общей навигационной модели.
- **ENG-Q006.** Основной рабочий контекст = change set + текущая предметная область/объект/location.
- **ENG-Q007.** Текущий change set должен быть постоянно и однозначно видим в Engineering UI.
- **ENG-Q008.** Published configuration можно просматривать без active change set в read-only режиме.
- **ENG-Q009.** Edit из published view предлагает выбрать существующий change set или создать новый; Compact UX может упростить шаг, не меняя модель.
- **ENG-Q010.** Один пользователь может работать в нескольких change sets; конкретный browser context/tab имеет один текущий change set.
- **ENG-Q011.** Change set всегда имеет устойчивое отображаемое название; система может автоматически предложить имя.
- **ENG-Q012.** Обязательная metadata change set: identity, name, creator, created time, last activity, state, scope summary; описание требуется по policy/risk, а не всегда.
- **ENG-Q013.** Один change set может включать изменения разных типов managed-configuration entities.
- **ENG-Q014.** Change set может охватывать managed configuration нескольких специализированных сервисов при наличии прав.
- **ENG-Q015.** Change set может жить длительно — дни/недели — и не привязан к одной browser session.
- **ENG-Q016.** Owner change set может отличаться от creator; поддерживается передача ответственности.
- **ENG-Q017.** Помимо owner поддерживаются collaborators/reviewers/approvers согласно rights/workflow.
- **ENG-Q018.** Видимость чужих change sets определяется permissions/scopes.
- **ENG-Q019.** Изменения внутри change set автоматически сохраняются как draft revisions; обычная Save не является главным механизмом и не означает Publish.
- **ENG-Q020.** UI явно показывает состояние autosave: Saved / Saving / Failed / Offline или эквивалентное.
- **ENG-Q021.** Вкладку можно закрыть после успешного autosave; при незавершённом/неудачном сохранении требуется предупреждение.
- **ENG-Q022.** Поддерживается локальный undo в разумных редакторах и возврат change set к revision/checkpoint без переписывания истории.
- **ENG-Q023.** Autosave может коалесцировать технические изменения; каждое нажатие клавиши не обязано быть отдельной пользовательской revision.
- **ENG-Q024.** Инженер может вручную создавать именованный checkpoint change set.
- **ENG-Q025.** Возврат к checkpoint создаёт новое состояние draft и не уничтожает последующую историю revisions.
- **ENG-Q026.** Поддерживается структурный compare состояний change set.
- **ENG-Q027.** Поддерживается compare change set с текущей active/published configuration как основной review mode.
- **ENG-Q028.** Основной registry view — таблица; для структурных/пространственных задач доступен tree view.
- **ENG-Q029.** Где обе формы осмысленны, table/tree переключаются без потери предметного контекста.
- **ENG-Q030.** Обычный выбор строки открывает inspector/preview; явное действие открывает полноценный editor/context.
- **ENG-Q031.** Double-click может быть desktop convenience, но не единственным/главным способом открытия editor.
- **ENG-Q032.** Multi-selection открывает общий selection inspector.
- **ENG-Q033.** Bulk edit разнородных объектов разрешён только для свойств/действий, семантически общих всей selection.
- **ENG-Q034.** Полноценное редактирование обычной сущности выполняется в рабочей области/editor, а не в большом modal dialog.
- **ENG-Q035.** Настройки сущности делятся на понятные semantic sections/tabs/workspaces; не создавать одну бесконечную форму.
- **ENG-Q036.** Object editor может показывать runtime/actual context, но он визуально и семантически отделён от editable configuration.
- **ENG-Q037.** Operational values не редактируются как config properties; operational/diagnostic actions используют соответствующую семантику и Command Model.
- **ENG-Q038.** Draft changes имеют явные состояния/метки Added / Modified / Removed / Conflict и аналоги; цвет не является единственным сигналом.
- **ENG-Q039.** Новый draft object виден в tree/registry текущего change set с явным New/Draft и не становится active до publication.
- **ENG-Q040.** Delete существующего объекта в change set означает planned removal; active object продолжает существовать до публикации.
- **ENG-Q041.** Planned-for-removal object продолжает показывать live runtime state до фактической публикации удаления.
- **ENG-Q042.** Несколько инженеров могут параллельно редактировать разные объекты одного change set.
- **ENG-Q043.** По умолчанию optimistic concurrency с обнаружением конфликтов; сложные editor могут применять временные editing claims/locks.
- **ENG-Q044.** UI показывает presence коллеги, работающего с тем же объектом, но presence сам по себе не обязательно блокирует редактирование.
- **ENG-Q045.** Настоящий конфликт показывается явно и разрешается на уровне затронутых semantic properties/structures, где возможно; скрытый last-write-wins недопустим.
- **ENG-Q046.** Неразрешённый conflict может оставаться explicit issue; публикация затронутого consistency/configuration scope блокируется до разрешения.
- **ENG-Q047.** Лёгкие проверки выполняются постоянно; full validation запускается явно и обязателен для состояния, которое публикуется.
- **ENG-Q048.** Базовые уровни результата validation: Error, Warning, Info; не раздувать глобальную severity taxonomy без предметной причины.
- **ENG-Q049.** Validation issues можно группировать по severity, объекту, типу проблемы, location/consistency domain и другим полезным срезам.
- **ENG-Q050.** Нельзя Publish состояние, изменённое после validation: публикация должна опираться на validation именно публикуемого состояния.

## 6. Decision Register — `ENG-Q051–ENG-Q110`

- **ENG-Q051.** Организация существует как системная сущность/контекст установки, но не обязана быть обычным managed object в Engineering tree.
- **ENG-Q052.** Location — отдельная структурная сущность общей объектной модели со своей семантикой размещения, а не просто обычный managed object.
- **ENG-Q053.** Location hierarchy допускает произвольную разумную глубину с типизированными location types.
- **ENG-Q054.** Платформа предоставляет базовые location types, packages могут добавлять свои; building-oriented схема не навязывается.
- **ENG-Q055.** Managed object может существовать без physical location для логических, виртуальных, системных, агрегированных и иных осмысленных классов.
- **ENG-Q056.** Физический объект может временно/допустимо не иметь location; UI показывает Location not assigned, а policy/validation определяет допустимость публикации.
- **ENG-Q057.** У объекта один primary physical placement; дополнительные пространственные/функциональные связи задаются явными typed relations.
- **ENG-Q058.** Перенос объекта между locations изменяет placement существующей identity, если это тот же объект.
- **ENG-Q059.** Physical placement и functional/technological принадлежность должны отображаться раздельно.
- **ENG-Q060.** Locations поддерживают типизированные свойства/extensions, включая уместные адресные, координатные, timezone и пространственные данные.
- **ENG-Q061.** Есть общий Object Registry всех managed objects плюс специализированные views.
- **ENG-Q062.** Основная строка Object Registry соответствует managed object identity.
- **ENG-Q063.** Object Registry включает logical/virtual/system и другие object classes с фильтрацией.
- **ENG-Q064.** Отдельный Physical Units view/registry доступен там, где используется split functional-position/physical-unit model.
- **ENG-Q065.** Observed objects представлены специализированным view/filter общего object foundation, а не независимой параллельной моделью.
- **ENG-Q066.** Большие деревья используют scoped browsing/lazy loading/filters; функционально не требуется загружать всё дерево установки целиком.
- **ENG-Q067.** Основные способы создания: manual, type/profile/template, duplicate/Create Copies, discovery proposal, import into draft, package-defined structures; Create context-sensitive.
- **ENG-Q068.** При создании требуется как минимум object class/type contract; допустим generic type, но не полностью бесформенная сущность.
- **ENG-Q069.** Display name объекта обязателен.
- **ENG-Q070.** Internal identity уникальна; display name не обязан быть глобально уникальным, локальная naming policy может требовать уникальность в scope.
- **ENG-Q071.** Поддерживается отдельный engineering code/tag как опциональный или policy-required человекочитаемый идентификатор; он не равен internal identity.
- **ENG-Q072.** Engineering code может изменяться как configuration property без смены immutable internal identity.
- **ENG-Q073.** Для сложных templates/profiles доступен preview результата; простой объект не требует обязательного wizard/preview.
- **ENG-Q074.** Простые сущности создаются быстрым flow; wizard применяется только когда действительно нужен последовательный ввод.
- **ENG-Q075.** После Create объект сразу появляется в текущем change set как New/Draft, но не становится active.
- **ENG-Q076.** Пользователь не задаёт internal Dispatcher ID вручную; identity управляется системой.
- **ENG-Q077.** Internal ID доступен в details/diagnostics/copy-reference, но не является главным визуальным именем.
- **ENG-Q078.** Rules/Dashboards/relations и другие устойчивые ссылки используют stable identity; UI показывает человекочитаемое имя.
- **ENG-Q079.** Rename/move объекта не ломает ссылки, пока identity сохраняется.
- **ENG-Q080.** Удалённый internal object ID не переиспользуется автоматически новым объектом.
- **ENG-Q081.** Object hierarchy не обязана означать semantic ownership; semantic relations задаются явно.
- **ENG-Q082.** Объект может иметь несколько functional relations/parents; основной navigation parent может быть один или отсутствовать.
- **ENG-Q083.** Появление объекта в нескольких views означает разные представления одной identity, а не копии.
- **ENG-Q084.** Relationships являются typed; типы описывают предметную семантику вроде feeds/controls/depends on/backup for.
- **ENG-Q085.** Packages могут добавлять relationship types с объявленной семантикой и совместимостью.
- **ENG-Q086.** Relationship type может определять собственные properties, направление и дополнительные атрибуты.
- **ENG-Q087.** Relationship type определяет endpoint constraints; Engineering/validation предотвращают семантически недопустимые связи.
- **ENG-Q088.** Object editor показывает связанные объекты в понятной секции/view с типами и направлением relations.
- **ENG-Q089.** Split functional-position/physical-unit model не обязателен для всех: simple combined object остаётся допустимым.
- **ENG-Q090.** Type/template задаёт разумный default combined/split; инженер применяет split там, где нужна traceability physical unit.
- **ENG-Q091.** Functional position сохраняет identity при замене установленной physical unit.
- **ENG-Q092.** Physical unit сохраняет identity при снятии, ремонте, хранении и повторной установке.
- **ENG-Q093.** При split model Engineering показывает текущую установленную physical unit на functional position.
- **ENG-Q094.** Install/remove physical unit — семантически значимая операция с историей/датами/checks, а не обычное редактирование поля.
- **ENG-Q095.** Validation запрещает одновременно установить одну physical unit в несовместимые mutually-exclusive positions.
- **ENG-Q096.** Observed objects видимы в Engineering с явным признаком source-owned/observed и ограничениями редактирования.
- **ENG-Q097.** Source-owned properties observed object не редактируются как Dispatcher config; редактируются только разрешённые Dispatcher-owned metadata/configuration.
- **ENG-Q098.** Observed object может пройти governed Promote/Manage, добавляя managed configuration без смены identity.
- **ENG-Q099.** Исчезновение observed object не удаляет identity; отображается presence/disappearance state по source contract.
- **ENG-Q100.** Возврат того же raw source ID обрабатывается по namespace/identity/incarnation contract; reused ID не склеивается автоматически.
- **ENG-Q101.** Engineering показывает provenance observed identity: source/namespace/external identity и при необходимости incarnation.
- **ENG-Q102.** Ambiguous merge/rebind/split допускает governed correction с preview влияния и сохранением provenance.
- **ENG-Q103.** Duplicate создаёт новый draft object с новой identity.
- **ENG-Q104.** Copy policy зависит от свойства; identity/runtime/history/physical-unit identity не копируются, а UI показывает состав копирования.
- **ENG-Q105.** Поддерживается полноценный массовый Create Copies.
- **ENG-Q106.** Create Copies поддерживает quantity, deterministic naming/numbering, code/tag rules, placement, typed substitutions, preview и collision detection без произвольного scripting как обязательной основы.
- **ENG-Q107.** Create Copies не публикует результат автоматически; создаются обычные draft entities текущего change set.
- **ENG-Q108.** Удаление объекта требует impact analysis до публикации, включая references/relations/rules/dashboards/commands/history semantics/extensions и другие dependencies.
- **ENG-Q109.** Planned deletion можно отменить обычным revert внутри change set до публикации.
- **ENG-Q110.** После опубликованного удаления active identity прекращается, но необходимая historical/semantic identity сохраняется; повторное создание получает новую identity.

## 7. Decision Register — `ENG-Q111–ENG-Q180`

- **ENG-Q111.** Object Type, Device Profile и Object Template остаются тремя разными сущностями с разными обязанностями.
- **ENG-Q112.** Managed object может иметь Object Type без Device Profile.
- **ENG-Q113.** Device Profile используется только вместе с совместимым semantic Object Type; profile не заменяет object model.
- **ENG-Q114.** Object Template может создавать объект без Device Profile.
- **ENG-Q115.** Object Template может создавать составную структуру из нескольких объектов и typed relations.
- **ENG-Q116.** Object Type — semantic contract: class, properties, parameter/command definitions, relationship/lifecycle capabilities, extensions, validation и допустимые representations.
- **ENG-Q117.** Object Type не является конкретным runtime driver; executable/runtime behaviour поставляется соответствующими contributions/profiles/modules.
- **ENG-Q118.** Есть минимальный системный generic managed-object foundation/base type contract.
- **ENG-Q119.** Организация может создавать собственные Object Types без программирования через governed no-code constructor.
- **ENG-Q120.** Packages могут поставлять Object Types.
- **ENG-Q121.** Происхождение типа (system/package/corporate/local и т.п.) и provenance должны быть видимы.
- **ENG-Q122.** Package-provided Object Type не редактируется пользователем напрямую.
- **ENG-Q123.** Локальное изменение package type выполняется через управляемое extension/derived type или разрешённые organization overrides.
- **ENG-Q124.** Повторное использование type semantics строится на ограниченной управляемой inheritance/composition, а не на произвольной глубокой OO-модели.
- **ENG-Q125.** Предпочтительна небольшая базовая inheritance плюс reusable typed capabilities/components вместо глубоких chains.
- **ENG-Q126.** Type может включать typed reusable semantic capabilities вроде HasTemperature/Controllable/MeteringPoint.
- **ENG-Q127.** UI показывает происхождение inherited/composed property: system/base/capability/local source.
- **ENG-Q128.** Несовместимое переопределение inherited property type запрещено.
- **ENG-Q129.** Derived type может усиливать совместимые constraints, например optional → required.
- **ENG-Q130.** Ослаблять safety-critical/base constraint можно только если базовый contract это явно допускает.
- **ENG-Q131.** Type properties поддерживают категории identity/display metadata, engineering metadata, technical config, source-owned observed facts, annotations, lifecycle и service-extension properties.
- **ENG-Q132.** Каждое type property имеет declared data type.
- **ENG-Q133.** Properties могут иметь typed constraints: enum, min/max, regex, quantity/unit, allowed references, required/optional и другие объявленные ограничения.
- **ENG-Q134.** Property может быть typed reference на другой managed object.
- **ENG-Q135.** Reference property используется для конфигурационного selection/value; typed relationship — когда связь сама имеет предметную семантику и участвует в graph/navigation/impact.
- **ENG-Q136.** Object Type имеет versioned identity/version state.
- **ENG-Q137.** Published изменение type создаёт новую version state.
- **ENG-Q138.** Во время governed migration/update объекты могут временно использовать разные версии одного type, и это состояние явно видно.
- **ENG-Q139.** Версия type доступна в Engineering details/diagnostics объекта.
- **ENG-Q140.** Новая версия type не применяется к существующим объектам скрытно; требуется compatibility/impact process.
- **ENG-Q141.** Для type поддерживается managed update policy: automatic compatible / review / pinned и аналогичные режимы.
- **ENG-Q142.** Dispatcher классифицирует type changes как compatible / conditionally compatible / breaking на основе semantic rules и impact.
- **ENG-Q143.** Удаление используемого property считается breaking change.
- **ENG-Q144.** Смена physical quantity dimension (например температура → давление) считается breaking.
- **ENG-Q145.** Смена units внутри одной physical quantity не обязательно breaking при однозначной governed conversion и совместимых consumers.
- **ENG-Q146.** Изменение enum анализирует существующие значения и consumers.
- **ENG-Q147.** Изменение semantic command signature анализирует Rules/Dashboards/API references и другие consumers.
- **ENG-Q148.** Device Profile — mapping между semantic model Dispatcher и конкретным техническим интерфейсом устройства/adapter.
- **ENG-Q149.** Device Profile может использовать один или несколько adapter capabilities/endpoints, если это осмысленно; он не обязан быть строго one-protocol.
- **ENG-Q150.** Один Object Type может иметь много совместимых Device Profiles.
- **ENG-Q151.** Один Device Profile может применяться к нескольким совместимым Object Types, если semantic contract совместим.
- **ENG-Q152.** Profile объявляет совместимые types/capabilities и Engineering проверяет совместимость выбора.
- **ENG-Q153.** Device Profile может содержать adapter requirements, endpoints/connections, addressing, parameter/command mappings, conversion, quality interpretation, diagnostics, discovery hints и compatibility metadata.
- **ENG-Q154.** Profile может поставлять semantic alarm/event definitions/default recommendations, но site operational alarm policy остаётся managed configuration.
- **ENG-Q155.** Profile может предлагать historization defaults/recommendations, но retention управляется installation policy/configuration.
- **ENG-Q156.** Profile может задавать default/recommended polling rates, ограниченные connection/runtime policy и capacity validation.
- **ENG-Q157.** Обычный Engineering UX может скрывать raw registers/technical addresses; они доступны в profile/advanced diagnostics, semantic layer остаётся основным.
- **ENG-Q158.** Организация может создавать no-code Device Profiles для поддерживаемых adapters/protocol mapping capabilities.
- **ENG-Q159.** Новый произвольный protocol может требовать code contribution; no-code profile не заменяет driver development.
- **ENG-Q160.** Package-provided Device Profile не редактируется напрямую.
- **ENG-Q161.** Поддерживается Clone as local profile.
- **ENG-Q162.** Локальный clone сохраняет provenance исходного profile.
- **ENG-Q163.** Device Profile имеет version.
- **ENG-Q164.** Profile может объявлять hardware/firmware compatibility range.
- **ENG-Q165.** Discovery учитывает profile compatibility и предлагает подходящие profiles с confidence/reason.
- **ENG-Q166.** При неизвестной firmware profile может быть назначен только если его policy это допускает, с явным compatibility state/warning.
- **ENG-Q167.** Profile update меняет mappings существующих устройств только через governed compatibility/impact/update process.
- **ENG-Q168.** Object Template — reusable engineering recipe/configuration pattern для создания и поддержания одной или нескольких сущностей.
- **ENG-Q169.** Template может содержать конкретные defaults: names, alarm thresholds, connection placeholders, historization, placements, relations и иные managed settings.
- **ENG-Q170.** Template поддерживает typed input parameters, а не только текстовую подстановку.
- **ENG-Q171.** Template может включать nested templates с контролируемыми dependency/version semantics.
- **ENG-Q172.** Circular template dependencies запрещены validation.
- **ENG-Q173.** Template может создавать locations, когда они являются частью reusable engineering structure.
- **ENG-Q174.** Template может создавать/configure connection instances или placeholders, если это предусмотрено contract.
- **ENG-Q175.** Template может создавать Rules/alarm/dashboard bindings через типизированные contributions/links при наличии соответствующих capabilities.
- **ENG-Q176.** После создания instance связь с template сохраняется по умолчанию как managed template linkage.
- **ENG-Q177.** При создании можно выбрать linked или detached copy, если template policy допускает detached usage.
- **ENG-Q178.** Instance хранит identity/version template, из которой произошло его managed состояние.
- **ENG-Q179.** Template instance может иметь local overrides только в разрешённых/осмысленных местах.
- **ENG-Q180.** UI явно отличает inherited/template value от local override и показывает источник effective value.

## 8. Decision Register — `ENG-Q181–ENG-Q250`

- **ENG-Q181.** Опубликованная версия template не редактируется напрямую; изменение создаётся как новая draft/version.
- **ENG-Q182.** Новая версия template не меняет существующие instances сразу; сначала формируется update proposal/impact.
- **ENG-Q183.** Перед публикацией новой версии Engineering показывает количество linked instances и affected scopes.
- **ENG-Q184.** Engineering показывает structural/semantic diff того, какие изменения получит instance.
- **ENG-Q185.** Publication новой template version и propagation/update существующих instances — разные действия.
- **ENG-Q186.** Часть instances может явно и управляемо оставаться на старой template version.
- **ENG-Q187.** Engineering показывает distribution template versions по linked instances.
- **ENG-Q188.** Linked instance имеет явный статус Update available, когда применимо.
- **ENG-Q189.** Template поддерживает governed update policy: auto-compatible/review/pinned и аналогичные режимы.
- **ENG-Q190.** Даже auto-compatible propagation не обходит change set/governed lifecycle; автоматизация может сформировать managed change.
- **ENG-Q191.** Compatibility определяется совместно template metadata, platform compatibility rules и фактическим impact конкретных instances.
- **ENG-Q192.** Organization policy может запретить automatic propagation независимо от recommendation template.
- **ENG-Q193.** Critical/safety-related scope может требовать review даже для compatible update.
- **ENG-Q194.** Автоматически созданный template-update change set видим как обычный change set с явным origin.
- **ENG-Q195.** Local override сохраняет связь/исходное inherited value для сравнения и updates.
- **ENG-Q196.** Если template меняет locally overridden property, local effective value сохраняется, но UI показывает divergence/new inherited value и возможный conflict.
- **ENG-Q197.** Инженер может Reset to template, возвращая property к inherited state.
- **ENG-Q198.** Reset to template показывает resulting effective value до применения.
- **ENG-Q199.** Template может запрещать override конкретного property, если это часть управляемого semantic contract.
- **ENG-Q200.** Различаются configurable per-instance, override allowed, fixed by template и computed/derived settings.
- **ENG-Q201.** Organization policy может дополнительно ограничивать local overrides.
- **ENG-Q202.** UI позволяет просмотреть все local overrides instance одним списком.
- **ENG-Q203.** Новую template version можно применить только к выбранной части instances.
- **ENG-Q204.** Selection для propagation поддерживает explicit selection, filters/query, location, current version, type/profile, tags/groups и compatibility state.
- **ENG-Q205.** Массовый propagation обязательно имеет preview.
- **ENG-Q206.** Preview показывает aggregate summary и per-instance exceptions/conflicts с drill-down, а не только count и не обязан раскрывать полный diff всех объектов сразу.
- **ENG-Q207.** Отдельные instances можно исключить из массового update.
- **ENG-Q208.** Причина manual exclusion опциональна в обычном scope и обязательна, когда это требует policy/critical scope.
- **ENG-Q209.** Selective update обязан масштабироваться на тысячи objects/instances.
- **ENG-Q210.** UI не обязан рендерить diff всех тысяч instances одновременно; используется aggregated/virtualized drill-down.
- **ENG-Q211.** Template conflicts включают incompatible override, removed referenced child, type/profile incompatibility, relationship/naming collision, changed required input и dependency/package mismatch.
- **ENG-Q212.** Unaffected instances могут обновляться отдельно от конфликтных, если consistency/risk policy допускает разделение scopes.
- **ENG-Q213.** Template conflicts классифицируются как blocking / resolvable / informational divergence или эквивалентно по смыслу.
- **ENG-Q214.** Conflict resolution может задаваться массовым правилом для выбранной группы instances.
- **ENG-Q215.** Результат массового conflict resolution предварительно показывается перед применением.
- **ENG-Q216.** Instance с unresolved conflict может оставаться на старой version как явно pending/diverged; конфликт не разрешается скрытно.
- **ENG-Q217.** Значения template inputs сохраняются как часть instance provenance.
- **ENG-Q218.** Mutable template input можно менять; зависимая generated configuration пересчитывается через preview.
- **ENG-Q219.** Template input может быть immutable после создания, если изменение разрушает смысл instance/structure.
- **ENG-Q220.** Изменение input, создающее/удаляющее child entities, требует impact analysis.
- **ENG-Q221.** Template/editor показывает explainability dependency input → generated values/entities.
- **ENG-Q222.** Parent template pin'ит конкретную child version либо использует declared compatible range/update policy; latest не подставляется скрытно.
- **ENG-Q223.** Изменение child template не создаёт автоматически новую parent version; parent dependency update — отдельное managed изменение.
- **ENG-Q224.** Engineering показывает dependency graph templates.
- **ENG-Q225.** Impact child-template update включает transitively affected parent templates и instances.
- **ENG-Q226.** Parent template может переопределять child settings только через declared interface/inputs/override points.
- **ENG-Q227.** Internal details child template могут быть скрыты от parent; взаимодействие идёт через contract.
- **ENG-Q228.** Detach from template превращает effective configuration в обычную local managed configuration, сохраняя object identity и provenance происхождения.
- **ENG-Q229.** Detach является governed configuration change.
- **ENG-Q230.** Перед detach выполняется impact analysis, особенно для compound/nested dependencies.
- **ENG-Q231.** После detach historical provenance прежнего template сохраняется.
- **ENG-Q232.** Detached instance больше не получает template updates.
- **ENG-Q233.** Поддерживается governed reattach/adopt существующего объекта к template.
- **ENG-Q234.** Структурное сходство не является основанием для автоматического reattach; требуется explicit matching/preview.
- **ENG-Q235.** При reattach local properties проходят явный mapping: inherited / retained override / conflict / discarded с review.
- **ENG-Q236.** Поддерживается массовый adopt существующих однотипных объектов под новый template с preview/compatibility analysis.
- **ENG-Q237.** Template нельзя бесследно удалить при наличии linked instances/dependencies без explicit resolution.
- **ENG-Q238.** Resolution template deletion: оставить template, detach instances, migrate to replacement template либо удалить после устранения dependencies.
- **ENG-Q239.** Package uninstall может удалить executable/template contribution, но historical semantic descriptor/provenance сохраняется там, где требуется Product Concept.
- **ENG-Q240.** Device Profile существующего managed object можно заменить без смены object identity, если semantic Object Type совместим и migration проходит validation.
- **ENG-Q241.** Profile replacement показывает mapping старых и новых parameters/commands.
- **ENG-Q242.** Исчезнувший в новом profile semantic parameter создаёт explicit migration issue: remove/unbind/substitute/map плюс impact consumers; не удаляется тихо.
- **ENG-Q243.** Новые semantic parameters нового profile добавляются согласно type/profile/template policy с preview.
- **ENG-Q244.** Поддерживается массовая замена Device Profile на группе совместимых устройств.
- **ENG-Q245.** Mass profile replacement поддерживает parameterized mapping rules, например address offsets/firmware-dependent mapping.
- **ENG-Q246.** Object Type существующего объекта можно изменить без смены identity только через explicit type migration при подтверждённой semantic continuity.
- **ENG-Q247.** Type migration имеет explicit field/parameter/command/relationship mapping.
- **ENG-Q248.** Type migration может создавать/удалять child entities/extensions при полном preview/impact.
- **ENG-Q249.** Если semantic identity реального объекта меняется, это replacement/new object lifecycle, а не маскировка под type migration.
- **ENG-Q250.** Engineering сохраняет provenance всей цепочки type/profile/template migrations: from/to version, mapping, actor/origin, time, change set/publication и result.

## 9. Decision Register — `ENG-Q251–ENG-Q340`

### 9.1 Сущности Adapter / Connection / Endpoint

- **ENG-Q251.** Adapter, Connection и Endpoint являются разными понятиями: Adapter = software capability/contribution, Connection = managed configuration канала/сеанса, Endpoint = адресуемая сторона/точка взаимодействия.
- **ENG-Q252.** Adapter не является managed configuration entity; это installed software contribution, а его использование и настройки входят в managed configuration.
- **ENG-Q253.** Connection является managed configuration entity.
- **ENG-Q254.** Endpoint является отдельной managed entity только при самостоятельной reuse/identity/configuration семантике; в простом случае может быть частью Connection.
- **ENG-Q255.** Connection может существовать без привязанного managed object как reusable shared infrastructure connection.
- **ENG-Q256.** Managed object может существовать без Connection.

### 9.2 Registry, navigation и создание Connection

- **ENG-Q257.** Engineering имеет отдельный Connections registry плюс contextual access из объектов.
- **ENG-Q258.** Object editor показывает связанные Connections.
- **ENG-Q259.** Connection показывает все objects/profiles, которые его используют.
- **ENG-Q260.** Из object editor можно перейти к Connection editor без потери change-set context.
- **ENG-Q261.** Из Connection editor можно перейти к affected objects/endpoints.
- **ENG-Q262.** При создании Connection сначала выбирается Adapter/connection kind, затем UI строится по его typed schema.
- **ENG-Q263.** Device Profile может предложить создание подходящего Connection как convenience workflow, но создаётся обычная Connection entity.
- **ENG-Q264.** Connection, созданный Object Template, остаётся обычной managed entity и сохраняет template provenance.
- **ENG-Q265.** Connection имеет понятное display name; оно может быть сгенерировано автоматически, но должно быть устойчиво отображаемым.
- **ENG-Q266.** Connection engineering code является optional/policy-required identifier отдельно от stable identity.

### 9.3 Adapter selection и typed Connection schema

- **ENG-Q267.** Список Adapters фильтруется по installed/allowed contributions.
- **ENG-Q268.** Engineering показывает provenance Adapter: package, version, trust level, vendor/author там, где доступно.
- **ENG-Q269.** До выбора Adapter UI показывает его ключевые capabilities: read/write/subscription/discovery/timestamps/diagnostics/Edge/security и другие declared capabilities.
- **ENG-Q270.** Organization policy может запрещать конкретный Adapter.
- **ENG-Q271.** Validation блокирует или помечает несовместимость Adapter с target execution node/platform/capabilities; runtime failure не используется как основной механизм проверки.
- **ENG-Q272.** Adapter предоставляет typed schema настроек Connection.
- **ENG-Q273.** Штатный Connection editor строится на typed schema/common primitives; произвольный HTML/JS editor допустим только как явно объявленная специализированная contribution.
- **ENG-Q274.** Connection schema fields имеют type, required/constraints/help, sensitivity и apply/restart/compatibility metadata где применимо.
- **ENG-Q275.** Package/Adapter может добавлять protocol-specific validation.
- **ENG-Q276.** Organization может накладывать дополнительные policy-validation rules поверх Adapter schema.

### 9.4 Endpoint, transport и shared buses

- **ENG-Q277.** Endpoint — typed addressable target/resource side, формат и semantics которого задаёт Adapter; он не ограничен IP:port.
- **ENG-Q278.** Один Connection может обслуживать несколько Endpoints, если protocol/session model это допускает.
- **ENG-Q279.** Один Endpoint может использоваться несколькими Connections, если это семантически допустимо и policy разрешает.
- **ENG-Q280.** Endpoint получает stable identity, если используется несколькими Connections/objects или имеет самостоятельную lifecycle/reuse семантику.
- **ENG-Q281.** Simple single-device Connection может хранить endpoint inline и не заставляет пользователя создавать отдельную именованную Endpoint entity.
- **ENG-Q282.** Semantic device settings и transport settings разделяются.
- **ENG-Q283.** Transport settings принадлежат Connection/Endpoint, а не Object Type.
- **ENG-Q284.** Device Profile может предоставлять default/recommended transport settings, но не immutable site-specific значения.
- **ENG-Q285.** Engineering показывает effective source connection setting: Adapter default / Profile / Template / local override и другие объявленные слои.
- **ENG-Q286.** Shared serial/physical bus моделируется как общий transport Connection/channel с несколькими logical device endpoints.
- **ENG-Q287.** Engineering предотвращает конфликтующие serial/physical transport settings на одном shared port/resource.
- **ENG-Q288.** Validation выявляет duplicate slave/node address внутри соответствующего bus scope.
- **ENG-Q289.** Разные device profiles на одном bus могут иметь разные polling policies, но общий scheduler/capacity validation учитывает shared resource.
- **ENG-Q290.** До Publish Engineering показывает aggregate load shared bus/connection и предупреждает/блокирует заведомо несостоятельную конфигурацию по policy.

### 9.5 Local I/O, secrets, certificates

- **ENG-Q291.** Local DI/DO/AI/AO используют тот же Adapter/Connection/Profile foundation через hardware adapters, а не отдельную объектную подсистему.
- **ENG-Q292.** Raw I/O channel доступен как technical binding; operational object остаётся semantic сущностью и не сводится к DO7/AI2.
- **ENG-Q293.** Один raw input может питать несколько semantic Parameters, если ownership/semantics не конфликтуют.
- **ENG-Q294.** Один actuator output не может иметь скрытое независимое управление несколькими semantic owners; multiple control допускается только через explicit arbitration/control semantics.
- **ENG-Q295.** Secret values не хранятся как обычный текст внутри Connection; Connection хранит secret reference.
- **ENG-Q296.** Право редактировать Connection не даёт автоматически права раскрывать existing secret; use/replace/reveal разделяются.
- **ENG-Q297.** UI показывает факт наличия configured secret без раскрытия его значения.
- **ENG-Q298.** Connection UX различает choose existing secret, create secret, replace/rotate и remove reference.
- **ENG-Q299.** Изменение secret value является отдельной secret operation; durable secret reference/policy может входить в managed configuration.
- **ENG-Q300.** Connection editor показывает разрешённую пользователю secret metadata: reference/type/scope/expiry-health/rotation и т.п. без reveal value.
- **ENG-Q301.** TLS/client certificate/private-key используют managed secret/certificate references.
- **ENG-Q302.** Certificate можно добавить из Connection flow как convenience action, но certificate/trust entity создаётся в соответствующем manager, а Connection получает reference.
- **ENG-Q303.** Validation проверяет expiration/trust/compatibility certificate references.
- **ENG-Q304.** Истёкший certificate не обязательно универсально блокирует Publish; severity определяется фактической работоспособностью и security policy, но проблема всегда явна.

### 9.6 Connection test, runtime state и enable/disable

- **ENG-Q305.** Engineering предоставляет Test Connection до Publish.
- **ENG-Q306.** Test Connection не является publication; это diagnostic/administrative test operation над draft settings.
- **ENG-Q307.** Connection Test использует draft values без активации их как operational configuration.
- **ENG-Q308.** Connection Test по умолчанию non-invasive; потенциально изменяющие оборудование capabilities проходят отдельный diagnostic Command Model/policy.
- **ENG-Q309.** Test Connection возвращает reachability, authentication, protocol handshake, identification где возможно, latency/basic diagnostics, used adapter/node и warnings.
- **ENG-Q310.** Significant Connection Tests имеют audit/diagnostic trace; последний результат может показываться в UI, но не считается постоянной operational truth.
- **ENG-Q311.** Configured и Connected — разные состояния.
- **ENG-Q312.** Connection editor раздельно показывает desired configuration, deployed/active version и current runtime state.
- **ENG-Q313.** Базовые runtime states Connection включают not deployed, disabled, starting, healthy/connected, degraded, disconnected, auth failed, config error, missing/unsupported adapter, executor unavailable, uncertain/stale; Adapter может уточнять reason.
- **ENG-Q314.** Adapter может добавлять protocol-specific diagnostic reasons к общей runtime state model.
- **ENG-Q315.** UI показывает время последнего успешного обмена Connection.
- **ENG-Q316.** UI показывает last error и timestamp с нормализованной категорией и технической детализацией по правам.
- **ENG-Q317.** Durable Enabled/Disabled является managed configuration property Connection.
- **ENG-Q318.** Обычное долговременное отключение Connection проходит change set/publish как изменение desired configuration.
- **ENG-Q319.** Для немедленного protective disable существует отдельный administrative protective override, не маскируемый изменением durable config.
- **ENG-Q320.** UI одновременно показывает desired enabled, actual disabled и active protective override, если эти состояния расходятся.

### 9.7 Execution placement, authority, handover и multiple Connections

- **ENG-Q321.** Connection имеет desired execution placement явно или через placement policy/group/profile.
- **ENG-Q322.** Desired executor/placement и current actual executor являются разными понятиями.
- **ENG-Q323.** Допускается explicit assignment Connection на конкретный execution node.
- **ENG-Q324.** Поддерживается assignment через node group/profile/policy для массовых Edge deployments.
- **ENG-Q325.** Engineering показывает причину effective placement: explicit assignment, group policy, compatibility, recovery state и т.п.
- **ENG-Q326.** Validation/diagnostics учитывают network/resource reachability target node; placement должен быть совместим с фактическим доступом к ресурсу.
- **ENG-Q327.** Device Profile/Adapter объявляют Edge execution capability.
- **ENG-Q328.** Desired configuration может ссылаться на Adapter, ещё не deployed на target node, только при явном deployment prerequisite/plan; activation блокируется до готовности.
- **ENG-Q329.** Operational Connection может быть multi-active только если semantics явно допускают безопасную read-only/redundant model; actuator-capable contour имеет одного authoritative executor.
- **ENG-Q330.** Read-only telemetry может использовать redundant/multi-source acquisition при явной identity/dedupe/active-source semantics.
- **ENG-Q331.** Actuator-capable Connection явно обозначает наличие managed actuator authority/capability.
- **ENG-Q332.** Generic network capability не даёт actuator authority автоматически; managed actuator execution — отдельная capability, связанная с Command Model.
- **ENG-Q333.** Изменение desired placement в config не равно фактическому authority handover; handover имеет отдельный runtime lifecycle.
- **ENG-Q334.** Engineering показывает handover states: current, target, pending, completed, blocked, uncertain.
- **ENG-Q335.** Недоступность старого executor по сети сама по себе не доказывает потерю actuator authority.
- **ENG-Q336.** Normal handover actuator connection блокируется, если нельзя доказать, что старый executor больше не может воздействовать на actuator.
- **ENG-Q337.** Emergency authority transfer существует как отдельная усиленная recovery/admin operation с отдельным правом, reason, warning и audit.
- **ENG-Q338.** Один managed object может иметь несколько Connections.
- **ENG-Q339.** Роль Connection относительно объекта типизируется: primary/backup telemetry, command execution, diagnostics, auxiliary/service и другие declared roles.
- **ENG-Q340.** При нескольких Connections/sources active-source/active-executor selection задаётся явной policy, а не правилом «первая доступная».

## 10. Decision Register — `ENG-Q341–ENG-Q790`

### 10.1 Parameter identity, definition и source classes

- **ENG-Q341.** Parameter является самостоятельной semantic entity с устойчивой identity, а не просто полем объекта.
- **ENG-Q342.** Parameter имеет одного primary semantic owner — managed object; другие сущности могут ссылаться на него.
- **ENG-Q343.** Parameter может существовать без внешнего physical source: calculated, aggregate, manual, service-derived и другие варианты.
- **ENG-Q344.** Parameter definition и runtime value являются разными слоями.
- **ENG-Q345.** Parameter definition входит в managed configuration.
- **ENG-Q346.** Runtime value является operational/runtime fact, а не managed configuration.
- **ENG-Q347.** Parameter имеет display name.
- **ENG-Q348.** Parameter имеет stable semantic key/code внутри соответствующего semantic contract отдельно от display name и internal identity.
- **ENG-Q349.** Изменение display name не ломает Rules/Dashboards/history, поскольку ссылки используют stable identity/contract.
- **ENG-Q350.** Удалённая Parameter identity не переиспользуется автоматически.
- **ENG-Q351.** Engineering показывает происхождение Parameter как acquired/calculated/aggregate/manual/service-derived/observed-source-owned и аналогичные declared classes.
- **ENG-Q352.** Разные source classes используют общий Parameter contract, а не независимые несовместимые сущности.
- **ENG-Q353.** Source class Parameter можно изменить через governed reconfiguration без смены identity, если semantic meaning сохраняется.
- **ENG-Q354.** При реальном изменении semantic meaning старую identity нельзя сохранять скрытно; нужен новый Parameter или explicit semantic migration.

### 10.2 Data types, quantities и units

- **ENG-Q355.** Parameter имеет declared data type.
- **ENG-Q356.** Базовые категории типов включают Boolean, signed/unsigned integer, floating/decimal, string, enum, timestamp/date/time, duration, structured, arrays/sequences и reference/identifier где contract допускает; concrete storage types определяет architecture.
- **ENG-Q357.** Boolean и двухсостоянийный enum различаются семантически.
- **ENG-Q358.** Enum members имеют stable machine codes отдельно от локализуемых/display labels.
- **ENG-Q359.** Enum label можно менять без изменения stored semantic value/code.
- **ENG-Q360.** Удаление enum member не уничтожает его semantic descriptor, если значение присутствует в истории.
- **ENG-Q361.** Structured Parameter имеет declared schema.
- **ENG-Q362.** Arbitrary JSON не является штатной заменой typed structured Parameter contract.
- **ENG-Q363.** Packages могут определять специализированные structured schemas.
- **ENG-Q364.** Для physical numeric Parameter machine semantics отдельно включают physical quantity и unit/display unit.
- **ENG-Q365.** Unit — semantic entity с conversion rules, а не просто строковая подпись.
- **ENG-Q366.** Dispatcher имеет общий registry физических quantities/units.
- **ENG-Q367.** Packages могут добавлять domain-specific quantities/units с declared semantics.
- **ENG-Q368.** Organization может создавать governed custom unit только с определённой dimension/conversion semantics, а не произвольную строку.
- **ENG-Q369.** Conversion между несовместимыми physical quantities запрещается validation.
- **ENG-Q370.** Canonical/storage representation может отличаться от display unit; физический смысл сохраняется.
- **ENG-Q371.** Пользователь может выбирать preferred compatible display unit там, где operational policy это допускает, без изменения canonical semantics.
- **ENG-Q372.** Dashboard/representation может выбирать совместимую display unit без изменения Parameter identity/semantics.
- **ENG-Q373.** Изменение display unit не меняет физический смысл alarms/limits; значения конвертируются совместимо.

### 10.3 Source binding и acquisition policy

- **ENG-Q374.** Source binding отделяется от semantic Parameter definition как отдельный configuration layer.
- **ENG-Q375.** Acquired Parameter source binding ссылается на Connection/Adapter technical resource по typed contract.
- **ENG-Q376.** Device Profile может предоставлять source-binding template/default mapping.
- **ENG-Q377.** Instance может override source mapping, если profile/template policy разрешает; override остаётся видимым.
- **ENG-Q378.** UI показывает semantic Parameter и raw technical binding отдельно.
- **ENG-Q379.** Raw mapping описывается typed Adapter schema, а не универсальными протокольными полями платформы.
- **ENG-Q380.** Один raw source value может питать несколько semantic Parameters, например декодирование status word.
- **ENG-Q381.** Несколько raw fragments могут формировать один semantic Parameter, например multi-register value.
- **ENG-Q382.** Сборку/декодирование raw fragments определяет Adapter/Profile binding contract, а не Dashboard/Historian.
- **ENG-Q383.** Protocol-specific settings вроде endianness/word order показываются только там, где Adapter contract их поддерживает.
- **ENG-Q384.** Polling, subscription и event-driven acquisition используют одну semantic Parameter model.
- **ENG-Q385.** Пользователь не обязан всегда выбирать transport mechanism на уровне Parameter; Profile/Adapter задаёт default mechanism, а override доступен только по contract.
- **ENG-Q386.** Polling interval является managed configuration там, где применяется polling.
- **ENG-Q387.** Polling policy задаётся иерархически через defaults/profile/connection/group/parameter override где разрешено.
- **ENG-Q388.** Engineering показывает effective polling interval и источник policy.
- **ENG-Q389.** Adapter может оптимизировать несколько logical reads в один protocol operation при сохранении semantic behaviour Parameters.
- **ENG-Q390.** Ручная packet-level оптимизация contiguous reads не является основным workflow; Adapter/Profile автоматизирует её, advanced tuning допускается.
- **ENG-Q391.** Validation анализирует acquisition rate/capacity.
- **ENG-Q392.** Capacity analysis учитывает shared bus/connection load.
- **ENG-Q393.** Severity возможного capacity exceed зависит от уверенности расчёта, runtime class и policy; не существует универсального только-warning или только-block поведения.

### 10.4 Timestamps, provenance и quality

- **ENG-Q394.** Operational sample может иметь несколько timestamps: source, receive/ingest и при необходимости corrected/normalized representation.
- **ENG-Q395.** Source timestamp нельзя скрытно заменять server/receive time.
- **ENG-Q396.** При отсутствии source timestamp receive time может использоваться для ordering/presentation, но provenance явно показывает отсутствие source time.
- **ENG-Q397.** Runtime value имеет time-quality information.
- **ENG-Q398.** Time quality хранится/моделируется отдельно от value quality.
- **ENG-Q399.** Engineering/diagnostics показывают timestamp provenance; operator UI раскрывает её по необходимости.
- **ENG-Q400.** Known clock offset может учитываться в corrected presentation/processing, но original timestamp и correction provenance сохраняются.
- **ENG-Q401.** Time uncertainty учитывается при ordering/correlation, когда влияет на достоверность.
- **ENG-Q402.** Каждый runtime value имеет provenance/source identity достаточную для трассировки.
- **ENG-Q403.** Derived value provenance включает calculation/version и inputs на достаточном уровне traceability.
- **ENG-Q404.** Manual substitution provenance явно обозначает manual source/exception.
- **ENG-Q405.** При source switching provenance каждого effective sample указывает фактически использованный source.
- **ENG-Q406.** Historical sample связан со stable semantic Parameter identity независимо от текущего display name.
- **ENG-Q407.** Quality является частью operational value semantics.
- **ENG-Q408.** Good/Bad недостаточно; нужна common quality category model плюс detailed reason.
- **ENG-Q409.** Базовые quality categories общие для protocols/adapters.
- **ENG-Q410.** Adapter может добавлять protocol-specific detailed quality reasons.
- **ENG-Q411.** Uncertain отличается от Bad как самостоятельная общая quality category/state.
- **ENG-Q412.** Connection state моделируется отдельно; Parameter quality может отражать consequence communication loss, но эти понятия не сливаются.
- **ENG-Q413.** Quality, freshness и connection state моделируются раздельно; effective operational presentation учитывает их совместно по policy.
- **ENG-Q414.** UI предоставляет detailed quality reason по запросу/diagnostic drill-down.
- **ENG-Q415.** Protocol-specific quality нормализуется в common quality model.
- **ENG-Q416.** Исходные protocol quality/details сохраняются для diagnostics/provenance где доступны.
- **ENG-Q417.** Calculated Parameter использует explicit quality-propagation policy с безопасными defaults, а не безусловное правило «любой bad input = bad output» для всех calculations.

### 10.5 Freshness, normalization, calibration и limits

- **ENG-Q418.** Freshness является отдельной характеристикой от quality.
- **ENG-Q419.** Freshness/stale threshold configurable.
- **ENG-Q420.** Freshness policy задаётся иерархически: platform/type/profile/template/connection/parameter override где применимо.
- **ENG-Q421.** Engineering показывает effective freshness threshold и источник policy.
- **ENG-Q422.** При отсутствии новых samples last known value не удаляется; сохраняются value, age, freshness/quality indication.
- **ENG-Q423.** No value ever и stale last-known value различаются.
- **ENG-Q424.** Stale и source explicitly reported invalid различаются.
- **ENG-Q425.** Freshness threshold может зависеть от operating mode, если это явно смоделировано managed policy.
- **ENG-Q426.** Adapter/source contract может сообщать expected update interval.
- **ENG-Q427.** Validation предупреждает/блокирует policy, если freshness threshold физически несовместим с acquisition interval/capacity.
- **ENG-Q428.** Raw value и normalized engineering value являются разными уровнями там, где действует conversion.
- **ENG-Q429.** Scaling/normalization задаётся в Profile/binding/Parameter normalization configuration, а не в Dashboard/Historian query.
- **ENG-Q430.** Linear scaling поддерживается как базовый conversion mechanism.
- **ENG-Q431.** Кроме linear scaling допускаются typed conversion functions/tables/modules с declared semantics.
- **ENG-Q432.** Arbitrary per-Parameter script не является основным scaling mechanism; предпочтительны reusable governed conversions, script extension — специальный случай.
- **ENG-Q433.** Conversion/scaling имеет version/provenance.
- **ENG-Q434.** Изменение scaling анализирует historical continuity/consumer impact.
- **ENG-Q435.** Изменение scaling не переписывает старую history автоматически; historical facts сохраняют original semantics/version, correction/recalculation выполняется отдельно.
- **ENG-Q436.** Engineering предоставляет preview raw→engineering conversion при настройке.
- **ENG-Q437.** Calibration и generic scaling различаются: calibration может иметь собственную lifecycle/provenance/physical-unit semantics, хотя участвует в normalization chain.
- **ENG-Q438.** Calibration data может принадлежать конкретной physical unit.
- **ENG-Q439.** При замене physical sensor calibration конкретного экземпляра следует за physical unit, а не за functional position.
- **ENG-Q440.** Functional position может иметь отдельную site correction поверх unit calibration с самостоятельным provenance layer.
- **ENG-Q441.** Engineering показывает effective correction/calibration chain.
- **ENG-Q442.** Physical/source range и operating range различаются.
- **ENG-Q443.** Normalization range хранится отдельно там, где применимо.
- **ENG-Q444.** Alarm limits не равны operating limits.
- **ENG-Q445.** Command limits не равны alarm limits.
- **ENG-Q446.** Recommended range — отдельная semantic category.
- **ENG-Q447.** UI различает source/physical, normalization, operating, recommended, command и alarm limits.
- **ENG-Q448.** Нельзя сводить все диапазоны к generic Min/Max без потери semantics.
- **ENG-Q449.** Type/Profile могут предоставлять default ranges.
- **ENG-Q450.** Instance может override разрешённые ranges согласно policy/template contract.
- **ENG-Q451.** Изменение ranges участвует в impact analysis, если consumers используют соответствующий semantic contract.

### 10.6 Historization, late data, gaps и deadbands

- **ENG-Q452.** Historization является свойством Parameter/data policy, а не Dashboard/visualization.
- **ENG-Q453.** Не все Parameters обязаны историзироваться; historization определяется governed policy.
- **ENG-Q454.** Type/Profile/Template могут задавать historization defaults.
- **ENG-Q455.** Organization/site policy может управляемо переопределять historization/retention defaults согласно правам и policy.
- **ENG-Q456.** Engineering показывает effective historization policy Parameter и источник её значений.
- **ENG-Q457.** Acquisition rate и historian recording policy являются разными configuration dimensions.
- **ENG-Q458.** Historian может записывать каждый acquired sample как одна из policies.
- **ENG-Q459.** Historian может использовать change-only/deadband recording policy.
- **ENG-Q460.** Historian может хранить typed aggregates дополнительно к raw history.
- **ENG-Q461.** Source sample и stored historical fact различаются как semantics/provenance layers.
- **ENG-Q462.** Historical facts сохраняют quality.
- **ENG-Q463.** Historical facts сохраняют source/provenance в объёме, достаточном для configured fidelity и traceability.
- **ENG-Q464.** Time quality/provenance сохраняется там, где она значима для интерпретации истории.
- **ENG-Q465.** Late sample может быть вставлен в historical timeline с исходным event/source time и provenance.
- **ENG-Q466.** Late arrival не является historical correction; это отдельная семантика.
- **ENG-Q467.** Historical gaps должны быть явными.
- **ENG-Q468.** Gap нельзя скрытно заполнять last-known value и выдавать за source measurement; interpolation допустима только как presentation/declared derived semantics.
- **ENG-Q469.** Visualization interpolation и stored backfill являются разными действиями/semantics.
- **ENG-Q470.** Acquisition/source deadband и historian recording deadband различаются.
- **ENG-Q471.** Source-side deadband может уменьшать incoming samples, если source/protocol это поддерживает.
- **ENG-Q472.** Historian deadband может уменьшать stored samples, сохраняя отдельную runtime stream semantics.
- **ENG-Q473.** Engineering показывает, на каком уровне действует deadband.
- **ENG-Q474.** Alarm evaluation по умолчанию не использует historian-filtered stream; historian optimization не меняет operational alarm semantics скрытно.
- **ENG-Q475.** Rule input по умолчанию не зависит от historian deadband; input stream определяется Rule/Parameter contract.

### 10.7 Multiple sources и manual substitution

- **ENG-Q476.** Один semantic Parameter может иметь несколько configured sources.
- **ENG-Q477.** Каждый configured source имеет самостоятельную identity/provenance.
- **ENG-Q478.** Source identities сохраняются независимо от active selection; historization каждого source configurable, полное хранение всех sources не универсально обязательно.
- **ENG-Q479.** Active/effective source value является отдельной representation поверх source-specific values.
- **ENG-Q480.** Active source выбирается explicit source-selection policy, а не «первый доступный/последний sample».
- **ENG-Q481.** Source-selection policy может учитывать quality.
- **ENG-Q482.** Source-selection policy может учитывать freshness.
- **ENG-Q483.** Source-selection policy может учитывать connection/executor state.
- **ENG-Q484.** Source-selection policy может учитывать explicit priority.
- **ENG-Q485.** Source-selection policy может учитывать operating mode, если это managed explicit policy.
- **ENG-Q486.** Source selection поддерживает anti-flapping semantics — hold/hysteresis/recovery policy где требуется.
- **ENG-Q487.** Source switch не меняет Parameter identity.
- **ENG-Q488.** Operator/engineer может увидеть текущий active source.
- **ENG-Q489.** Historian/effective history сохраняет provenance того source, который сформировал effective value.
- **ENG-Q490.** Manual forced source selection допускается как governed operational/configuration override в зависимости от semantics/duration и сохраняет provenance.
- **ENG-Q491.** Manual substitution не изменяет original source value/fact; она создаёт отдельный substitution/effective-value layer.
- **ENG-Q492.** Manual substitution — operational exception, а не обычное редактирование Parameter definition.
- **ENG-Q493.** Manual substitution имеет reason.
- **ENG-Q494.** Manual substitution имеет initiator и owner/authority.
- **ENG-Q495.** Manual substitution имеет start time.
- **ENG-Q496.** Planned end не обязательно универсально обязателен, но policy может его требовать; indefinite substitution должна быть явно видимой/reviewable.
- **ENG-Q497.** Substitution lifecycle поддерживает absolute time, duration, explicit removal и condition-end semantics согласно operational-exception foundation.
- **ENG-Q498.** При degraded time quality автоматическое завершение substitution определяется safety policy и не должно переводить систему в потенциально менее безопасное состояние скрытно.
- **ENG-Q499.** Source acquisition по умолчанию продолжается во время substitution, если отдельная policy не требует остановки.
- **ENG-Q500.** UI может одновременно показать original/source value и substituted effective value.
- **ENG-Q501.** Substitution имеет собственную quality/provenance semantics.
- **ENG-Q502.** Rules получают явно определённый semantic layer (raw/source/effective) по contract; это не угадывается скрытно.
- **ENG-Q503.** Alarm evaluation contract явно определяет, использует он source/raw или effective value и как учитывает substitution.
- **ENG-Q504.** Historian может по policy сохранять source и effective substituted series раздельно/совместно.
- **ENG-Q505.** Historical views могут показывать intervals действия substitution.
- **ENG-Q506.** Substitution, влияющая на operational meaning, не может быть скрыта от соответствующего оператора.

### 10.8 Calculated и aggregate Parameters

- **ENG-Q507.** Calculated Parameter использует общий semantic Parameter contract и отличается calculation source semantics.
- **ENG-Q508.** Calculation definition является managed configuration.
- **ENG-Q509.** Calculated Parameter может использовать inputs разных managed objects.
- **ENG-Q510.** Calculation может использовать historical/window inputs для calculation classes, которым это разрешено.
- **ENG-Q511.** Calculated Parameter может исполняться на Full или Edge согласно placement capabilities.
- **ENG-Q512.** Calculation execution placement определяется явно или через declared placement policy.
- **ENG-Q513.** Validation учитывает location/availability inputs и execution placement calculated Parameter.
- **ENG-Q514.** Calculated value имеет timestamp semantics.
- **ENG-Q515.** Calculation contract определяет timestamp/event-time semantics calculated output и provenance; server-now не является универсальным правилом.
- **ENG-Q516.** Quality calculated value определяется explicit quality-propagation policy.
- **ENG-Q517.** Dependency cycles calculations обнаруживаются validation до activation.
- **ENG-Q518.** Arbitrary cyclic calculations/iterative solver не поддерживаются по умолчанию; это отдельный специализированный calculation type при явном contract.
- **ENG-Q519.** Governed JS/runtime foundation может быть одним из mechanisms calculation, но простые typed calculations должны иметь более простой deterministic UX.
- **ENG-Q520.** Изменение calculation проходит dependency/impact analysis.
- **ENG-Q521.** Aggregate Parameter может быть специализированным calculation class с явной aggregate semantics/provenance.
- **ENG-Q522.** Aggregate operations типизированы: sum, average, min/max, count, weighted average, runtime duration, integration и другие declared operations.
- **ENG-Q523.** Aggregate compatibility зависит от Parameter type/quantity semantics; operation нельзя применять к любому типу произвольно.
- **ENG-Q524.** Aggregate сохраняет provenance input set.
- **ENG-Q525.** Dynamic-membership aggregate допустим с declared membership rule/version/provenance.

### 10.9 Discrete states, bit fields, counters и window/event calculations

- **ENG-Q526.** Raw discrete values могут иметь explicit state mapping.
- **ENG-Q527.** Raw values вроде 0/1 могут переводиться в semantic states Closed/Open, Stopped/Running, Normal/Fault и т.п. по Profile/Type contract.
- **ENG-Q528.** State labels являются semantic/localizable presentation of stable state codes.
- **ENG-Q529.** Цвет не является абсолютной частью Parameter semantics; Type/Template может дать presentation hints, конечный UI следует presentation policy.
- **ENG-Q530.** Alarm не кодируется как обычный Parameter enum: Parameter может давать condition/status, Alarm остаётся отдельной operational entity.
- **ENG-Q531.** Device Profile поддерживает typed decoding bit fields/status words.
- **ENG-Q532.** Raw status word может сохраняться вместе с derived semantic child Parameters по policy.
- **ENG-Q533.** Semantic bit Parameters имеют собственные stable identities.
- **ENG-Q534.** Изменение bit mapping проходит profile/version compatibility/impact analysis.
- **ENG-Q535.** Counter моделируется как semantic Parameter capability/type, а не generic numeric без lifecycle semantics.
- **ENG-Q536.** Monotonic counter отличается от ordinary numeric value.
- **ENG-Q537.** Counter semantics учитывают rollover.
- **ENG-Q538.** Counter semantics учитывают reset events.
- **ENG-Q539.** Counter semantics учитывают physical device replacement.
- **ENG-Q540.** Counter semantics поддерживают governed correction.
- **ENG-Q541.** Reverse/decrease поддерживается только как explicit capability там, где физический meter это допускает.
- **ENG-Q542.** Простое уменьшение counter не классифицируется автоматически как reset или rollover без соответствующей semantics/evidence.
- **ENG-Q543.** Для rollover задаётся modulus/range там, где применимо.
- **ENG-Q544.** Physical meter replacement создаёт новый counter segment.
- **ENG-Q545.** Functional metering point identity сохраняется при replacement physical meter.
- **ENG-Q546.** Aggregate consumption может пересекать несколько physical counter segments с provenance.
- **ENG-Q547.** Counter correction не переписывает raw source history; original facts сохраняются.
- **ENG-Q548.** Поддерживается derived rate-of-change Parameter/calculation.
- **ENG-Q549.** Поддерживается integration over time, например power→energy, через typed calculation semantics.
- **ENG-Q550.** Rate/integration calculations учитывают gaps/quality.
- **ENG-Q551.** При gap last value может считаться постоянным только если конкретная calculation policy это явно разрешает.
- **ENG-Q552.** Результат integration/window calculation показывает uncertainty/data coverage там, где неполные данные влияют на достоверность.
- **ENG-Q553.** Поддерживаются moving average/min/max/statistical window calculations.
- **ENG-Q554.** Calculation window может быть time-based или sample-count-based согласно type.
- **ENG-Q555.** Window calculations имеют restart/recovery semantics.
- **ENG-Q556.** Для window calculation определяется warm-up state после start/restart.
- **ENG-Q557.** До заполнения необходимого window output может иметь Uncertain/incomplete quality/status согласно policy.
- **ENG-Q558.** Не каждый source event обязан иметь continuous current-value semantics; если он используется как Parameter, current-value behaviour должно быть определено contract.
- **ENG-Q559.** Event fact и Parameter current value являются разными operational concepts.
- **ENG-Q560.** Adapter event может одновременно обновить Parameter и породить Event, если Device Profile contract это определяет.

### 10.10 Commands boundary, ownership, lifecycle, migrations и corrections

- **ENG-Q561.** Writable Parameter не означает прямую запись value: любое намерение изменить state managed equipment проходит semantic Command Model.
- **ENG-Q562.** UI может визуально представить простую command как setpoint control/edit, но backend semantics остаётся Command, а не raw write.
- **ENG-Q563.** Desired setpoint и measured process value могут быть разными Parameters/semantic values.
- **ENG-Q564.** Durable policy/config setpoint и operational runtime setpoint различаются; выбор configuration vs Command определяется предметной semantics, а не формой поля.
- **ENG-Q565.** Source-owned observed property и Dispatcher-managed Parameter/configuration имеют явное ownership distinction.
- **ENG-Q566.** Source-reported metadata не становится Dispatcher desired config автоматически без соответствующего ownership contract.
- **ENG-Q567.** Drift существует только там, где есть declared desired state для сравнения с actual/source fact.
- **ENG-Q568.** Само изменение source-owned fact не является drift/Alarm автоматически.
- **ENG-Q569.** Parameter можно planned-delete в change set аналогично другим managed configuration entities.
- **ENG-Q570.** До publication удаления active Parameter/runtime binding продолжает действовать согласно текущей active configuration.
- **ENG-Q571.** Удаление Parameter требует impact analysis всех прямых/транзитивных consumers.
- **ENG-Q572.** Historical identity/semantic descriptor Parameter сохраняется после удаления active definition.
- **ENG-Q573.** Новый Parameter с тем же semantic key не получает автоматически identity удалённого; восстановление/migration — explicit operation.
- **ENG-Q574.** Изменение Parameter data type требует compatibility/migration analysis.
- **ENG-Q575.** Integer→wider compatible integer может быть совместимым после проверки range/consumers.
- **ENG-Q576.** Float→Boolean обычно breaking semantic/type change.
- **ENG-Q577.** Quantity change Pressure→Temperature является breaking.
- **ENG-Q578.** Unit change bar→kPa может быть compatible при однозначной conversion и совместимых consumers.
- **ENG-Q579.** Type/unit migration анализирует historical data semantics.
- **ENG-Q580.** Type/unit migration анализирует Rules consumers.
- **ENG-Q581.** Type/unit migration анализирует Dashboards/Mimics consumers.
- **ENG-Q582.** Type/unit migration анализирует Alarm definitions.
- **ENG-Q583.** Type/unit migration анализирует Reports/API/integrations и другие consumers.
- **ENG-Q584.** Compatible unit change не требует физического переписывания historical samples; presentation/effective conversion может использовать version/provenance, explicit migration — отдельная операция.
- **ENG-Q585.** Если historical migration выполняется, original semantics/provenance сохраняются.
- **ENG-Q586.** Historical UI показывает semantic-version boundary там, где она влияет на интерпретацию series/report.
- **ENG-Q587.** Historical Parameter values можно корректировать через governed historical correction.
- **ENG-Q588.** Historical correction не удаляет original source fact.
- **ENG-Q589.** Correction хранит reason/source/actor/time и audit/provenance.
- **ENG-Q590.** Correction может охватывать time range.
- **ENG-Q591.** Historical correction может использовать replacement dataset; late/backfill остаётся отдельной semantic operation и не маскируется correction.
- **ENG-Q592.** Correction может задавать governed correction rule/formula, если semantics это допускает.
- **ENG-Q593.** UI/history различает original series и effective corrected series.
- **ENG-Q594.** Correction запускает recalculation dependent aggregates/derived views согласно declared recalculation policy и impact graph, а не универсально синхронно.
- **ENG-Q595.** Уже выпущенный immutable report не переписывается задним числом; он может ссылаться на later correction.

### 10.11 Bulk Engineering, templates и observed Parameters

- **ENG-Q596.** Object/Type/Profile Engineering предоставляет Parameters registry/list для соответствующего scope.
- **ENG-Q597.** Поддерживается cross-object Parameters Engineering view для bulk work/diagnostics, не заменяющий Object Registry.
- **ENG-Q598.** Parameters поддерживают multi-select и bulk edit общих semantic properties.
- **ENG-Q599.** Bulk editor явно показывает mixed values.
- **ENG-Q600.** Bulk action показывает affected Parameter/object count и scope summary.
- **ENG-Q601.** Historization policy можно изменять массово.
- **ENG-Q602.** Polling/acquisition policy можно изменять массово там, где применимо.
- **ENG-Q603.** Freshness policy можно изменять массово.
- **ENG-Q604.** Display unit policy/preferences можно изменять массово там, где это managed configuration, отдельно от personal preference.
- **ENG-Q605.** Source mappings можно массово изменять через structured transformation/address rules с preview, а не только вручную по одному.
- **ENG-Q606.** Create Copies поддерживает deterministic generation Parameter/device addresses/bindings.
- **ENG-Q607.** Adapter-aware generation может increment/register/unit-ID/topic suffix и другие declared fields.
- **ENG-Q608.** Все generated bindings показываются в preview до применения.
- **ENG-Q609.** Address/resource collisions выявляются до publication.
- **ENG-Q610.** Declarative mass-address rules являются основным workflow; arbitrary scripting — extension для действительно сложных случаев, а не default.
- **ENG-Q611.** Parameter semantic definitions могут поступать из Object Type.
- **ENG-Q612.** Device Profile может привязывать technical source/command mapping к type-defined Parameter.
- **ENG-Q613.** Object Template может задавать instance-level Parameter configuration/defaults/overrides.
- **ENG-Q614.** Engineering показывает одновременно provenance semantic definition, technical binding и instance/template override layers.
- **ENG-Q615.** Instance не может удалить type-required Parameter, если semantic contract требует его присутствие.
- **ENG-Q616.** Instance может отключить optional source/binding Parameter, если contract/policy это допускает.
- **ENG-Q617.** Observed object может получать Parameters runtime без individual Publish согласно source contract.
- **ENG-Q618.** Dynamic observed Parameters имеют stable source-derived identity/incarnation semantics.
- **ENG-Q619.** Удаление/исчезновение observed Parameter у source не удаляет его history автоматически.
- **ENG-Q620.** Disappeared observed Parameter сохраняет presence/disappearance state и provenance.
- **ENG-Q621.** При promotion observed→managed не все source Parameters автоматически становятся Dispatcher-owned; ownership определяется type/profile/source contract.
- **ENG-Q622.** Engineering до promotion показывает, какие properties/Parameters останутся source-owned, какие станут managed и какие требуют explicit mapping.

### 10.12 Runtime inspection, draft tests, scale и rights

- **ENG-Q623.** Engineering Parameter inspector показывает current value.
- **ENG-Q624.** Parameter inspector показывает quality.
- **ENG-Q625.** Parameter inspector показывает freshness/age.
- **ENG-Q626.** Parameter inspector показывает source timestamp, если он существует.
- **ENG-Q627.** Detailed diagnostics показывают receive/ingest timestamp.
- **ENG-Q628.** Parameter inspector показывает current active source при multi-source semantics.
- **ENG-Q629.** Raw value доступен в technical/diagnostic view согласно rights, но не обязан быть primary presentation.
- **ENG-Q630.** Normalized/effective value доступен и отличим от raw/source-specific values.
- **ENG-Q631.** Parameter inspector показывает active substitution/operational override state.
- **ENG-Q632.** Parameter inspector показывает historization state/effective policy summary.
- **ENG-Q633.** Parameter inspector показывает current executor/Connection/source path where applicable.
- **ENG-Q634.** Draft Parameter mapping можно проверить до Publish через diagnostic/test flow.
- **ENG-Q635.** Draft mapping test может читать raw source при наличии rights/capabilities.
- **ENG-Q636.** Draft test не превращает tested source/binding в active operational Parameter source.
- **ENG-Q637.** Test preview может показывать raw→decoded→normalized→semantic value chain.
- **ENG-Q638.** Test-write/output mapping допускается только как explicit diagnostic Command Model operation с отдельными rights/reason/safety checks.
- **ENG-Q639.** Invalid Parameter binding не обязательно блокирует publication unrelated scopes, если consistency/dependency analysis позволяет безопасно отделить affected scope.
- **ENG-Q640.** Validation issue Parameter указывает конкретную причину и source/configuration field, где возможно.
- **ENG-Q641.** Missing Adapter/Connection root cause отображается у root resource, а affected Parameters получают derived impact indication вместо независимых ложных root causes.
- **ENG-Q642.** Один общий root cause агрегируется с affected count/drill-down, а не размножается тысячами неразличимых ошибок.
- **ENG-Q643.** Engineering рассчитан на десятки/сотни тысяч Parameters как функциональный scalability contract.
- **ENG-Q644.** UI не должен требовать загрузить все Parameter values установки/объекта до начала работы; используется scoped/incremental presentation.
- **ENG-Q645.** Live subscriptions ограничиваются текущим нужным context/viewport/workspace вместо подписки на все значения.
- **ENG-Q646.** Массовые Parameter registries поддерживают server-side filtering/query semantics; конкретный API определяется архитектурой.
- **ENG-Q647.** Engineering показывает saturation/resource impact high-frequency acquisition where relevant.
- **ENG-Q648.** Package/Profile может объявлять expected resource cost/passport для Parameters/acquisition.
- **ENG-Q649.** Rights могут различать просмотр Parameter configuration и runtime value.
- **ENG-Q650.** Просмотр raw technical binding может быть отдельным permission/capability.
- **ENG-Q651.** Право редактировать Parameter configuration не даёт автоматически право выполнять Commands.
- **ENG-Q652.** Право runtime substitution отдельно от configuration edit.
- **ENG-Q653.** Sensitive Parameters могут иметь sensitivity classification там, где это предметно применимо.
- **ENG-Q654.** Export Parameter history/data повторно проверяет rights/sensitivity/export policy.

### 10.13 Explainability, naming, documentation, restart, Edge и retention

- **ENG-Q655.** Для сложного effective Parameter value Engineering предоставляет explainability «почему сейчас такое значение?» с источником, transformations, selection, substitution/calculation.
- **ENG-Q656.** Explainability показывает reason выбора current active source.
- **ENG-Q657.** Explainability показывает quality propagation reason.
- **ENG-Q658.** Explainability показывает effective freshness policy/source.
- **ENG-Q659.** Engineering explainability показывает relevant template/local overrides.
- **ENG-Q660.** Engineering предоставляет drill-down calculation dependency chain с разумным масштабированием.
- **ENG-Q661.** Parameter display name может быть локализуемым через package/definition translations.
- **ENG-Q662.** Semantic key/machine code не локализуется.
- **ENG-Q663.** Изменение semantic key после использования проходит compatibility/migration semantics; stable internal identity отдельно сохраняется.
- **ENG-Q664.** Validation предотвращает duplicate semantic keys внутри соответствующего contract scope.
- **ENG-Q665.** Parameter может иметь description/help documentation.
- **ENG-Q666.** Parameter может иметь source documentation/reference metadata.
- **ENG-Q667.** Device Profile может предоставлять register/protocol documentation для binding.
- **ENG-Q668.** Organization может добавлять annotation/note к Parameter без изменения package-owned definition.
- **ENG-Q669.** Organization annotation сохраняется через compatible profile update, если Parameter identity/semantic continuity сохраняется.
- **ENG-Q670.** Default configuration value не является автоматически runtime fallback/substitution; эти semantics различаются.
- **ENG-Q671.** Calculated/manual Parameter может иметь explicit initialization policy.
- **ENG-Q672.** После restart persisted/restored value нельзя выдавать за fresh current sample; он получает соответствующее provenance/quality/freshness state.
- **ENG-Q673.** Restored cached value и новый source sample различаются.
- **ENG-Q674.** Edge offline продолжает acquisition/historization Parameters для опубликованного local contour.
- **ENG-Q675.** Parameter identities сохраняются при offline accumulation на Edge.
- **ENG-Q676.** После reconnect late Edge samples синхронизируются с сохранением source timestamps, receive context, quality и provenance.
- **ENG-Q677.** Full receive time не заменяет Edge/source timestamps.
- **ENG-Q678.** Если Full/Edge работали на разных active config versions, historical samples сохраняют достаточную configuration lineage для корректной интерпретации semantics.
- **ENG-Q679.** Duplicate sample detection использует source/adapter sequence/message identity там, где источник её предоставляет.
- **ENG-Q680.** Dispatcher не обещает exactly-once acquisition внешних samples там, где источник/transport этого не гарантирует; gaps/dedupe/uncertainty явны.
- **ENG-Q681.** Dedupe не удаляет два реально разных samples только потому, что их values совпали; используется sample/message identity semantics.
- **ENG-Q682.** Sequence number/source message identity сохраняются там, где полезны для provenance/dedupe/diagnostics.
- **ENG-Q683.** Retention policy задаётся иерархически через defaults/policies/controlled overrides, а не вручную на каждом Parameter.
- **ENG-Q684.** Retention и historization enable являются разными configuration dimensions.
- **ENG-Q685.** Legal Hold может продлевать сохранение данных сверх normal retention.
- **ENG-Q686.** Удаление Parameter definition не удаляет history автоматически и не является обычным механизмом data erasure.

### 10.14 Historical semantics, import/export, diagnostics, alarms, Rules и API

- **ENG-Q687.** При совместимых physical quantity/unit versions trend может показывать единую normalized series в выбранной unit, сохраняя version boundaries/provenance.
- **ENG-Q688.** Incompatible semantic migrations не склеиваются в одну historical series автоматически без explicit mapping.
- **ENG-Q689.** Historical viewer умеет показывать semantic-version boundaries.
- **ENG-Q690.** Template update, удаляющий Parameter, учитывает historical semantics/identity и downstream consumers.
- **ENG-Q691.** Наличие history не обязательно блокирует удаление active Parameter definition, но required historical semantic descriptor/identity сохраняется.
- **ENG-Q692.** Перед publication удаления Parameter его consumers должны быть разрешены, мигрированы или удалены в том же coherent change согласно validation/impact.
- **ENG-Q693.** Parameter definitions можно создавать через subject import в обычный draft flow.
- **ENG-Q694.** Import не обновляет существующий Parameter: collision с existing entity является validation error; overwrite/sync — не import semantics.
- **ENG-Q695.** Configuration export может включать selected Parameter definitions.
- **ENG-Q696.** Runtime values/history не входят автоматически в configuration export; configuration и operational-data export разделены.
- **ENG-Q697.** Engineering предоставляет temporary high-detail/raw diagnostic tracing без обязательного изменения permanent historization policy.
- **ENG-Q698.** Diagnostic trace может временно запросить более высокую детализацию/rate, если source/Adapter это поддерживает, не меняя durable acquisition policy скрытно.
- **ENG-Q699.** Diagnostic tracing имеет scope/limits/expiry/audit.
- **ENG-Q700.** Adapter/Package может объявлять maximum diagnostic rate/resource limits.
- **ENG-Q701.** Engineering имеет commissioning view Parameters: live table raw/normalized/effective value, quality, source/time и related diagnostics.
- **ENG-Q702.** Commissioning view поддерживает temporary filtering/selection/worksets.
- **ENG-Q703.** Commissioning view может предоставлять safe semantic Commands рядом с Parameters при наличии rights и Command Model checks.
- **ENG-Q704.** Raw diagnostic writes доступны только через separate diagnostic command contour, а не редактированием raw field.
- **ENG-Q705.** Alarm configuration ссылается на stable semantic Parameter identity.
- **ENG-Q706.** Alarm threshold может задаваться в compatible unit, отличной от Parameter display unit.
- **ENG-Q707.** Изменение display unit не меняет physical alarm threshold.
- **ENG-Q708.** Alarm evaluation policy отдельно учитывает threshold condition, quality и freshness/availability semantics.
- **ENG-Q709.** Communication loss не моделируется подменой Parameter value на 0; это отдельное condition/state.
- **ENG-Q710.** Rule references используют stable Parameter identity.
- **ENG-Q711.** Rule editor получает typed contract Parameter: type, quantity, unit, quality/freshness capabilities и другие relevant metadata.
- **ENG-Q712.** Rule publication/continued compatibility блокируется или требует migration при incompatible Parameter change.
- **ENG-Q713.** Rules имеют typed access к quality/freshness/time/provenance metadata согласно API contract.
- **ENG-Q714.** Public Parameter API использует stable public identifier/contract, а не внутренний database ID как внешний contract.
- **ENG-Q715.** API read возвращает value plus timestamps/quality/provenance согласно declared contract и caller rights.
- **ENG-Q716.** API ingestion может передавать source timestamp, quality, sequence/message identity и provenance metadata.
- **ENG-Q717.** External API caller не может подменять arbitrary Parameter source без configured ingestion source/capability.
- **ENG-Q718.** API ingestion source предварительно configured/authorized и имеет identity/scope.

### 10.15 Formatting, grouping, service extensions, failover и fallback

- **ENG-Q719.** Decimal separator/local number formatting не является частью stored numeric value semantics.
- **ENG-Q720.** Precision/display formatting настраивается отдельно от underlying numeric semantics.
- **ENG-Q721.** Display rounding не меняет alarm/calculation/command evaluation value.
- **ENG-Q722.** Potential significant precision loss при conversion/type migration выявляется validation/impact.
- **ENG-Q723.** Parameters могут иметь semantic/presentation groups/sections внутри object: Status, Measurements, Setpoints, Energy, Diagnostics и т.п.
- **ENG-Q724.** Parameter group не обязательно является identity-owning hierarchy object; чаще это presentation/semantic grouping, если самостоятельная сущность не нужна.
- **ENG-Q725.** Type/Profile может задавать default grouping/order Parameters.
- **ENG-Q726.** Organization/Template может адаптировать grouping/order без изменения Parameter identities.
- **ENG-Q727.** Object overall status не обязан быть одним Parameter; это multidimensional derived representation из Parameters, alarms, lifecycle, connectivity и других state dimensions.
- **ENG-Q728.** Type/Profile может определить contributions Parameters/states в object summary state по typed contract.
- **ENG-Q729.** Summary state semantics типизированы и не сводятся к arbitrary UI color rules.
- **ENG-Q730.** Service extensions VMS/ACS/TOиР/IT и другие могут публиковать Parameters общей core object identity через typed extension contract.
- **ENG-Q731.** Semantic owner service может иметь специализированный editor таких Parameters/configuration, но lifecycle остаётся общей governed configuration.
- **ENG-Q732.** Service-extension Parameters могут использоваться общими trends/Rules/alarms/API там, где typed contract и rights это допускают.
- **ENG-Q733.** Stale value нельзя показывать как обычное актуальное число без indication.
- **ENG-Q734.** Presentation может скрывать/ослаблять display bad last-known value по policy, но underlying last value/age остаются доступны; факт не теряется.
- **ENG-Q735.** UI различает Bad/no value и Bad with last-known value X/age.
- **ENG-Q736.** Color не является единственным quality/freshness indicator.
- **ENG-Q737.** При source failover semantic Parameter history сохраняет continuity одной identity.
- **ENG-Q738.** Source-specific historical series могут быть доступны отдельно согласно historization/retention policy.
- **ENG-Q739.** Source switch записывается как provenance/operational transition там, где влияет на interpretation.
- **ENG-Q740.** Система может объяснить, почему конкретный source был выбран в конкретный период/момент, если decision data доступна.
- **ENG-Q741.** Historical correction ошибочного measurement не удаляет реально произошедший Alarm episode.
- **ENG-Q742.** Correction может быть связана с Alarm/Incident как объяснение, что abnormal condition была вызвана bad measurement.
- **ENG-Q743.** Audit/commands/actions, совершённые на основании прежних данных, не переписываются после historical correction.
- **ENG-Q744.** Parameter definition внутри Type/Profile имеет version provenance.
- **ENG-Q745.** При compatible profile update Parameter semantic identity сохраняется, если technical mapping меняется, а semantic meaning остаётся тем же.
- **ENG-Q746.** Migration Parameters не сопоставляет сущности только по display name.
- **ENG-Q747.** Migration mapping использует explicit/stable identities/keys/rules и сохраняет provenance.
- **ENG-Q748.** Fallback source и manual substitution являются разными mechanisms.
- **ENG-Q749.** Fallback source выбирается automatic explicit source-selection policy.
- **ENG-Q750.** Manual substitution является intentional governed operational exception.
- **ENG-Q751.** Hardcoded/default value становится operational fallback только если это явно configured safety/operational policy; наличие default само по себе не даёт fallback semantics.

### 10.16 Command feedback, source diagnostics, Compact, validation, impact и value pipeline

- **ENG-Q752.** Semantic Command может использовать Parameter как success/feedback criterion.
- **ENG-Q753.** Command success criterion учитывает quality/freshness/identity of feedback согласно contract.
- **ENG-Q754.** Stale/invalid feedback Parameter не позволяет автоматически считать command successful, если criterion требует подтверждённый актуальный feedback.
- **ENG-Q755.** Desired/setpoint echo и actual process feedback различаются как semantic sources/Parameters там, где система их предоставляет.
- **ENG-Q756.** Adapter может предоставлять optional live source browser/debugger как специализированную diagnostic capability.
- **ENG-Q757.** Из discovered raw field можно создать proposed Parameter binding через normal draft/configuration flow.
- **ENG-Q758.** Raw-source discovery/browse не меняет active configuration автоматически.
- **ENG-Q759.** Uninstall Adapter/Package не удаляет historical Parameter semantics/provenance.
- **ENG-Q760.** Minimal non-executable semantic descriptor после uninstall позволяет интерпретировать persisted historical Parameter values.
- **ENG-Q761.** Для просмотра history не требуется сохранять runtime Adapter UI; generic safe rendering допустим.
- **ENG-Q762.** Compact/simple setup может скрывать большую часть сложной Parameter model, но не заменяет underlying contract другой моделью.
- **ENG-Q763.** Simple flow вроде «датчик температуры → AI2 → 0–10V → 0…50°C» может объединять несколько underlying configuration steps в один guided setup.
- **ENG-Q764.** Professional Engineering открывает тот же Parameter, созданный simple setup, без migration/format conversion.
- **ENG-Q765.** Full Parameter validation проверяет schema/data type.
- **ENG-Q766.** Full Parameter validation проверяет quantity/unit compatibility.
- **ENG-Q767.** Full Parameter validation проверяет source mapping/schema.
- **ENG-Q768.** Full Parameter validation проверяет Connection/Adapter prerequisites.
- **ENG-Q769.** Full Parameter validation проверяет execution placement compatibility.
- **ENG-Q770.** Full Parameter validation проверяет polling/acquisition/resource capacity.
- **ENG-Q771.** Full Parameter validation проверяет freshness feasibility.
- **ENG-Q772.** Full Parameter validation проверяет calculation cycles/dependencies.
- **ENG-Q773.** Full Parameter validation проверяет historization/retention policy validity where relevant.
- **ENG-Q774.** Full Parameter validation проверяет Alarm/Rule/Command и другие consumer compatibility.
- **ENG-Q775.** Full Parameter validation учитывает security/sensitivity/rights impact.
- **ENG-Q776.** Full Parameter validation учитывает Template/Profile migration conflicts.
- **ENG-Q777.** При изменении Parameter Engineering показывает его consumers/impact graph.
- **ENG-Q778.** Consumers группируются по типам: alarms, Rules, commands, calculations, dashboards/mimics, historian/reports, API/integrations, service extensions и другие declared consumers.
- **ENG-Q779.** Impact analysis различает direct и transitive dependencies.
- **ENG-Q780.** Массовое изменение тысяч Parameters показывает aggregated impact с drill-down, а не требует открытия каждого consumer отдельно.
- **ENG-Q781.** Configuration lifecycle states Parameter включают Active, New, Modified, Planned removal, Conflict, Invalid, Update available, Migrating/Diverged where applicable.
- **ENG-Q782.** Runtime states quality/freshness/source connectivity/substitution не смешиваются с configuration lifecycle state.
- **ENG-Q783.** UI может одновременно показывать configuration state и runtime operational state Parameter.
- **ENG-Q784.** Value pipeline концептуально различает Source fact/raw → decoded → normalized engineering → source-specific semantic → active-source selection → operational exception/substitution → effective semantic value → consumers; неиспользуемые стадии могут отсутствовать.
- **ENG-Q785.** Обычному оператору не показывается весь technical pipeline постоянно; primary UI показывает effective operational meaning, Engineering/diagnostics дают drill-down.
- **ENG-Q786.** Rules/Alarms/API contracts явно определяют, какой semantic layer они потребляют; default effective value не отменяет explicit contract semantics.
- **ENG-Q787.** Package/Adapter не может скрытно вставлять transformation между source и semantic value без provenance/versioned contract.
- **ENG-Q788.** Все значимые transformations effective Parameter value должны быть versioned/explainable в достаточной степени.
- **ENG-Q789.** Из Parameter можно перейти к source/Connection/Profile/Type/Template и другим relevant origins без потери Engineering context.
- **ENG-Q790.** Из Connection/Profile можно перейти к affected Parameters/objects с фильтрацией/impact context.

## 11. Принятые Functional Requirements — `ENG-FR001–ENG-FR150`

- **ENG-FR001.** Engineering является специализированным workspace внутри общего Web Shell Dispatcher, а не отдельным приложением.
- **ENG-FR002.** Engineering работает с общей объектной и конфигурационной моделью Dispatcher; специализированные сервисы не получают параллельный механизм сохранения managed configuration.
- **ENG-FR003.** Active published configuration по умолчанию не редактируется непосредственно: изменения выполняются через draft/change set.
- **ENG-FR004.** Переход между объектами, реестрами и редакторами не должен терять незавершённые изменения текущего change set.
- **ENG-FR005.** Validation issue, связанная с сущностью, идентифицирует сущность и даёт переход к месту исправления, где это возможно.
- **ENG-FR006.** Engineering работает от Compact до Full/Edge; composition/UX density могут отличаться, но underlying configuration model едина.
- **ENG-FR007.** Permissions применяются к Engineering, действиям, объектным scopes, областям конфигурации, review/approval/publication.
- **ENG-FR008.** Published и draft состояния визуально и семантически различаются.
- **ENG-FR009.** При недоступности backend/Full/Edge UI не выдаёт stale/unknown данные за актуальные и показывает состояние связи/достоверности.
- **ENG-FR010.** Engineering routes/URLs должны быть достаточно устойчивыми для открытия конкретного object/registry/change set по ссылке при наличии прав.
- **ENG-FR011.** Engineering предоставляет общий Object Registry с фильтрацией по class/type/location/lifecycle/configuration state и другим поддерживаемым свойствам.
- **ENG-FR012.** Один managed object может отображаться в нескольких структурных/функциональных представлениях без создания дополнительных identities.
- **ENG-FR013.** Display name, engineering code/path и internal stable identity — разные понятия; изменение первых не меняет identity.
- **ENG-FR014.** Создание, duplicate, discovery promotion и import создают сущности в текущем change set и не обходят governed configuration lifecycle.
- **ENG-FR015.** Draft-состояние объекта явно отличается от active; пользователь может сравнить effective draft и published representation.
- **ENG-FR016.** Location hierarchy не ограничивается building-oriented схемой и поддерживает domain-specific location types.
- **ENG-FR017.** Изменение location, navigation parent и relationships сохраняет object identity, если не выполняется семантическая замена объекта.
- **ENG-FR018.** Typed relations доступны как минимум из editor/inspector обоих endpoints с указанием типа и направления.
- **ENG-FR019.** Source-owned properties observed objects визуально отличаются от Dispatcher-managed configuration.
- **ENG-FR020.** Disappeared observed object не удаляется автоматически вместе с накопленной operational/history identity.
- **ENG-FR021.** Split functional-position/physical-unit model и simple combined model — два допустимых способа моделирования на одном foundation.
- **ENG-FR022.** Install/remove physical unit сохраняет историю installation intervals и identities обеих сторон.
- **ENG-FR023.** Duplicate/Create Copies никогда не копирует internal identity, runtime history, audit history или identity конкретной physical unit.
- **ENG-FR024.** Create Copies показывает preview итоговых entities и обнаруживает conflicts до добавления результата в draft.
- **ENG-FR025.** Published deletion configuration object не уничтожает данные, необходимые для интерпретации исторических фактов/ссылок.
- **ENG-FR026.** Engineering визуально и семантически различает Object Type, Device Profile и Object Template; слово template не используется как универсальная сущность для всех трёх.
- **ENG-FR027.** Object Type определяет semantic contract объекта независимо от конкретного protocol/device implementation.
- **ENG-FR028.** Device Profile связывает semantic contract с техническим устройством через поддерживаемые adapter capabilities.
- **ENG-FR029.** Object Template представляет reusable engineering configuration и может создавать составную структуру из нескольких сущностей и relations.
- **ENG-FR030.** Package-provided type/profile/template показывает provenance, package identity и version.
- **ENG-FR031.** Package-owned definitions не редактируются напрямую; локальное изменение выполняется через допустимый extension/clone/override mechanism.
- **ENG-FR032.** Для inherited/composed property Engineering показывает источник определения и effective value.
- **ENG-FR033.** Type editor предотвращает publication структурно несовместимого override базового semantic contract.
- **ENG-FR034.** Published Object Type имеет versioned identity, а изменение published definition создаёт новое version state.
- **ENG-FR035.** Обновление type существующих instances выполняется с compatibility и impact analysis.
- **ENG-FR036.** Engineering показывает objects, использующие устаревшую или отличающуюся version type/profile/template.
- **ENG-FR037.** Device Profile может объявлять hardware/firmware compatibility, а Engineering показывает результат matching.
- **ENG-FR038.** Raw technical mapping и semantic parameter/command identity остаются различимыми слоями.
- **ENG-FR039.** Изменение profile mapping существующего устройства является governed configuration change и не применяется незаметно к active system.
- **ENG-FR040.** Template inputs имеют typed schema, validation и отображаемые значения до создания instances.
- **ENG-FR041.** Linked template instance сохраняет identity template и version, от которой произошло текущее managed состояние.
- **ENG-FR042.** Engineering показывает для template-managed значения как минимум состояния Inherited, Overridden и Changed in template или эквивалентные по смыслу.
- **ENG-FR043.** Publication новой template version и применение этой version к существующим instances являются разными управляемыми действиями.
- **ENG-FR044.** Engineering показывает distribution используемых версий type/profile/template и instances с доступными обновлениями.
- **ENG-FR045.** Local override сохраняет связь с inherited definition и не превращает inherited value в непрозрачную копию.
- **ENG-FR046.** Для template-managed значения UI позволяет определить effective source: template, parent template, instance input, local override либо другой объявленный слой.
- **ENG-FR047.** Массовое применение template update обязательно проходит preview с aggregate summary и drill-down до exceptions/conflicts.
- **ENG-FR048.** Массовая операция над тысячами instances не требует одновременного отображения полного diff каждого instance.
- **ENG-FR049.** Unresolved template conflicts остаются явным состоянием и не разрешаются last-write-wins.
- **ENG-FR050.** Template instance хранит использованные template inputs и их связь с generated configuration.
- **ENG-FR051.** Nested templates используют versioned dependency contracts; update вложенной зависимости не меняет published parent configuration скрытно.
- **ENG-FR052.** Detach сохраняет identity объекта, effective configuration и historical provenance, но прекращает дальнейшее template management instance.
- **ENG-FR053.** Reattach/adopt является governed migration operation с matching, preview и conflict resolution.
- **ENG-FR054.** Template с linked instances не может быть бесследно удалён.
- **ENG-FR055.** Замена Device Profile сохраняет semantic object identity, когда реальный управляемый объект остаётся тем же.
- **ENG-FR056.** Profile replacement анализирует parameters, commands, alarms, Rules, dashboards, history bindings и другие consumers изменяемого semantic contract.
- **ENG-FR057.** Type migration явно отличается от физической/семантической replacement объекта.
- **ENG-FR058.** Engineering сохраняет traceability type/profile/template migrations и version transitions.
- **ENG-FR059.** Engineering различает Adapter как software contribution, Connection как managed configuration и Endpoint как адресуемый resource/target interaction side.
- **ENG-FR060.** Connections имеют самостоятельный registry и одновременно доступны contextually из связанных managed objects.
- **ENG-FR061.** Connection editor использует typed schema Adapter contribution и не требует raw JSON как штатный Engineering workflow.
- **ENG-FR062.** Connection schema поддерживает field-level type, validation, help, sensitivity и functional impact metadata.
- **ENG-FR063.** Endpoint representation определяется Adapter semantics и не ограничивается IP/port model.
- **ENG-FR064.** Simple single-device workflow не заставляет инженера вручную создавать отдельную Endpoint entity, если у неё нет самостоятельной identity/reuse/lifecycle semantics.
- **ENG-FR065.** Shared physical channels моделируются как общий resource, а device endpoints — как отдельные logical addresses, когда protocol topology этого требует.
- **ENG-FR066.** Engineering анализирует conflicts shared-channel addressing и incompatible transport settings до publication.
- **ENG-FR067.** Local hardware I/O использует общий Adapter/Connection/Profile foundation, а raw channels остаются technical bindings semantic objects.
- **ENG-FR068.** Secret values не являются обычными configuration properties; Connection хранит references на Secret Store.
- **ENG-FR069.** Право use/replace secret не даёт автоматически право reveal existing secret value.
- **ENG-FR070.** Certificate/trust material подключается к Connection через managed references, а не хранится как opaque embedded blob.
- **ENG-FR071.** Test Connection работает с draft settings без publication и по умолчанию является non-invasive diagnostic operation.
- **ENG-FR072.** Результат Connection Test не считается доказательством текущего operational health после окончания теста.
- **ENG-FR073.** Engineering одновременно и раздельно показывает desired configuration, deployed/active configuration version и actual runtime connection state.
- **ENG-FR074.** Durable Enabled/Disabled является managed configuration, а emergency/protective disable — отдельный operational/admin override.
- **ENG-FR075.** Desired execution placement и фактический authoritative/current executor отображаются как разные states.
- **ENG-FR076.** Validation учитывает наличие required Adapter/capability на target execution nodes и hardware/runtime platform compatibility.
- **ENG-FR077.** Configuration publication может опережать deployment required Adapter только как explicit desired state; activation невозможна до выполнения prerequisites.
- **ENG-FR078.** Actuator-capable connection contour не получает multi-active authority автоматически; execution регулируется authority semantics Command Model.
- **ENG-FR079.** Обычная потеря связи с executor не является достаточным доказательством прекращения его actuator authority.
- **ENG-FR080.** Connection handover показывает current/target/pending/completed/blocked/uncertain и не маскируется мгновенным изменением configuration field.
- **ENG-FR081.** Managed object может использовать несколько Connections с typed roles; active source/executor selection задаётся explicit policy.
- **ENG-FR082.** Parameter является самостоятельной semantic entity со stable identity, definition и runtime representation.
- **ENG-FR083.** Display name, semantic key и internal Parameter identity являются разными concepts.
- **ENG-FR084.** Acquired, calculated, aggregate, manual и service-derived Parameters используют общий Parameter foundation и отличаются source semantics.
- **ENG-FR085.** Parameter имеет declared type/schema; arbitrary JSON не является штатной заменой typed data contract.
- **ENG-FR086.** Physical quantity и unit имеют machine semantics и проверяемую compatibility, а не являются только display strings.
- **ENG-FR087.** Source binding отделён от semantic Parameter definition и может изменяться без смены identity, если semantic meaning сохраняется.
- **ENG-FR088.** Raw technical representation, normalization и effective semantic value остаются различимыми layers.
- **ENG-FR089.** Polling/subscription/event-driven acquisition не меняет semantic identity Parameter.
- **ENG-FR090.** Acquisition configuration поддерживает hierarchical defaults и effective-value/policy explainability.
- **ENG-FR091.** Adapter может оптимизировать physical protocol operations без вынесения packet-level optimization в основной Engineering workflow.
- **ENG-FR092.** Operational sample сохраняет timestamp provenance, достаточную для различения source time и receive/ingest time.
- **ENG-FR093.** Original source timestamp не переписывается скрытно при clock correction/normalization.
- **ENG-FR094.** Value quality, time quality, freshness и connection state являются связанными, но отдельными dimensions.
- **ENG-FR095.** Common quality model нормализует protocol-specific quality, сохраняя исходные diagnostic details/provenance.
- **ENG-FR096.** Last known value сохраняется вместе с age/quality/freshness и не выдаётся за новый достоверный sample.
- **ENG-FR097.** No value ever, stale, bad source value и communication loss являются различимыми states/causes.
- **ENG-FR098.** Normalization/scaling является versioned configuration с preview и provenance.
- **ENG-FR099.** Изменение scaling не переписывает existing historical truth автоматически.
- **ENG-FR100.** Calibration конкретной physical unit сохраняет связь с identity этого unit и не смешивается с site-level correction.
- **ENG-FR101.** Physical/source, normalization, operating, recommended, command и alarm limits являются разными semantic concepts.
- **ENG-FR102.** Historization policy принадлежит Parameter/data policy, а не visualization.
- **ENG-FR103.** Acquisition rate, historian recording policy и retention являются отдельными configuration dimensions.
- **ENG-FR104.** Historian сохраняет timestamps/quality/provenance в объёме, необходимом для корректной интерпретации historical facts.
- **ENG-FR105.** Late arrival, backfill, interpolation и correction являются различными semantics.
- **ENG-FR106.** Visualization interpolation не создаёт вымышленные historical source facts.
- **ENG-FR107.** Acquisition deadband и historian deadband различаются и не меняют Alarm/Rule semantics скрытно.
- **ENG-FR108.** Semantic Parameter может иметь несколько independent source identities и explicit active-source policy.
- **ENG-FR109.** Source switching сохраняет Parameter identity и provenance фактически выбранного source.
- **ENG-FR110.** Manual substitution не изменяет original source fact, а создаёт отдельный governed effective-value layer.
- **ENG-FR111.** Manual substitution использует общий operational-exception lifecycle и остаётся видимой пользователю, если влияет на operational meaning.
- **ENG-FR112.** Calculated Parameter использует общий semantic Parameter contract и имеет versioned calculation, input dependencies, placement, timestamp и quality semantics.
- **ENG-FR113.** Calculation dependency cycles обнаруживаются до activation, кроме специально объявленных calculation classes.
- **ENG-FR114.** Aggregates имеют typed aggregation semantics и сохраняют provenance состава inputs.
- **ENG-FR115.** Discrete state machine codes отделены от локализуемых labels и presentation hints.
- **ENG-FR116.** Raw status words могут декодироваться в самостоятельные semantic Parameters без потери diagnostic raw representation.
- **ENG-FR117.** Counter является semantic capability с rollover/reset/replacement/correction semantics, а не ordinary numeric field.
- **ENG-FR118.** Physical meter replacement создаёт новый physical counter segment, сохраняя functional metering-point continuity.
- **ENG-FR119.** Rate/integration/window calculations учитывают quality, gaps, coverage и restart/warm-up state.
- **ENG-FR120.** Event facts и current Parameter values являются разными operational concepts.
- **ENG-FR121.** Writable representation Parameter не обходит Command Model; UI setpoint control может быть простым, execution остаётся semantic Command.
- **ENG-FR122.** Source-owned facts не становятся Dispatcher desired configuration автоматически.
- **ENG-FR123.** Удаление Parameter не уничтожает его historical identity/semantic descriptor.
- **ENG-FR124.** Type/quantity/unit migration Parameter требует dependency and compatibility analysis.
- **ENG-FR125.** Historical migration/correction сохраняет original provenance и не переписывает immutable audit/command/report facts.
- **ENG-FR126.** Bulk Parameter Engineering поддерживает filtering, multi-selection, mixed values, structured transformations и preview.
- **ENG-FR127.** Mass source/address generation использует Adapter-aware declarative rules с collision detection до publication.
- **ENG-FR128.** Engineering раздельно показывает provenance semantic definition, Device Profile binding, template configuration и local overrides.
- **ENG-FR129.** Observed Parameters могут появляться runtime без individual publication согласно source identity contract и сохраняют presence/provenance.
- **ENG-FR130.** Engineering Parameter inspector предоставляет live operational context и technical drill-down без смешивания runtime facts с editable configuration.
- **ENG-FR131.** Draft source binding можно diagnostically проверить без превращения его в active configuration.
- **ENG-FR132.** Parameter validation errors агрегируются по common root cause, когда одна проблема затрагивает множество Parameters.
- **ENG-FR133.** Parameter Engineering рассчитан на high cardinality и не предполагает загрузку/подписку всех live values одновременно.
- **ENG-FR134.** Rights на configuration, runtime value, raw binding, substitution и Commands являются independent capabilities/scopes.
- **ENG-FR135.** Для сложного effective value Engineering предоставляет explainability source→transformation→selection→substitution/calculation.
- **ENG-FR136.** Formatting, localization и display rounding не изменяют underlying numeric semantics.
- **ENG-FR137.** Parameter presentation grouping не создаёт новые semantic identities без предметной необходимости.
- **ENG-FR138.** Service extensions могут добавлять Parameters общей object identity через typed extension contracts.
- **ENG-FR139.** Edge offline сохраняет Parameter identity, acquisition, history, timestamps, quality и configuration lineage согласно published local contour.
- **ENG-FR140.** Reconnect не заменяет source timestamps Full receive time и не обещает exactly-once там, где source/transport этого не гарантирует.
- **ENG-FR141.** Retention/Legal Hold отделены от удаления active Parameter definition.
- **ENG-FR142.** Historical viewer учитывает semantic/unit-version boundaries без обязательного физического переписывания old samples.
- **ENG-FR143.** Historical correction не удаляет реально произошедшие alarms/actions, но может объяснять их связь с ошибочными исходными данными.
- **ENG-FR144.** Package/Adapter uninstall не делает сохранённую Parameter history семантически нечитаемой.
- **ENG-FR145.** Compact simple setup использует тот же полный Parameter model, скрывая лишнюю complexity UX, а не создавая альтернативный format.
- **ENG-FR146.** Commissioning предоставляет specialized live diagnostic view Parameters и safe Command entry points.
- **ENG-FR147.** Alarm, Rule, Command, Dashboard и API bindings используют stable Parameter identity и typed semantic contract.
- **ENG-FR148.** Full Parameter validation охватывает type, units, sources, placement, resources, freshness, calculations, consumers, security и migration compatibility.
- **ENG-FR149.** Configuration lifecycle state Parameter и runtime operational state отображаются раздельно и одновременно.
- **ENG-FR150.** Effective Parameter value концептуально строится через explainable pipeline от source fact до effective semantic value; неиспользуемые stages могут отсутствовать.

## 12. Functional model после `ENG-Q790`

### 12.1 Connection / execution path

- **Adapter** — installed software contribution/capability; **Connection** — managed configuration; **Endpoint** — typed addressable resource side when it has independent semantics.
- Connection configuration, software deployment, activation и runtime health остаются разными states.
- Desired execution placement не равно фактическому executor/authority. Для actuator-capable contour действует один authoritative executor, а handover имеет явный lifecycle и recovery semantics.
- Secrets/certificates подключаются references, а diagnostic connection test не публикует draft configuration.

### 12.2 Parameter semantic pipeline

Parameter имеет stable identity и отдельные layers definition/source/runtime. Effective value концептуально может проходить цепочку:

`source fact/raw → decode → normalization/calibration → source-specific semantic value → active-source selection → operational exception/substitution → effective semantic value → consumers`.

Не каждый Parameter обязан использовать каждый layer, но применённые transformations должны оставаться typed, versioned и explainable.

### 12.3 Data trust

Value quality, time quality, freshness, connection state и provenance не сливаются в один флаг. Last-known value сохраняется, но не выдаётся за fresh sample. Late data, backfill, interpolation и historical correction имеют разные semantics.

### 12.4 History continuity

Parameter identity, source provenance, semantic/unit versions и configuration lineage позволяют интерпретировать history после profile changes, Edge offline, source failover, corrections и package uninstall. Historical correction не переписывает original facts, audit, commands или already-generated immutable evidence.

### 12.5 Operational control boundary

Writable-looking Parameter/setpoint не предоставляет direct equipment write. Намерение изменить managed equipment выполняется через Semantic Command Model; raw diagnostic write остаётся отдельным усиленным diagnostic command contour.

## 13. Checkpoint traceability

| Checkpoint | Диапазон | Содержание | Git |
|---|---|---|---|
| `ENG-CP01` | `ENG-Q001–ENG-Q110`, `ENG-FR001–ENG-FR025` | Engineering foundation + Objects & Structure | `688392edb17ddce6e4d3874ff54344aacc2033b0` |
| `ENG-CP02` | добавлены `ENG-Q111–ENG-Q250`, `ENG-FR026–ENG-FR058` | Types / Profiles / Templates + lifecycle / propagation / migrations | `fa38f437a90f98cdb4091a25187eec67f2213e6a` |
| `ENG-CP03` | добавлены `ENG-Q251–ENG-Q790`, `ENG-FR059–ENG-FR150` | Connections / execution placement + complete Parameter/value pipeline | `READY TO COMMIT` |

## 14. Точка продолжения

Продолжить с `ENG-Q791` и подробно определить **Semantic Commands**: command definitions, arguments, risk, preconditions/interlocks, confirmations/approvals, execution authority, timeout/uncertain results, idempotency/retry, concurrency, feedback/success criteria, offline Edge policy, diagnostics и integration with Rules/API/UI. Git checkpoint выполнять автоматически по `../ROADMAP.md`.
