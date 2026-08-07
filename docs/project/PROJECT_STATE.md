# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA до этого пакета:** `3da5f084eeb3f354c218a9fe6dfe02a3e9c7abdb`.

## 1. Текущий режим

Общая продуктовая концепция зрелой универсальной модульной SCADA-платформы «Диспетчер» прошла первую независимую сквозную проверку. Все `AUD-001–AUD-026` разобраны в основном концептуальном чате и отражены решениями `PRD-Q528–PRD-Q676`. Дорожная карта, программная архитектура, технологический стек и код по-прежнему не формируются.

## 2. Зафиксированный объём

Редакция 4 сохраняет все решения `PRD-Q001–PRD-Q527` и дополнительно фиксирует:

- единый безопасный actuator path и границы Rules/basic/safety control: `PRD-Q528–PRD-Q539`;
- четыре класса изменений, общий configuration governance и typed package contributions: `PRD-Q540–PRD-Q557`;
- distributed publication/activation, consistency domains, execution authority, offline authorization, reconciliation и audit recovery lineage: `PRD-Q558–PRD-Q583`;
- observed objects, service extensions, Person linkage, principal↔resource и physical-unit lifecycle: `PRD-Q584–PRD-Q610`;
- Rule runtime state, operational exceptions, maintenance semantics, time quality и counters: `PRD-Q611–PRD-Q638`;
- cross-domain sensitivity/privacy и лицензионный safety/security/recovery carve-out: `PRD-Q639–PRD-Q659`;
- notification ACK boundary, VMS authoritative archive, BIM boundary и Dashboard/Knowledge semantics: `PRD-Q660–PRD-Q676`.

Суффиксы `R` и `N` в более ранних решениях обозначают итоговые заменяющие формулировки. Отвергнутые черновые варианты не являются продуктовыми решениями.

## 3. Ключевые инварианты после первой проверки

### Управление

Любое действие, предназначенное изменить состояние управляемого объекта, проходит Command Model. Raw diagnostic write существует только как отдельная усиленно контролируемая команда. Rules поддерживают supervisory и совместимое некритическое closed-loop управление, но не заменяют hard-real-time/safety controller.

### Change governance

Любое изменение относится к одному из четырёх классов: managed configuration, administrative transaction, package/software deployment или secret/credential operation. Специализированные сервисы не получают собственный параллельный Save/Apply lifecycle. Package остаётся install/version unit, внутри которой могут быть несколько typed contributions.

### Full / Edge / HA

Publish отделён от фактической activation. Edge показывает desired/active configuration, взаимозависимые изменения образуют consistency domains, а каждый execution contour имеет одного authoritative executor. После partition применяется class-specific reconciliation, не last-write-wins. Offline authorization зависит от риска; restore создаёт явную recovery lineage.

### Object foundation

Общая модель включает managed и observed dynamic objects. Специализированные сервисы расширяют одну core identity service extensions. Person минимально связывает SCADA account, ACS subject, ТОиР и notification roles без HR-модели. Managed resource и authentication principal различаются. Индивидуальная physical unit сохраняет identity через склад/установку/ремонт.

### Достоверность и эксплуатационные исключения

Persistent Rule state не является скрытой конфигурацией. Manual substitution, manual control, automation override, alarm suppression и maintenance остаются разными mechanisms, но формируют единый effective operational context. Time quality является отдельной системной характеристикой. Counters имеют reset/rollover/replacement segments.

### Security/privacy/licensing

Cross-domain sensitivity дополняет permissions/scopes и работает при export/API, не превращая продукт в DLP. Licensing регулирует коммерческое расширение, но никогда не блокирует safety, security remediation, backup/restore, emergency access, audit integrity и recoverability существующего контура.

### VMS, BIM и Knowledge

VMS имеет authoritative archive policy и может использовать внешнюю VMS/NVR или Edge как owner. BIM остаётся специализированной предметной функциональностью с 3D workspace; окончательная top-level service topology решается в BIM-концепции. Dashboard — operational live view, Knowledge/Document — долговременный управляемый информационный content.

## 4. Актуальные продуктовые документы

1. `docs/product/PRODUCT_CONCEPT.md` — общая концепция, редакция 4.
2. `docs/product/PRODUCT_DECISIONS.md` — точный реестр решений через `PRD-Q676`.
3. `docs/product/DASHBOARDS_AND_MIMICS_CONCEPT.md` — углублённая рабочая концепция дашбордов и мнемосхем.
4. `docs/project/CONCEPT_AUDIT_RESOLUTION_2026-08-07.md` — карта первой независимой проверки `AUD-001–AUD-026` к принятым решениям.
5. `docs/product/BASELINE_CONCEPT_2026-08-04.md` — историческая исходная концепция.

## 5. Точка продолжения

Следующий рекомендуемый шаг — передать редакцию 4 в отдельный независимый чат на **повторную сквозную проверку**, прежде всего на то, что `AUD-001–AUD-026` действительно закрыты и решения `PRD-Q528–PRD-Q676` не создали новых противоречий. Основной концептуальный чат остаётся владельцем решений и не принимает предложения аудитора автоматически.

После успешной повторной проверки отдельно и подробно обсуждаются крупные предметные области: HVAC, энергетика/энергоучёт, пожарные системы, СКУД, BIM, карты/планы, IT и сети, вода/насосы, освещение, лифты/эскалаторы, VMS, ТОиР и другие области. Предметные концепции могут уточнять сегодняшние границы.

Перед работой читать `AGENTS.md`, настоящий файл, `docs/product/PRODUCT_CONCEPT.md`, `PRODUCT_DECISIONS.md` и карту аудита. Не восстанавливать решения по истории чата.

## 6. Действующие ограничения работы

- Не использовать отменённый черновик дорожной карты.
- Не определять очередность реализации внутри продуктовой концепции.
- Не обозначать функции «будущими» только ради упрощения концепции.
- Не смешивать продуктовый сервис с программным микросервисом.
- Не считать отдельную инженерную область автоматическим основанием для отдельного сервиса.
- Независимый аудит формирует замечания, но не изменяет репозиторий и не принимает продуктовые решения вместо основного чата.
- Не тратить контекст на исчерпывающую проверку каждого подтверждённого коммита без причины.
