import { act, fireEvent, render, screen, within } from '@testing-library/react';

import { App } from './App';
import {
  PROJECT_CONTEXT_STORAGE_KEY,
  ProjectContextProvider,
} from './project-context/ProjectContextProvider';
import type {
  ServiceHubConnectionState,
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from './service-hub/ServiceHubClient';
import {
  type ServiceHubClientAccess,
  ServiceHubProvider,
} from './service-hub/ServiceHubProvider';
import {
  BrowserSessionServiceHubClient,
  BrowserSessionStore,
  USER_SESSION_STORAGE_KEY,
} from './user-session/BrowserSessionTransport';
import { UserSessionProvider } from './user-session/UserSessionProvider';

const token = 'd'.repeat(64);
const testSession = {
  user: {
    id: 'user-admin',
    login: 'admin',
    display_name: 'Test Admin',
    enabled: true,
  },
  issued_at_unix_ms: 10,
  absolute_expires_at_unix_ms: 20,
  idle_timeout_ms: 30,
};

class TestServiceHubClient implements ServiceHubClientAccess {
  connectionState: ServiceHubConnectionState;

  private readonly listeners = new Set<
    (state: ServiceHubConnectionState) => void
  >();

  constructor(connectionState: ServiceHubConnectionState = 'disconnected') {
    this.connectionState = connectionState;
  }

  subscribeConnectionState(
    listener: (state: ServiceHubConnectionState) => void,
  ): () => void {
    this.listeners.add(listener);
    listener(this.connectionState);

    return () => {
      this.listeners.delete(listener);
    };
  }

  connect(): Promise<void> {
    return Promise.resolve();
  }

  disconnect(): void {}

  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    if (service === 'users-access.v1' && operation === 'login') {
      expect(options?.auth).toBeUndefined();
      return this.handle<TResponse>(Promise.resolve({
        session_token: token,
        session: testSession,
      }));
    }

    if (service === 'users-access.v1' && operation === 'current-session') {
      expect(options?.auth?.token).toBe(token);
      return this.handle<TResponse>(Promise.resolve({ session: testSession }));
    }

    if (service === 'users-access.v1' && operation === 'evaluate-access') {
      expect(options?.auth?.token).toBe(token);
      return this.handle<TResponse>(Promise.resolve({
        allowed: true,
        effective_capabilities: ['view', 'control', 'edit', 'admin'],
      }));
    }

    if (service === 'users-access.v1' && operation === 'logout') {
      expect(options?.auth?.token).toBe(token);
      return this.handle<TResponse>(Promise.resolve({}));
    }

    if (service === 'project-manager.v1' && operation === 'list-projects') {
      expect(options?.auth?.token).toBe(token);
      return this.handle<TResponse>(Promise.resolve({ projects: [] }));
    }

    if (service === 'project-manager.v1' && operation === 'get-project') {
      expect(options?.auth?.token).toBe(token);
      const id = (payload as { id: string }).id;
      return this.handle<TResponse>(Promise.resolve({
        project: {
          id,
          name: 'Объект 1',
          description: 'Основной объект',
        },
      }));
    }

    throw new Error(`Unexpected request ${service}/${operation}`);
  }

  cancel(_requestId: string): boolean {
    return false;
  }

  publishState(state: ServiceHubConnectionState): void {
    this.connectionState = state;

    for (const listener of this.listeners) {
      listener(state);
    }
  }

  private handle<TResponse>(
    response: Promise<unknown>,
  ): ServiceHubRequestHandle<TResponse> {
    return {
      id: 'test-request',
      response: response as Promise<TResponse>,
      cancel: () => false,
    };
  }
}

function renderApp(options: {
  connectionState?: ServiceHubConnectionState;
  authenticated?: boolean;
} = {}) {
  const baseClient = new TestServiceHubClient(
    options.connectionState ?? 'disconnected',
  );
  const sessionStore = new BrowserSessionStore();
  if (options.authenticated) {
    sessionStore.setToken(token);
  }
  const client = new BrowserSessionServiceHubClient(baseClient, sessionStore);
  const view = render(
    <ServiceHubProvider client={client}>
      <UserSessionProvider sessionStore={sessionStore}>
        <ProjectContextProvider>
          <App />
        </ProjectContextProvider>
      </UserSessionProvider>
    </ServiceHubProvider>,
  );

  return { baseClient, sessionStore, ...view };
}

