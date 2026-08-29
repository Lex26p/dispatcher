import type {
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';

export const PROJECT_MANAGER_SERVICE = 'project-manager.v1';

export interface Project {
  id: string;
  name: string;
  description: string;
}

export interface CreateProjectInput {
  name: string;
  description?: string;
}

export interface UpdateProjectInput {
  id: string;
  name: string;
  description: string;
}

export interface ProjectManagerTransport {
  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse>;
}

export class ProjectManagerClientResponseError extends Error {
  readonly code = 'project.client_invalid_response';

  constructor(message: string) {
    super(message);
    this.name = 'ProjectManagerClientResponseError';
  }
}

export class ProjectManagerClient {
  constructor(private readonly transport: ProjectManagerTransport) {}

  createProject(
    input: CreateProjectInput,
  ): ServiceHubRequestHandle<Project> {
    const payload: Record<string, unknown> = {
      name: input.name,
    };

    if (input.description !== undefined) {
      payload.description = input.description;
    }

    return mapResponse(
      this.transport.request(
        PROJECT_MANAGER_SERVICE,
        'create-project',
        payload,
      ),
      parseProjectResponse,
    );
  }

  listProjects(): ServiceHubRequestHandle<Project[]> {
    return mapResponse(
      this.transport.request(
        PROJECT_MANAGER_SERVICE,
        'list-projects',
        {},
      ),
      parseProjectListResponse,
    );
  }

  getProject(id: string): ServiceHubRequestHandle<Project> {
    return mapResponse(
      this.transport.request(
        PROJECT_MANAGER_SERVICE,
        'get-project',
        { id },
      ),
      parseProjectResponse,
    );
  }

  updateProject(
    input: UpdateProjectInput,
  ): ServiceHubRequestHandle<Project> {
    return mapResponse(
      this.transport.request(
        PROJECT_MANAGER_SERVICE,
        'update-project',
        {
          id: input.id,
          name: input.name,
          description: input.description,
        },
      ),
      parseProjectResponse,
    );
  }
}

function mapResponse<T>(
  request: ServiceHubRequestHandle<unknown>,
  parse: (payload: unknown) => T,
): ServiceHubRequestHandle<T> {
  return {
    id: request.id,
    response: request.response.then(parse),
    cancel: () => request.cancel(),
  };
}

function parseProjectResponse(payload: unknown): Project {
  if (!isRecord(payload) || !('project' in payload)) {
    throw invalidResponse('Project Manager response is missing project');
  }

  return parseProject(payload.project);
}

function parseProjectListResponse(payload: unknown): Project[] {
  if (!isRecord(payload) || !Array.isArray(payload.projects)) {
    throw invalidResponse('Project Manager response is missing projects');
  }

  return payload.projects.map(parseProject);
}

function parseProject(value: unknown): Project {
  if (
    !isRecord(value) ||
    typeof value.id !== 'string' ||
    value.id.length === 0 ||
    typeof value.name !== 'string' ||
    typeof value.description !== 'string'
  ) {
    throw invalidResponse('Project Manager returned an invalid project');
  }

  return {
    id: value.id,
    name: value.name,
    description: value.description,
  };
}

function invalidResponse(message: string): ProjectManagerClientResponseError {
  return new ProjectManagerClientResponseError(message);
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}
