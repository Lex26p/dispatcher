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
    _service: string,
    _operation: string,
    _payload: unknown,
    _options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    throw new Error('request is not used by App tests');
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
}

function renderApp(connectionState: ServiceHubConnectionState = 'disconnected') {
  const client = new TestServiceHubClient(connectionState);
  const view = render(
    <ServiceHubProvider client={client}>
      <ProjectContextProvider>
        <App />
      </ProjectContextProvider>
    </ServiceHubProvider>,
  );

  return { client, ...view };
}

describe('App Shell navigation', () => {
  beforeEach(() => {
    window.history.replaceState(null, '', '/');
    window.sessionStorage.clear();
  });

  it('opens and closes the global menu with keyboard-friendly focus behavior', () => {
    renderApp();

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });

    expect(menuTrigger).toBeEnabled();
    expect(menuTrigger).toHaveAttribute('aria-expanded', 'false');
    expect(
      screen.getByRole('heading', { name: 'Рабочая область' }),
    ).toBeInTheDocument();

    fireEvent.click(menuTrigger);

    const navigation = screen.getByRole('navigation', {
      name: 'Глобальная навигация',
    });
    const workspaceLink = within(navigation).getByRole('link', {
      name: 'Рабочая область',
    });
    const projectsLink = within(navigation).getByRole('link', {
      name: 'Проекты',
    });

    expect(menuTrigger).toHaveAttribute('aria-expanded', 'true');
    expect(workspaceLink).toHaveAttribute('aria-current', 'page');
    expect(projectsLink).not.toHaveAttribute('aria-current');
    expect(within(navigation).getAllByRole('link')).toHaveLength(2);
    expect(workspaceLink).toHaveFocus();

    fireEvent.keyDown(document, { key: 'Escape' });

    expect(
      screen.queryByRole('navigation', { name: 'Глобальная навигация' }),
    ).not.toBeInTheDocument();
    expect(menuTrigger).toHaveAttribute('aria-expanded', 'false');
    expect(menuTrigger).toHaveFocus();

    fireEvent.click(menuTrigger);
    fireEvent.click(menuTrigger);

    expect(
      screen.queryByRole('navigation', { name: 'Глобальная навигация' }),
    ).not.toBeInTheDocument();
  });

  it('opens the real Project Manager destination without introducing project context', () => {
    renderApp('disconnected');

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });
    fireEvent.click(menuTrigger);

    const navigation = screen.getByRole('navigation', {
      name: 'Глобальная навигация',
    });
    const projectsLink = within(navigation).getByRole('link', {
      name: 'Проекты',
    });

    fireEvent.click(projectsLink);

    expect(window.location.pathname).toBe('/projects');
    expect(
      screen.getByRole('heading', { name: 'Проекты' }),
    ).toBeInTheDocument();
    expect(screen.getByRole('alert')).toHaveTextContent(
      'Service Hub недоступен.',
    );

    fireEvent.click(menuTrigger);

    const reopenedNavigation = screen.getByRole('navigation', {
      name: 'Глобальная навигация',
    });
    expect(
      within(reopenedNavigation).getByRole('link', { name: 'Проекты' }),
    ).toHaveAttribute('aria-current', 'page');
    expect(
      within(reopenedNavigation).getByRole('link', { name: 'Рабочая область' }),
    ).not.toHaveAttribute('aria-current');
  });

  it('keeps selected project context across shell navigation and can return to global context', () => {
    window.sessionStorage.setItem(
      PROJECT_CONTEXT_STORAGE_KEY,
      JSON.stringify({
        id: 'project-1',
        name: 'Объект 1',
        description: 'Основной объект',
      }),
    );

    renderApp('disconnected');

    const projectContext = screen.getByRole('group', { name: 'Контекст проекта' });
    expect(projectContext).toHaveTextContent('Объект 1');

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });
    fireEvent.click(menuTrigger);
    fireEvent.click(
      within(
        screen.getByRole('navigation', { name: 'Глобальная навигация' }),
      ).getByRole('link', { name: 'Проекты' }),
    );

    expect(window.location.pathname).toBe('/projects');
    expect(projectContext).toHaveTextContent('Объект 1');

    fireEvent.click(
      screen.getByRole('button', { name: 'Перейти в глобальный контекст' }),
    );

    expect(projectContext).toHaveTextContent('Глобальный');
    expect(window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY)).toBeNull();
  });

  it('shows an unknown-route fallback and returns to the shell workspace', () => {
    window.history.replaceState(null, '', '/missing');
    renderApp();

    expect(
      screen.getByRole('heading', { name: 'Страница не найдена' }),
    ).toBeInTheDocument();

    const menuTrigger = screen.getByRole('button', { name: 'Основное меню' });
    fireEvent.click(menuTrigger);

    const navigation = screen.getByRole('navigation', {
      name: 'Глобальная навигация',
    });
    const workspaceLink = within(navigation).getByRole('link', {
      name: 'Рабочая область',
    });

    expect(workspaceLink).not.toHaveAttribute('aria-current');

    fireEvent.click(workspaceLink);

    expect(window.location.pathname).toBe('/');
    expect(
      screen.getByRole('heading', { name: 'Рабочая область' }),
    ).toBeInTheDocument();
    expect(
      screen.queryByRole('navigation', { name: 'Глобальная навигация' }),
    ).not.toBeInTheDocument();
  });

  it('shows Service Hub connection state without blocking the workspace', () => {
    const { client } = renderApp('disconnected');
    const status = screen.getByRole('status', { name: 'Состояние Service Hub' });

    expect(status).toHaveTextContent('Service Hub недоступен');
    expect(
      screen.getByRole('heading', { name: 'Рабочая область' }),
    ).toBeInTheDocument();

    act(() => {
      client.publishState('connecting');
    });
    expect(status).toHaveTextContent('Service Hub: подключение');

    act(() => {
      client.publishState('connected');
    });
    expect(status).toHaveTextContent('Service Hub подключен');
  });
});
