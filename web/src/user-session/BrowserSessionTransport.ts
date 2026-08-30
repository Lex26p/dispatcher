import type {
  ServiceHubConnectionState,
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';
import { ServiceHubRequestError } from '../service-hub/ServiceHubClient';
import type { ServiceHubClientAccess } from '../service-hub/ServiceHubProvider';

export const USER_SESSION_STORAGE_KEY = 'dispatcher.user-session.v1';

const sessionTokenPattern = /^[0-9a-f]{64}$/;
const usersAccessService = 'users-access.v1';
const loginOperation = 'login';

export type BrowserSessionTokenListener = (token: string | null) => void;

export class BrowserSessionStore {
  private readonly listeners = new Set<BrowserSessionTokenListener>();
  private token: string | null | undefined;

  getToken(): string | null {
    if (this.token !== undefined) {
      return this.token;
    }

    try {
      const stored = window.sessionStorage.getItem(USER_SESSION_STORAGE_KEY);
      if (stored === null) {
        this.token = null;
        return null;
      }
      if (!sessionTokenPattern.test(stored)) {
        window.sessionStorage.removeItem(USER_SESSION_STORAGE_KEY);
        this.token = null;
        return null;
      }
      this.token = stored;
      return stored;
    } catch {
      this.token = null;
      return null;
    }
  }

  setToken(token: string | null): void {
    if (token !== null && !sessionTokenPattern.test(token)) {
      throw new TypeError('Users & Access session token has invalid v1 shape');
    }

    this.token = token;

    try {
      if (token === null) {
        window.sessionStorage.removeItem(USER_SESSION_STORAGE_KEY);
      } else {
        window.sessionStorage.setItem(USER_SESSION_STORAGE_KEY, token);
      }
    } finally {
      for (const listener of this.listeners) {
        listener(token);
      }
    }
  }

  subscribe(listener: BrowserSessionTokenListener): () => void {
    this.listeners.add(listener);
    listener(this.getToken());

    return () => {
      this.listeners.delete(listener);
    };
  }
}

export class BrowserSessionServiceHubClient implements ServiceHubClientAccess {
  constructor(
    private readonly client: ServiceHubClientAccess,
    private readonly sessionStore: BrowserSessionStore,
  ) {}

  get connectionState(): ServiceHubConnectionState {
    return this.client.connectionState;
  }

  subscribeConnectionState(
    listener: (state: ServiceHubConnectionState) => void,
  ): () => void {
    return this.client.subscribeConnectionState(listener);
  }

  connect(): Promise<void> {
    return this.client.connect();
  }

  disconnect(): void {
    this.client.disconnect();
  }

  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options: ServiceHubRequestOptions = {},
  ): ServiceHubRequestHandle<TResponse> {
    const publicLogin = service === usersAccessService && operation === loginOperation;
    const token = publicLogin ? null : this.sessionStore.getToken();
    const authentication =
      options.auth ??
      (token === null
        ? undefined
        : {
            type: 'session' as const,
            token,
          });

    const request = this.client.request<TResponse>(service, operation, payload, {
      ...options,
      auth: authentication,
    });

    return {
      id: request.id,
      response: request.response.catch((error: unknown) => {
        if (
          token !== null &&
          error instanceof ServiceHubRequestError &&
          (error.code === 'auth.invalid_session' ||
            error.code === 'auth.session_expired') &&
          this.sessionStore.getToken() === token
        ) {
          this.sessionStore.setToken(null);
        }
        throw error;
      }),
      cancel: () => request.cancel(),
    };
  }

  cancel(requestId: string): boolean {
    return this.client.cancel(requestId);
  }
}
