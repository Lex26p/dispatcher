import type {
  ServiceHubConnectionState,
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';
import { ServiceHubRequestError } from '../service-hub/ServiceHubClient';
import type { ServiceHubClientAccess } from '../service-hub/ServiceHubProvider';
import {
  BrowserSessionServiceHubClient,
  BrowserSessionStore,
  USER_SESSION_STORAGE_KEY,
} from './BrowserSessionTransport';

const token = 'a'.repeat(64);

class TestClient implements ServiceHubClientAccess {
  connectionState: ServiceHubConnectionState = 'connected';
  readonly requests: Array<{
    service: string;
    operation: string;
    options: ServiceHubRequestOptions | undefined;
  }> = [];
  nextResponse: Promise<unknown> = Promise.resolve({});

  subscribeConnectionState(): () => void {
    return () => {};
  }
  connect(): Promise<void> {
    return Promise.resolve();
  }
  disconnect(): void {}
  cancel(): boolean {
    return false;
  }

  request<TResponse = unknown>(
    service: string,
    operation: string,
    _payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    this.requests.push({ service, operation, options });
    return {
      id: `request-${this.requests.length}`,
      response: this.nextResponse as Promise<TResponse>,
      cancel: () => false,
    };
  }
}

describe('BrowserSessionServiceHubClient', () => {
  beforeEach(() => {
    window.sessionStorage.clear();
  });

  it('injects the stored session into protected requests but keeps login public', async () => {
    const base = new TestClient();
    const store = new BrowserSessionStore();
    store.setToken(token);
    const client = new BrowserSessionServiceHubClient(base, store);

    await client.request('project-manager.v1', 'list-projects', {}).response;
    await client.request('users-access.v1', 'login', {
      login: 'admin',
      password: 'not-recorded-here',
    }).response;

    expect(base.requests[0]?.options?.auth).toEqual({
      type: 'session',
      token,
    });
    expect(base.requests[1]?.options?.auth).toBeUndefined();
  });

  it('clears the matching browser session after an authoritative invalid-session response', async () => {
    const base = new TestClient();
    const store = new BrowserSessionStore();
    store.setToken(token);
    base.nextResponse = Promise.reject(
      new ServiceHubRequestError('request-1', {
        code: 'auth.invalid_session',
        message: 'invalid session',
      }),
    );
    const client = new BrowserSessionServiceHubClient(base, store);

    await expect(
      client.request('project-manager.v1', 'list-projects', {}).response,
    ).rejects.toMatchObject({ code: 'auth.invalid_session' });

    expect(store.getToken()).toBeNull();
    expect(window.sessionStorage.getItem(USER_SESSION_STORAGE_KEY)).toBeNull();
  });
});