describe('App Shell navigation and user session', () => {
  beforeEach(() => {
    window.history.replaceState(null, '', '/');
    window.sessionStorage.clear();
  });

  it('keeps public shell navigation compact while unauthenticated', () => {
    renderApp();

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });
    fireEvent.click(menuTrigger);

    const navigation = screen.getByRole('navigation', {
      name: 'Глобальная навигация',
    });
    const workspaceLink = within(navigation).getByRole('link', {
      name: 'Рабочая область',
    });

    expect(workspaceLink).toHaveAttribute('aria-current', 'page');
    expect(within(navigation).getByRole('link', { name: 'Вход' })).toBeInTheDocument();
    expect(within(navigation).queryByRole('link', { name: 'Проекты' })).toBeNull();
    expect(workspaceLink).toHaveFocus();

    fireEvent.keyDown(document, { key: 'Escape' });
    expect(menuTrigger).toHaveFocus();
  });

  it('logs in from a protected project route and then uses authenticated Project Manager requests', async () => {
    window.history.replaceState(null, '', '/projects');
    renderApp({ connectionState: 'connected' });

    expect(
      screen.getByRole('heading', { name: 'Вход для работы с проектами' }),
    ).toBeInTheDocument();

    fireEvent.change(screen.getByRole('textbox', { name: 'Логин' }), {
      target: { value: 'admin' },
    });
    fireEvent.change(screen.getByLabelText('Пароль'), {
      target: { value: 'a sufficiently long password' },
    });
    fireEvent.click(screen.getByRole('button', { name: 'Войти' }));

    expect(await screen.findByRole('heading', { name: 'Проекты' })).toBeInTheDocument();
    expect(await screen.findByText('Проектов пока нет')).toBeInTheDocument();
    expect(screen.getByRole('group', { name: 'Текущий пользователь' })).toHaveTextContent(
      'Test Admin',
    );
  });

  it('restores authenticated user and project context from the current browser session', async () => {
    window.sessionStorage.setItem(USER_SESSION_STORAGE_KEY, token);
    window.sessionStorage.setItem(
      PROJECT_CONTEXT_STORAGE_KEY,
      JSON.stringify({
        id: 'project-1',
        name: 'Объект 1',
        description: 'Основной объект',
      }),
    );

    renderApp({ connectionState: 'connected' });

    expect(await screen.findByText('Test Admin')).toBeInTheDocument();
    expect(screen.getByRole('group', { name: 'Контекст проекта' })).toHaveTextContent(
      'Объект 1',
    );

    fireEvent.click(screen.getByRole('button', { name: 'Основное меню' }));
    const navigation = screen.getByRole('navigation', { name: 'Глобальная навигация' });
    expect(within(navigation).getByRole('link', { name: 'Проекты' })).toBeInTheDocument();
    expect(
      within(navigation).getByRole('link', { name: 'Пользователи и доступ' }),
    ).toBeInTheDocument();
  });

  it('clears user-sensitive project context on logout', async () => {
    window.sessionStorage.setItem(USER_SESSION_STORAGE_KEY, token);
    window.sessionStorage.setItem(
      PROJECT_CONTEXT_STORAGE_KEY,
      JSON.stringify({
        id: 'project-1',
        name: 'Объект 1',
        description: 'Основной объект',
      }),
    );
    renderApp({ connectionState: 'connected' });

    const logoutButton = await screen.findByRole('button', { name: 'Выйти' });
    await act(async () => {
      fireEvent.click(logoutButton);
      await Promise.resolve();
    });

    expect(window.sessionStorage.getItem(USER_SESSION_STORAGE_KEY)).toBeNull();
    expect(window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY)).toBeNull();
    expect(screen.getByRole('button', { name: 'Открыть вход' })).toBeInTheDocument();
    expect(screen.getByRole('group', { name: 'Контекст проекта' })).toHaveTextContent(
      'Глобальный',
    );
  });

  it('shows Service Hub connection state without blocking the public workspace', () => {
    const { baseClient } = renderApp({ connectionState: 'disconnected' });
    const status = screen.getByRole('status', { name: 'Состояние Service Hub' });

    expect(status).toHaveTextContent('Service Hub недоступен');
    expect(screen.getByRole('heading', { name: 'Рабочая область' })).toBeInTheDocument();

    act(() => {
      baseClient.publishState('connecting');
    });
    expect(status).toHaveTextContent('Service Hub: подключение');
  });
});
