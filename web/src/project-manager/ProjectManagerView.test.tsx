import { fireEvent, render, screen, within } from '@testing-library/react';

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
import { ProjectManagerView } from './ProjectManagerView';

interface TestProject {
  id: string;
  name: string;
  description: string;
}

class TestServiceHubClient implements ServiceHubClientAccess {
  connectionState: ServiceHubConnectionState;
  readonly operations: string[] = [];

  private readonly listeners = new Set<
    (state: ServiceHubConnectionState) => void
  >();
  private projects: TestProject[];
  private nextId = 2;
  private failList = false;

  constructor(
    connectionState: ServiceHubConnectionState = 'connected',
    projects: TestProject[] = [
      {
        id: 'project-1',
        name: 'Объект 1',
        description: 'Основной объект',
      },
    ],
  ) {
    this.connectionState = connectionState;
    this.projects = [...projects];
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
    _options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    expect(service).toBe('project-manager.v1');
    this.operations.push(operation);

    if (operation === 'list-projects' && this.failList) {
      return this.handle<TResponse>(
        Promise.reject(
          new ServiceHubRequestError('test-list', {
            code: 'hub.unknown_service',
            message: 'No provider',
          }),
        ),
      );
    }

    if (operation === 'list-projects') {
      return this.handle<TResponse>(
        Promise.resolve({
          projects: this.projects.map((project) => ({ ...project })),
        }),
      );
    }

    if (operation === 'create-project') {
      const input = payload as { name: string; description?: string };
      const project = {
        id: `project-${this.nextId++}`,
        name: input.name,
        description: input.description ?? '',
      };
      this.projects.push(project);

      return this.handle<TResponse>(Promise.resolve({ project }));
    }

    if (operation === 'update-project') {
      const input = payload as TestProject;
      this.projects = this.projects.map((project) =>
        project.id === input.id ? { ...input } : project,
      );

      return this.handle<TResponse>(Promise.resolve({ project: { ...input } }));
    }

    throw new Error(`Unexpected Project Manager operation: ${operation}`);
  }

  cancel(_requestId: string): boolean {
    return false;
  }

  makeListUnavailable(): void {
    this.failList = true;
  }

  private handle<TResponse>(
    response: Promise<unknown>,
  ): ServiceHubRequestHandle<TResponse> {
    return {
      id: `test-${this.operations.length}`,
      response: response as Promise<TResponse>,
      cancel: () => false,
    };
  }
}

function renderView(client = new TestServiceHubClient()) {
  render(
    <ServiceHubProvider client={client}>
      <ProjectManagerView />
    </ServiceHubProvider>,
  );

  return client;
}

describe('ProjectManagerView', () => {
  it('lists, creates and edits projects through the shared Service Hub client', async () => {
    const client = renderView();

    const list = await screen.findByRole('list', { name: 'Список проектов' });
    expect(
      within(list).getByRole('button', { name: /Объект 1/ }),
    ).toBeInTheDocument();

    fireEvent.click(screen.getByRole('button', { name: 'Создать проект' }));

    expect(
      screen.getByRole('heading', { name: 'Новый проект' }),
    ).toBeInTheDocument();

    fireEvent.change(screen.getByRole('textbox', { name: 'Название' }), {
      target: { value: 'Новый проект' },
    });
    fireEvent.change(screen.getByRole('textbox', { name: 'Описание' }), {
      target: { value: 'Создан из Web' },
    });
    fireEvent.click(screen.getByRole('button', { name: 'Сохранить' }));

    const createdButton = await screen.findByRole('button', {
      name: /Новый проект/,
    });
    expect(createdButton).toHaveTextContent('Создан из Web');

    fireEvent.click(createdButton);

    expect(
      screen.getByRole('heading', { name: 'Редактирование проекта' }),
    ).toBeInTheDocument();

    const nameInput = screen.getByRole('textbox', { name: 'Название' });
    fireEvent.change(nameInput, {
      target: { value: 'Новый проект 2' },
    });
    fireEvent.click(screen.getByRole('button', { name: 'Сохранить' }));

    await screen.findByRole('button', { name: /Новый проект 2/ });

    expect(client.operations).toEqual([
      'list-projects',
      'create-project',
      'update-project',
    ]);
  });

  it('keeps a local error state when Project Manager is unavailable', async () => {
    const client = new TestServiceHubClient();
    client.makeListUnavailable();
    renderView(client);

    expect(await screen.findByRole('alert')).toHaveTextContent(
      'Project Manager недоступен.',
    );
    expect(
      screen.getByRole('button', { name: 'Повторить' }),
    ).toBeInTheDocument();
  });

  it('does not send project requests while Service Hub is disconnected', () => {
    const client = renderView(new TestServiceHubClient('disconnected'));

    expect(screen.getByRole('alert')).toHaveTextContent(
      'Service Hub недоступен.',
    );
    expect(client.operations).toHaveLength(0);
    expect(
      screen.getByRole('button', { name: 'Создать проект' }),
    ).toBeDisabled();
  });
});
