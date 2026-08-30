import type {
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';

export const USERS_ACCESS_SERVICE = 'users-access.v1';

export type CapabilityName = 'view' | 'control' | 'edit' | 'admin';

export type AccessScope =
  | { kind: 'global' }
  | { kind: 'project'; project_id: string };

export interface User {
  id: string;
  login: string;
  display_name: string;
  enabled: boolean;
}

export interface SessionSummary {
  user: User;
  issued_at_unix_ms: number;
  absolute_expires_at_unix_ms: number;
  idle_timeout_ms: number;
}

export interface LoginResult {
  sessionToken: string;
  session: SessionSummary;
}

export interface AccessEvaluation {
  allowed: boolean;
  effectiveCapabilities: CapabilityName[];
}

export interface PermissionSet {
  id: string;
  name: string;
  capabilities: CapabilityName[];
}

export interface AccessAssignment {
  user_id: string;
  permission_set_id: string;
  scope: AccessScope;
}

export interface CreateUserInput {
  login: string;
  displayName: string;
  enabled: boolean;
  password: string;
}

export interface CreatePermissionSetInput {
  name: string;
  capabilities: CapabilityName[];
}

export interface UsersAccessTransport {
  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse>;
}

export class UsersAccessClientResponseError extends Error {
  readonly code = 'access.client_invalid_response';

  constructor(message: string) {
    super(message);
    this.name = 'UsersAccessClientResponseError';
  }
}

export class UsersAccessClient {
  constructor(private readonly transport: UsersAccessTransport) {}

  login(login: string, password: string): ServiceHubRequestHandle<LoginResult> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'login', { login, password }),
      parseLoginResult,
    );
  }

  logout(): ServiceHubRequestHandle<void> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'logout', {}),
      parseEmptyResponse,
    );
  }

  currentSession(): ServiceHubRequestHandle<SessionSummary> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'current-session', {}),
      (payload) => parseSessionEnvelope(payload).session,
    );
  }

  evaluateAccess(
    scope: AccessScope,
    capability: CapabilityName,
  ): ServiceHubRequestHandle<AccessEvaluation> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'evaluate-access', {
        scope,
        capability,
      }),
      parseAccessEvaluation,
    );
  }

  listUsers(): ServiceHubRequestHandle<User[]> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'list-users', {}),
      parseUsers,
    );
  }

  createUser(input: CreateUserInput): ServiceHubRequestHandle<User> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'create-user', {
        login: input.login,
        display_name: input.displayName,
        enabled: input.enabled,
        password: input.password,
      }),
      (payload) => parseUserEnvelope(payload).user,
    );
  }

  setUserEnabled(userId: string, enabled: boolean): ServiceHubRequestHandle<User> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'set-user-enabled', {
        user_id: userId,
        enabled,
      }),
      (payload) => parseUserEnvelope(payload).user,
    );
  }

  setUserPassword(userId: string, password: string): ServiceHubRequestHandle<void> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'set-user-password', {
        user_id: userId,
        password,
      }),
      parseEmptyResponse,
    );
  }

  listPermissionSets(): ServiceHubRequestHandle<PermissionSet[]> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'list-permission-sets', {}),
      parsePermissionSets,
    );
  }

  createPermissionSet(
    input: CreatePermissionSetInput,
  ): ServiceHubRequestHandle<PermissionSet> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'create-permission-set', {
        name: input.name,
        capabilities: input.capabilities,
      }),
      (payload) => parsePermissionSetEnvelope(payload).permission_set,
    );
  }

  listAccessAssignments(userId?: string): ServiceHubRequestHandle<AccessAssignment[]> {
    return mapResponse(
      this.transport.request(
        USERS_ACCESS_SERVICE,
        'list-access-assignments',
        userId === undefined ? {} : { user_id: userId },
      ),
      parseAssignments,
    );
  }

  assignAccess(assignment: AccessAssignment): ServiceHubRequestHandle<AccessAssignment> {
    return mapResponse(
      this.transport.request(USERS_ACCESS_SERVICE, 'assign-access', assignment),
      (payload) => parseAssignmentEnvelope(payload).assignment,
    );
  }

  removeAccessAssignment(assignment: AccessAssignment): ServiceHubRequestHandle<void> {
    return mapResponse(
      this.transport.request(
        USERS_ACCESS_SERVICE,
        'remove-access-assignment',
        assignment,
      ),
      parseEmptyResponse,
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

function parseLoginResult(payload: unknown): LoginResult {
  if (!isRecord(payload) || typeof payload.session_token !== 'string') {
    throw invalidResponse('Users & Access login response is invalid');
  }
  if (!/^[0-9a-f]{64}$/.test(payload.session_token)) {
    throw invalidResponse('Users & Access returned an invalid session token');
  }
  const session = parseSessionEnvelope(payload).session;
  return {
    sessionToken: payload.session_token,
    session,
  };
}

function parseSessionEnvelope(payload: unknown): { session: SessionSummary } {
  if (!isRecord(payload) || !('session' in payload)) {
    throw invalidResponse('Users & Access response is missing session');
  }
  return { session: parseSession(payload.session) };
}

function parseSession(value: unknown): SessionSummary {
  if (
    !isRecord(value) ||
    !('user' in value) ||
    !isSafeNumber(value.issued_at_unix_ms) ||
    !isSafeNumber(value.absolute_expires_at_unix_ms) ||
    !isSafeNumber(value.idle_timeout_ms)
  ) {
    throw invalidResponse('Users & Access returned an invalid session summary');
  }
  return {
    user: parseUser(value.user),
    issued_at_unix_ms: value.issued_at_unix_ms,
    absolute_expires_at_unix_ms: value.absolute_expires_at_unix_ms,
    idle_timeout_ms: value.idle_timeout_ms,
  };
}

function parseAccessEvaluation(payload: unknown): AccessEvaluation {
  if (
    !isRecord(payload) ||
    typeof payload.allowed !== 'boolean' ||
    !Array.isArray(payload.effective_capabilities)
  ) {
    throw invalidResponse('Users & Access returned an invalid access evaluation');
  }
  return {
    allowed: payload.allowed,
    effectiveCapabilities: payload.effective_capabilities.map(parseCapability),
  };
}

function parseUsers(payload: unknown): User[] {
  if (!isRecord(payload) || !Array.isArray(payload.users)) {
    throw invalidResponse('Users & Access response is missing users');
  }
  return payload.users.map(parseUser);
}

function parseUserEnvelope(payload: unknown): { user: User } {
  if (!isRecord(payload) || !('user' in payload)) {
    throw invalidResponse('Users & Access response is missing user');
  }
  return { user: parseUser(payload.user) };
}

function parseUser(value: unknown): User {
  if (
    !isRecord(value) ||
    typeof value.id !== 'string' ||
    value.id.length === 0 ||
    typeof value.login !== 'string' ||
    typeof value.display_name !== 'string' ||
    typeof value.enabled !== 'boolean'
  ) {
    throw invalidResponse('Users & Access returned an invalid user');
  }
  return {
    id: value.id,
    login: value.login,
    display_name: value.display_name,
    enabled: value.enabled,
  };
}

function parsePermissionSets(payload: unknown): PermissionSet[] {
  if (!isRecord(payload) || !Array.isArray(payload.permission_sets)) {
    throw invalidResponse('Users & Access response is missing permission sets');
  }
  return payload.permission_sets.map(parsePermissionSet);
}

function parsePermissionSetEnvelope(payload: unknown): { permission_set: PermissionSet } {
  if (!isRecord(payload) || !('permission_set' in payload)) {
    throw invalidResponse('Users & Access response is missing permission set');
  }
  return { permission_set: parsePermissionSet(payload.permission_set) };
}

function parsePermissionSet(value: unknown): PermissionSet {
  if (
    !isRecord(value) ||
    typeof value.id !== 'string' ||
    value.id.length === 0 ||
    typeof value.name !== 'string' ||
    !Array.isArray(value.capabilities)
  ) {
    throw invalidResponse('Users & Access returned an invalid permission set');
  }
  return {
    id: value.id,
    name: value.name,
    capabilities: value.capabilities.map(parseCapability),
  };
}

function parseAssignments(payload: unknown): AccessAssignment[] {
  if (!isRecord(payload) || !Array.isArray(payload.assignments)) {
    throw invalidResponse('Users & Access response is missing assignments');
  }
  return payload.assignments.map(parseAssignment);
}

function parseAssignmentEnvelope(payload: unknown): { assignment: AccessAssignment } {
  if (!isRecord(payload) || !('assignment' in payload)) {
    throw invalidResponse('Users & Access response is missing assignment');
  }
  return { assignment: parseAssignment(payload.assignment) };
}

function parseAssignment(value: unknown): AccessAssignment {
  if (
    !isRecord(value) ||
    typeof value.user_id !== 'string' ||
    value.user_id.length === 0 ||
    typeof value.permission_set_id !== 'string' ||
    value.permission_set_id.length === 0 ||
    !('scope' in value)
  ) {
    throw invalidResponse('Users & Access returned an invalid assignment');
  }
  return {
    user_id: value.user_id,
    permission_set_id: value.permission_set_id,
    scope: parseScope(value.scope),
  };
}

function parseScope(value: unknown): AccessScope {
  if (!isRecord(value) || typeof value.kind !== 'string') {
    throw invalidResponse('Users & Access returned an invalid scope');
  }
  if (value.kind === 'global' && !('project_id' in value)) {
    return { kind: 'global' };
  }
  if (
    value.kind === 'project' &&
    typeof value.project_id === 'string' &&
    value.project_id.length > 0
  ) {
    return { kind: 'project', project_id: value.project_id };
  }
  throw invalidResponse('Users & Access returned an invalid scope');
}

function parseCapability(value: unknown): CapabilityName {
  if (value === 'view' || value === 'control' || value === 'edit' || value === 'admin') {
    return value;
  }
  throw invalidResponse('Users & Access returned an unknown capability');
}

function parseEmptyResponse(payload: unknown): void {
  if (!isRecord(payload)) {
    throw invalidResponse('Users & Access returned an invalid empty response');
  }
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function isSafeNumber(value: unknown): value is number {
  return typeof value === 'number' && Number.isSafeInteger(value);
}

function invalidResponse(message: string): UsersAccessClientResponseError {
  return new UsersAccessClientResponseError(message);
}
