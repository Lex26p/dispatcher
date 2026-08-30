import {
  type MouseEvent,
  useEffect,
  useRef,
  useState,
} from 'react';

import { useProjectContext } from './project-context/ProjectContextProvider';
import { ProjectManagerView } from './project-manager/ProjectManagerView';
import type { ServiceHubConnectionState } from './service-hub/ServiceHubClient';
import { useServiceHub } from './service-hub/ServiceHubProvider';
import { useUserSession } from './user-session/UserSessionProvider';
import { LoginView } from './users-access/LoginView';
import { UsersAccessAdminView } from './users-access/UsersAccessAdminView';

const workspacePath = '/';
const projectsPath = '/projects';
const loginPath = '/login';
const accessPath = '/access';

const connectionStateLabels: Record<ServiceHubConnectionState, string> = {
  disconnected: 'Service Hub недоступен',
  connecting: 'Service Hub: подключение',
  connected: 'Service Hub подключен',
  disconnecting: 'Service Hub: отключение',
};

export function App() {
  const { connectionState } = useServiceHub();
  const { selectedProject, clearProject } = useProjectContext();
  const {
    status: userStatus,
    session,
    logout,
    hasGlobalCapability,
  } = useUserSession();
  const [currentPath, setCurrentPath] = useState(() => window.location.pathname);
  const [menuOpen, setMenuOpen] = useState(false);
  const menuTriggerRef = useRef<HTMLButtonElement>(null);
  const workspaceLinkRef = useRef<HTMLAnchorElement>(null);

  useEffect(() => {
    const handlePopState = () => {
      setCurrentPath(window.location.pathname);
      setMenuOpen(false);
    };

    window.addEventListener('popstate', handlePopState);

    return () => {
      window.removeEventListener('popstate', handlePopState);
    };
  }, []);

  useEffect(() => {
    if (!menuOpen) {
      return;
    }

    workspaceLinkRef.current?.focus();

    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        setMenuOpen(false);
        menuTriggerRef.current?.focus();
      }
    };

    document.addEventListener('keydown', handleKeyDown);

    return () => {
      document.removeEventListener('keydown', handleKeyDown);
    };
  }, [menuOpen]);

  const navigate = (path: string) => {
    if (window.location.pathname !== path) {
      window.history.pushState(null, '', path);
    }

    setCurrentPath(path);
    setMenuOpen(false);
  };

  const navigateTo = (
    event: MouseEvent<HTMLAnchorElement>,
    path: string,
  ) => {
    event.preventDefault();
    navigate(path);
  };

  const closeMenu = () => {
    setMenuOpen(false);
    menuTriggerRef.current?.focus();
  };

  const handleLogout = async () => {
    clearProject();
    try {
      await logout();
    } catch {
      // Local session state is cleared even if remote logout cannot complete.
    }
    navigate(workspacePath);
  };

  const authenticated = userStatus === 'authenticated' && session !== null;
  const canAdministerAccess = authenticated && hasGlobalCapability('admin');
  const workspaceActive = currentPath === workspacePath;
  const projectsActive = currentPath === projectsPath;
  const loginActive = currentPath === loginPath;
  const accessActive = currentPath === accessPath;

  return (
    <div className="app-shell">
      <header className="global-header">
        <div className="global-header__leading">
          <button
            ref={menuTriggerRef}
            className="menu-trigger"
            type="button"
            aria-label="Основное меню"
            aria-controls="global-navigation"
            aria-expanded={menuOpen}
            onClick={() => setMenuOpen((open) => !open)}
          >
            <span className="menu-trigger__icon" aria-hidden="true">
              <span />
              <span />
              <span />
            </span>
          </button>

          <strong className="product-name">Диспетчер</strong>

          <div
            className="project-context-summary"
            role="group"
            aria-label="Контекст проекта"
            title={selectedProject?.name ?? 'Глобальный контекст'}
          >
            <span className="project-context-summary__label">Контекст</span>
            <strong className="project-context-summary__value">
              {selectedProject?.name ?? 'Глобальный'}
            </strong>
            {selectedProject !== null ? (
              <button
                className="project-context-summary__clear"
                type="button"
                aria-label="Перейти в глобальный контекст"
                onClick={clearProject}
              >
                ×
              </button>
            ) : null}
          </div>
        </div>

        <div
          className="global-actions-slot"
          role="group"
          aria-label="Область глобальных действий"
        >
          <div
            className={`service-hub-status service-hub-status--${connectionState}`}
            role="status"
            aria-live="polite"
            aria-label="Состояние Service Hub"
          >
            <span className="service-hub-status__indicator" aria-hidden="true" />
            <span>{connectionStateLabels[connectionState]}</span>
          </div>

          {authenticated ? (
            <div
              className="user-session-summary"
              role="group"
              aria-label="Текущий пользователь"
            >
              <span className="user-session-summary__name" title={session.user.login}>
                {session.user.display_name || session.user.login}
              </span>
              <button
                className="header-session-action"
                type="button"
                onClick={() => void handleLogout()}
              >
                Выйти
              </button>
            </div>
          ) : userStatus === 'restoring' ? (
            <span className="user-session-summary__pending">Проверка пользователя…</span>
          ) : (
            <button
              className="header-session-action"
              type="button"
              aria-label="Открыть вход"
              onClick={() => navigate(loginPath)}
            >
              Войти
            </button>
          )}
        </div>
      </header>

      {menuOpen ? (
        <div className="navigation-layer">
          <button
            className="navigation-backdrop"
            type="button"
            aria-label="Закрыть основное меню"
            tabIndex={-1}
            onClick={closeMenu}
          />

          <nav
            id="global-navigation"
            className="global-navigation"
            aria-label="Глобальная навигация"
          >
            <p className="global-navigation__title">Навигация</p>

            <a
              ref={workspaceLinkRef}
              className="global-navigation__link"
              href={workspacePath}
              aria-current={workspaceActive ? 'page' : undefined}
              onClick={(event) => navigateTo(event, workspacePath)}
            >
              Рабочая область
            </a>

            {authenticated ? (
              <a
                className="global-navigation__link"
                href={projectsPath}
                aria-current={projectsActive ? 'page' : undefined}
                onClick={(event) => navigateTo(event, projectsPath)}
              >
                Проекты
              </a>
            ) : null}

            {canAdministerAccess ? (
              <a
                className="global-navigation__link"
                href={accessPath}
                aria-current={accessActive ? 'page' : undefined}
                onClick={(event) => navigateTo(event, accessPath)}
              >
                Пользователи и доступ
              </a>
            ) : null}

            {!authenticated && userStatus !== 'restoring' ? (
              <a
                className="global-navigation__link"
                href={loginPath}
                aria-current={loginActive ? 'page' : undefined}
                onClick={(event) => navigateTo(event, loginPath)}
              >
                Вход
              </a>
            ) : null}
          </nav>
        </div>
      ) : null}

      <main className="workspace">
        {workspaceActive ? (
          <section className="workspace__content" aria-labelledby="workspace-title">
            <p className="workspace__eyebrow">Web Shell</p>
            <h1 id="workspace-title">Рабочая область</h1>
            <p className="workspace__description">
              Здесь будут открываться интерфейсы сервисов платформы.
            </p>
          </section>
        ) : projectsActive ? (
          authenticated ? (
            <ProjectManagerView />
          ) : (
            <LoginView
              title="Вход для работы с проектами"
              description="Project Manager доступен только в authenticated user context."
            />
          )
        ) : accessActive ? (
          !authenticated ? (
            <LoginView
              title="Вход для администрирования"
              description="Администрирование Users & Access требует authenticated session."
            />
          ) : canAdministerAccess ? (
            <UsersAccessAdminView />
          ) : (
            <section className="workspace__content auth-view" aria-labelledby="access-denied-title">
              <p className="workspace__eyebrow">Users &amp; Access</p>
              <h1 id="access-denied-title">Недостаточно прав</h1>
              <p className="workspace__description">
                Для этого раздела требуется global capability admin.
              </p>
            </section>
          )
        ) : loginActive ? (
          <LoginView onAuthenticated={() => navigate(workspacePath)} />
        ) : (
          <section className="workspace__content" aria-labelledby="not-found-title">
            <p className="workspace__eyebrow">Web Shell</p>
            <h1 id="not-found-title">Страница не найдена</h1>
            <p className="workspace__description">
              В текущем Web Shell нет маршрута для этого адреса.
            </p>
            <a
              className="workspace__return-link"
              href={workspacePath}
              onClick={(event) => navigateTo(event, workspacePath)}
            >
              Вернуться в рабочую область
            </a>
          </section>
        )}
      </main>
    </div>
  );
}
