export function App() {
  return (
    <div className="app-shell">
      <header className="global-header">
        <div className="global-header__leading">
          <button
            className="menu-trigger"
            type="button"
            aria-label="Основное меню"
            aria-disabled="true"
            disabled
            title="Навигация будет добавлена в CORE-003 / Step 3"
          >
            <span className="menu-trigger__icon" aria-hidden="true">
              <span />
              <span />
              <span />
            </span>
          </button>

          <strong className="product-name">Диспетчер</strong>
        </div>

        <div
          className="global-actions-slot"
          role="group"
          aria-label="Область глобальных действий"
        />
      </header>

      <main className="workspace">
        <section className="workspace__content" aria-labelledby="workspace-title">
          <p className="workspace__eyebrow">Web Shell</p>
          <h1 id="workspace-title">Рабочая область</h1>
          <p className="workspace__description">
            Здесь будут открываться интерфейсы сервисов платформы.
          </p>
        </section>
      </main>
    </div>
  );
}
