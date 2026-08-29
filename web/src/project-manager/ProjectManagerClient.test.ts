import type {
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';
import {
  PROJECT_MANAGER_SERVICE,
  type ProjectManagerTransport,
  ProjectManagerClient,
  ProjectManagerClientResponseError,
} from './ProjectManagerClient';

interface RecordedRequest {
  service: string;
  operation: string;
  payload: unknown;
  options?: ServiceHubRequestOptions;
}

class TestTransport implements ProjectManagerTransport {
  readonly requests: RecordedRequest[] = [];
  private readonly responses: unknown[];

  constructor(responses: unknown[]) {
    this.responses = [...responses];
  }

  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    this.requests.push({ service, operation, payload, options });

    const response = this.responses.shift();

    return {
      id: `test-${this.requests.length}`,
      response: Promise.resolve(response as TResponse),
      cancel: () => false,
    };
  }
}

describe('ProjectManagerClient', () => {
  it('uses the versioned Project Manager service contract for CRUD-like operations', async () => {
    const firstProject = {
      id: 'project-1',
      name: 'Объект 1',
      description: 'Описание',
    };
    const updatedProject = {
      ...firstProject,
      name: 'Объект 1А',
    };
    const transport = new TestTransport([
      { project: firstProject },
      { projects: [firstProject] },
      { project: firstProject },
      { project: updatedProject },
    ]);
    const client = new ProjectManagerClient(transport);

    await expect(
      client.createProject({
        name: firstProject.name,
        description: firstProject.description,
      }).response,
    ).resolves.toEqual(firstProject);

    await expect(client.listProjects().response).resolves.toEqual([firstProject]);
    await expect(client.getProject(firstProject.id).response).resolves.toEqual(
      firstProject,
    );
    await expect(
      client.updateProject({
        id: firstProject.id,
        name: updatedProject.name,
        description: updatedProject.description,
      }).response,
    ).resolves.toEqual(updatedProject);

    expect(transport.requests).toEqual([
      {
        service: PROJECT_MANAGER_SERVICE,
        operation: 'create-project',
        payload: {
          name: 'Объект 1',
          description: 'Описание',
        },
        options: undefined,
      },
      {
        service: PROJECT_MANAGER_SERVICE,
        operation: 'list-projects',
        payload: {},
        options: undefined,
      },
      {
        service: PROJECT_MANAGER_SERVICE,
        operation: 'get-project',
        payload: { id: 'project-1' },
        options: undefined,
      },
      {
        service: PROJECT_MANAGER_SERVICE,
        operation: 'update-project',
        payload: {
          id: 'project-1',
          name: 'Объект 1А',
          description: 'Описание',
        },
        options: undefined,
      },
    ]);
  });

  it('omits optional description from create payload when it is not supplied', async () => {
    const transport = new TestTransport([
      {
        project: {
          id: 'project-2',
          name: 'Без описания',
          description: '',
        },
      },
    ]);
    const client = new ProjectManagerClient(transport);

    await client.createProject({ name: 'Без описания' }).response;

    expect(transport.requests[0]?.payload).toEqual({
      name: 'Без описания',
    });
  });

  it('rejects successful Service Hub payloads that violate Project Manager response shape', async () => {
    const transport = new TestTransport([
      {
        project: {
          id: '',
          name: 'Некорректный проект',
          description: '',
        },
      },
    ]);
    const client = new ProjectManagerClient(transport);

    await expect(
      client.getProject('project-1').response,
    ).rejects.toBeInstanceOf(ProjectManagerClientResponseError);
  });
});
