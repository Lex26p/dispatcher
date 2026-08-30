import { act, render, screen } from '@testing-library/react';

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
  BrowserSessionServiceHubClient,
  BrowserSessionStore,
  USER_SESSION_STORAGE_KEY,
} from './BrowserSessionTransport';
import { UserSessionProvider, useUserSession } from './UserSessionProvider';

const token = 'c'.repeat(64);

const session = {
  user: {
    id: 'user-1',
    login: 'admin',
    display_name: 'Administrator',
    enabled: true,
  },
  issued_at_unix_ms: 10,
  absolute_expires_at_unix_ms: 20,
  idle_timeout_ms: 30,
};

class TestClient implements ServiceHubClientAccess {
  connectionState: ServiceHubConnectionState = 'connected';
  invalidSession = false;

  subscribeConnectionState(listener: (state: ServiceHubConnectionState) => void): () => void {
    listener(this.connectionState);
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
    expect(service).toBe('users-access.v1');

    if (operation === 'current-session') {
      if (this.invalidSession) {
        return this.handle<TResponse>(Promise.reject(
          new ServiceHubRequestError('current', {
            code: 'auth.invalid_session',
            message: 'invalid session',
          }),
        ));
      }
      expect(options?.auth?.token).toBe(token);
      return this.handle<TResponse>(Promise.resolve({ session }));
    }

    if (operation === 'evaluate-access') {
      return this.handle<TResponse>(Promise.resolve({
        allowed: true,
        effective_capabilities: ['view', 'control', 'edit', 'admin'],
      }));
    }

    throw new Error(`Unexpected operation ${operation}`);
  }

  private handle<TResponse>(response: Promise<unknown>): ServiceHubRequestHandle<TResponse> {
    return {
      id: 'request',
      response: response as Promise<TResponse>,
      cancel: () => false,
    };
  }
}

function Probe() {
  const value = useUserSession();
  return (
    <div>
      <span data-testid="status">{value.status}</span>
      <span data-testid="user">{value.session?.user.login ?? '-'}</span>
      <span data-testid="admin">{String(value.hasGlobalCapability('admin'))}</span>
    </div>
  );
}

function renderProvider(base: TestClient, store: BrowserSessionStore) {
  const client = new BrowserSessionServiceHubClient(base, store);
  render(
    <ServiceHubProvider client={client}>
      <UserSessionProvider sessionStore={store}>
        <Probe />
      </UserSessionProvider>
    </ServiceHubProvider>,
  );
}

describe('UserSessionProvider', () => {
  beforeEach(() => {
    window.sessionStorage.clear();
  });

  it('restores a browser session through authoritative current-session', async () => {
    const base = new TestClient();
    const store = new BrowserSessionStore();
    store.setToken(token);

    renderProvider(base, store);

    expect(await screen.findByText('authenticated')).toBeInTheDocument();
    expect(screen.getByTestId('user')).toHaveTextContent('admin');
    expect(screen.getByTestId('admin')).toHaveTextContent('true');
  });

  it('clears persisted state after authoritative invalid-session', async () => {
    const base = new TestClient();
    base.invalidSession = true;
    const store = new BrowserSessionStore();
    store.setToken(token);

    renderProvider(base, store);

    await act(async () => {
      await Promise.resolve();
      await Promise.resolve();
    });

    expect(await screen.findByText('unauthenticated')).toBeInTheDocument();
    expect(store.getToken()).toBeNull();
    expect(window.sessionStorage.getItem(USER_SESSION_STORAGE_KEY)).toBeNull();
  });
});
