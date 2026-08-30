import type {
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';
import {
  type AccessAssignment,
  UsersAccessClient,
  type UsersAccessTransport,
} from './UsersAccessClient';

class TestTransport implements UsersAccessTransport {
  readonly requests: Array<{
    service: string;
    operation: string;
    payload: unknown;
    options?: ServiceHubRequestOptions;
  }> = [];
  responses: unknown[] = [];

  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    this.requests.push({ service, operation, payload, options });
    const response = this.responses.shift();
    return {
      id: `request-${this.requests.length}`,
      response: Promise.resolve(response) as Promise<TResponse>,
      cancel: () => false,
    };
  }
}

describe('UsersAccessClient', () => {
  it('parses login/session and access evaluation responses', async () => {
    const transport = new TestTransport();
    transport.responses.push(
      {
        session_token: 'b'.repeat(64),
        session: {
          user: {
            id: 'user-1',
            login: 'admin',
            display_name: 'Administrator',
            enabled: true,
          },
          issued_at_unix_ms: 10,
          absolute_expires_at_unix_ms: 20,
          idle_timeout_ms: 30,
        },
      },
      {
        allowed: true,
        effective_capabilities: ['view', 'admin'],
      },
    );
    const client = new UsersAccessClient(transport);

    const login = await client.login('admin', 'secret').response;
    const access = await client.evaluateAccess({ kind: 'global' }, 'admin').response;

    expect(login.sessionToken).toBe('b'.repeat(64));
    expect(login.session.user.login).toBe('admin');
    expect(access).toEqual({
      allowed: true,
      effectiveCapabilities: ['view', 'admin'],
    });
    expect(transport.requests.map((request) => request.operation)).toEqual([
      'login',
      'evaluate-access',
    ]);
  });

  it('maps administration operations to the existing v1 payloads', async () => {
    const transport = new TestTransport();
    const assignment: AccessAssignment = {
      user_id: 'user-2',
      permission_set_id: 'permission-1',
      scope: { kind: 'project', project_id: 'project-1' },
    };
    transport.responses.push(
      { users: [] },
      { permission_sets: [] },
      { assignments: [] },
      { assignment },
      {},
    );
    const client = new UsersAccessClient(transport);

    await client.listUsers().response;
    await client.listPermissionSets().response;
    await client.listAccessAssignments().response;
    await client.assignAccess(assignment).response;
    await client.removeAccessAssignment(assignment).response;

    expect(transport.requests).toMatchObject([
      { operation: 'list-users', payload: {} },
      { operation: 'list-permission-sets', payload: {} },
      { operation: 'list-access-assignments', payload: {} },
      { operation: 'assign-access', payload: assignment },
      { operation: 'remove-access-assignment', payload: assignment },
    ]);
  });
});
