# Engineering & Configuration — Functional Specification

**ID namespace:** `ENG-*`  
**Статус:** `IN PROGRESS` — приняты `ENG-Q001–ENG-Q250` и `ENG-FR001–ENG-FR058`; checkpoint `ENG-CP02` подготовлен.  
**Основание Product Concept:** `PRD-Q001–PRD-Q803`.  
**Последний подтверждённый Git SHA перед `ENG-CP02`:** `688392edb17ddce6e4d3874ff54344aacc2033b0`.  
**Следующая точка:** `ENG-Q251` — Connections / Adapters / Endpoints / Credentials / execution placement.

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
| Connections / Adapters / Endpoints | `NEXT` | с `ENG-Q251` |
| Parameters | `OPEN` | — |
| Semantic Commands | `OPEN` | — |
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

## 9. Принятые Functional Requirements — `ENG-FR001–ENG-FR058`

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

## 10. Functional model после `ENG-Q250`

### 10.1 Type / Profile / Template

- **Object Type** определяет semantic contract управляемой сущности.
- **Device Profile** связывает semantic contract с техническим устройством/adapters/endpoints.
- **Object Template** является reusable engineering recipe и может создавать/поддерживать составную configuration structure.
- package-owned definitions versioned и не редактируются локально напрямую;
- organization-defined variants используют extension/clone/override mechanisms с provenance.

### 10.2 Управляемые обновления

Новая type/profile/template version не является скрытой мутацией тысяч active объектов. Update проходит compatibility/impact, preview и обычный configuration lifecycle. Допускается управляемое сосуществование versions, pinning, selective rollout и explicit divergence.

### 10.3 Template lineage

Linked instance хранит template/version/inputs/provenance. Local override остаётся отдельным слоем над inherited value. UI показывает effective source и divergence. Nested templates используют versioned dependency contracts.

### 10.4 Migration

Profile replacement, type migration, detach и adopt/reattach являются явными governed operations с preview/impact/traceability. Stable object identity сохраняется только при сохранении semantic continuity реального объекта.

## 11. Checkpoint traceability

| Checkpoint | Диапазон | Содержание | Git |
|---|---|---|---|
| `ENG-CP01` | `ENG-Q001–ENG-Q110`, `ENG-FR001–ENG-FR025` | Engineering foundation + Objects & Structure | `688392edb17ddce6e4d3874ff54344aacc2033b0` |
| `ENG-CP02` | добавлены `ENG-Q111–ENG-Q250`, `ENG-FR026–ENG-FR058` | Types / Profiles / Templates + lifecycle / propagation / migrations | `READY TO COMMIT` |

## 12. Точка продолжения

Продолжить с `ENG-Q251` и подробно определить **Connections / Adapters / Endpoints / Credentials / execution placement**. После него перейти к Parameters. Git checkpoint выполнять по правилам `../ROADMAP.md`, без ручного отслеживания количества сообщений.
