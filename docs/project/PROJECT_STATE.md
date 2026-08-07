# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA до этого пакета:** `339a706ca988679f7d429cd0976d786e9378602b`.

## 1. Текущий режим

Общая продуктовая концепция зрелой универсальной модульной SCADA-платформы «Диспетчер» прошла первую независимую сквозную проверку и повторную проверку качества исправлений. Исходные `AUD-001–AUD-026`, остаточная несогласованность discovery и новые `AUD2-001–AUD2-002` закрыты решениями через `PRD-Q690`, включая заменяющий `PRD-Q042R`. Дорожная карта, программная архитектура, технологический стек и код по-прежнему не формируются.

## 2. Зафиксированный объём

Редакция 5 содержит итоговый реестр решений `PRD-Q001–PRD-Q690` и отдельно подчёркивает результаты второй проверки:

- `PRD-Q042R`: configuration discovery/proposal отделено от authoritative runtime observation;
- `PRD-Q677–PRD-Q682`: Emergency Disable является persistent protective override и не отменяется обычным reconciliation;
- `PRD-Q683–PRD-Q690`: observed objects при license exceed сохраняют обязательную safety/visibility baseline, а расширенные функции регулируются прозрачной license policy;
- первая проверка `AUD-001–AUD-026` и вторая проверка качества исправлений документированы отдельными audit-resolution файлами.

Суффиксы `R` и `N` в более ранних решениях обозначают итоговые заменяющие формулировки. Отвергнутые черновые варианты не являются продуктовыми решениями.

## 3. Ключевые инварианты после двух проверок

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

Cross-domain sensitivity дополняет permissions/scopes и работает при export/API, не превращая продукт в DLP. Licensing регулирует коммерческое расширение, но никогда не блокирует safety, security remediation, backup/restore, emergency access, audit integrity и recoverability существующего контура. Emergency Disable удерживает безопасный actual state как явный protective override до авторизованного снятия и не может быть автоматически отменён reconciler. Dynamic object-scale exceed не скрывает реально observed ресурсы: обязательная visibility baseline сохраняется.

### VMS, BIM и Knowledge

VMS имеет authoritative archive policy и может использовать внешнюю VMS/NVR или Edge как owner. BIM остаётся специализированной предметной функциональностью с 3D workspace; окончательная top-level service topology решается в BIM-концепции. Dashboard — operational live view, Knowledge/Document — долговременный управляемый информационный content.

## 4. Актуальные продуктовые документы

1. `docs/product/PRODUCT_CONCEPT.md` — общая концепция, редакция 5.
2. `docs/product/PRODUCT_DECISIONS.md` — точный реестр решений через `PRD-Q690`.
3. `docs/product/DASHBOARDS_AND_MIMICS_CONCEPT.md` — углублённая рабочая концепция дашбордов и мнемосхем.
4. `docs/project/CONCEPT_AUDIT_RESOLUTION_2026-08-07.md` — карта первой независимой проверки `AUD-001–AUD-026` к принятым решениям.
5. `docs/project/CONCEPT_AUDIT_RECHECK_2026-08-07.md` — результат второй проверки и закрытие `AUD2-001–AUD2-002`.
6. `docs/product/BASELINE_CONCEPT_2026-08-04.md` — историческая исходная концепция.

## 5. Точка продолжения

Следующий рекомендуемый шаг — передать **редакцию 5 в совершенно новый независимый чат на третью сквозную проверку с чистого листа**. Третьему аудитору следует дать актуальный commit и задание на полный аудит концепции, но не использовать результаты первых двух проверок как список того, что нужно искать. Это снижает anchoring и позволяет обнаружить проблемы, пропущенные предыдущим аудиторским контекстом.

После третьей проверки отдельно и подробно обсуждаются крупные предметные области: HVAC, энергетика/энергоучёт, пожарные системы, СКУД, BIM, карты/планы, IT и сети, вода/насосы, освещение, лифты/эскалаторы, VMS, ТОиР и другие области. Предметные концепции могут уточнять сегодняшние границы.

Для обычной работы читать `AGENTS.md`, настоящий файл, `docs/product/PRODUCT_CONCEPT.md`, `PRODUCT_DECISIONS.md` и оба audit-resolution документа. Для третьего независимого аудита сначала анализировать продуктовые документы без audit-resolution файлов и открыть историю аудитов только после собственного списка находок. Не восстанавливать решения по истории чата.

## 6. Действующие ограничения работы

- Не использовать отменённый черновик дорожной карты.
- Не определять очередность реализации внутри продуктовой концепции.
- Не обозначать функции «будущими» только ради упрощения концепции.
- Не смешивать продуктовый сервис с программным микросервисом.
- Не считать отдельную инженерную область автоматическим основанием для отдельного сервиса.
- Независимый аудит формирует замечания, но не изменяет репозиторий и не принимает продуктовые решения вместо основного чата.
- Не тратить контекст на исчерпывающую проверку каждого подтверждённого коммита без причины.
