import { fireEvent, render, screen, waitFor } from '@testing-library/react';

import type {
  ServiceHubConnectionState,
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';
import { ServiceHubRequestError } from '../service-hub/ServiceHubClient';
import {
  type ServiceHubClientAccess,
  ServiceHubProvider,
} from '../service-hub/ServiceHubProvider';
import {
  PROJECT_CONTEXT_STORAGE_KEY,
  ProjectContextProvider,
  useProjectContext,
} from './ProjectContextProvider';

const storedProject = {
  id: 'project-1',
  name: 'Сохранённый проект',
  description: 'Сессия браузера',
};

class TestServiceHubClient implements ServiceHubClientAccess {
  connectionState: ServiceHubConnectionState;
  readonly operations: string[] = [];

  private readonly response: Promise<unknown>;
  private readonly listeners = new Set<
    (state: ServiceHubConnectionState) => void
  >();

  constructor(
    connectionState: ServiceHubConnectionState,
    response: Promise<unknown> = Promise.resolve({ project: storedProject }),
  ) {
    this.connectionState = connectionState;
    this.response = response;
  }

  subscribeConnectionState(
    listener: (state: ServiceHubConnectionState) => void,
  ): () => void {
    this.listeners.add(listener);
    listener(this.connectionState);
    return () => this.listeners.delete(listener);
  }

  connect(): Promise<void> {
    return Promise.resolve();
  }

  disconnect(): void {}

  request<TResponse = unknown>(
    service: string,
    operation: string,
    _payload: unknown,
    _options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    expect(service).toBe('project-manager.v1');
    expect(operation).toBe('get-project');
    this.operations.push(operation);

    return {
      id: `test-${this.operations.length}`,
      response: this.response as Promise<TResponse>,
      cancel: () => false,
    };
  }

  cancel(_requestId: string): boolean {
    return false;
  }
}

function ContextProbe() {
  const { selectedProject, selectProject, clearProject } = useProjectContext();

  return (
    <div>
      <span data-testid="current-project">
        {selectedProject?.name ?? 'Глобальный'}
      </span>
      <button
        type="button"
        onClick={() =>
          selectProject({
            id: 'project-2',
            name: 'Выбранный проект',
            description: 'Выбран из UI',
          })
        }
      >
        Выбрать
      </button>
      <button type="button" onClick={clearProject}>
        Очистить
      </button>
    </div>
  );
}

function renderContext(client: TestServiceHubClient) {
  return render(
    <ServiceHubProvider client={client}>
      <ProjectContextProvider>
        <ContextProbe />
      </ProjectContextProvider>
    </ServiceHubProvider>,
  );
}

describe('ProjectContextProvider', () => {
  beforeEach(() => {
    window.sessionStorage.clear();
  });

  it('stores a selected project for the current browser session and clears it explicitly', () => {
    renderContext(new TestServiceHubClient('disconnected'));

    expect(screen.getByTestId('current-project')).toHaveTextContent('Глобальный');

    fireEvent.click(screen.getByRole('button', { name: 'Выбрать' }));

    expect(screen.getByTestId('current-project')).toHaveTextContent(
      'Выбранный проект',
    );
    expect(window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY)).toContain(
      'project-2',
    );

    fireEvent.click(screen.getByRole('button', { name: 'Очистить' }));

    expect(screen.getByTestId('current-project')).toHaveTextContent('Глобальный');
    expect(window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY)).toBeNull();
  });

  it('restores and refreshes the selected project when Service Hub is connected', async () => {
    window.sessionStorage.setItem(
      PROJECT_CONTEXT_STORAGE_KEY,
      JSON.stringify(storedProject),
    );

    const client = new TestServiceHubClient(
      'connected',
      Promise.resolve({
        project: {
          ...storedProject,
          name: 'Актуальное имя',
        },
      }),
    );

    renderContext(client);

    expect(screen.getByTestId('current-project')).toHaveTextContent(
      'Сохранённый проект',
    );

    await screen.findByText('Актуальное имя');
    expect(client.operations).toEqual(['get-project']);
    await waitFor(() => {
      expect(window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY)).toContain(
        'Актуальное имя',
      );
    });
  });

  it('clears a stored selection only when Project Manager confirms project.not_found', async () => {
    window.sessionStorage.setItem(
      PROJECT_CONTEXT_STORAGE_KEY,
      JSON.stringify(storedProject),
    );

    const client = new TestServiceHubClient(
      'connected',
      Promise.reject(
        new ServiceHubRequestError('test-get', {
          code: 'project.not_found',
          message: 'Project not found',
        }),
      ),
    );

    renderContext(client);

    await waitFor(() => {
      expect(screen.getByTestId('current-project')).toHaveTextContent('Глобальный');
    });
    expect(window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY)).toBeNull();
  });

  it('keeps the selection when Project Manager is temporarily unavailable', async () => {
    window.sessionStorage.setItem(
      PROJECT_CONTEXT_STORAGE_KEY,
      JSON.stringify(storedProject),
    );

    const client = new TestServiceHubClient(
      'connected',
      Promise.reject(
        new ServiceHubRequestError('test-get', {
          code: 'hub.provider_unavailable',
          message: 'Provider unavailable',
        }),
      ),
    );

    renderContext(client);

    await waitFor(() => {
      expect(client.operations).toEqual(['get-project']);
    });
    expect(screen.getByTestId('current-project')).toHaveTextContent(
      'Сохранённый проект',
    );
    expect(window.sessionStorage.getItem(PROJECT_CONTEXT_STORAGE_KEY)).toContain(
      'project-1',
    );
  });
});
