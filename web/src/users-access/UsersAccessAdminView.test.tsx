import { fireEvent, render, screen, within } from '@testing-library/react';

import type {
  ServiceHubConnectionState,
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from '../service-hub/ServiceHubClient';
import {
  type ServiceHubClientAccess,
  ServiceHubProvider,
} from '../service-hub/ServiceHubProvider';
import {
  BrowserSessionServiceHubClient,
  BrowserSessionStore,
} from '../user-session/BrowserSessionTransport';
import { UserSessionProvider } from '../user-session/UserSessionProvider';
import { UsersAccessAdminView } from './UsersAccessAdminView';

const token = 'e'.repeat(64);
const adminSession = {
  user: {
    id: 'admin-user',
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
  private users = [adminSession.user];
  private permissionSets = [
    {
      id: 'permission-admin',
      name: 'Administrators',
      capabilities: ['view', 'control', 'edit', 'admin'],
    },
  ];

  subscribeConnectionState(listener: (state: ServiceHubConnectionState) => void): () => void {
    listener(this.connectionState);
    return () => {};
  }
  connect(): Promise<void> { return Promise.resolve(); }
  disconnect(): void {}
  cancel(): boolean { return false; }

  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    expect(service).toBe('users-access.v1');
    if (operation !== 'login') {
      expect(options?.auth?.token).toBe(token);
    }

    if (operation === 'current-session') {
      return this.handle<TResponse>({ session: adminSession });
    }
    if (operation === 'evaluate-access') {
      return this.handle<TResponse>({
        allowed: true,
        effective_capabilities: ['view', 'control', 'edit', 'admin'],
      });
    }
    if (operation === 'list-users') {
      return this.handle<TResponse>({ users: this.users });
    }
    if (operation === 'list-permission-sets') {
      return this.handle<TResponse>({ permission_sets: this.permissionSets });
    }
    if (operation === 'list-access-assignments') {
      return this.handle<TResponse>({ assignments: [] });
    }
    if (operation === 'create-user') {
      const input = payload as {
        login: string;
        display_name: string;
        enabled: boolean;
      };
      const user = {
        id: 'user-2',
        login: input.login,
        display_name: input.display_name,
        enabled: input.enabled,
      };
      this.users = [...this.users, user];
      return this.handle<TResponse>({ user });
    }
    if (operation === 'create-permission-set') {
      const input = payload as { name: string; capabilities: string[] };
      const permissionSet = {
        id: 'permission-2',
        name: input.name,
        capabilities: input.capabilities,
      };
      this.permissionSets = [...this.permissionSets, permissionSet];
      return this.handle<TResponse>({ permission_set: permissionSet });
    }

    throw new Error(`Unexpected operation ${operation}`);
  }

  private handle<TResponse>(payload: unknown): ServiceHubRequestHandle<TResponse> {
    return {
      id: 'request',
      response: Promise.resolve(payload) as Promise<TResponse>,
      cancel: () => false,
    };
  }
}

function renderView() {
  const base = new TestClient();
  const store = new BrowserSessionStore();
  store.setToken(token);
  const client = new BrowserSessionServiceHubClient(base, store);
  render(
    <ServiceHubProvider client={client}>
      <UserSessionProvider sessionStore={store}>
        <UsersAccessAdminView />
      </UserSessionProvider>
    </ServiceHubProvider>,
  );
}

describe('UsersAccessAdminView', () => {
  beforeEach(() => {
    window.sessionStorage.clear();
  });

  it('loads administration data and performs user and permission-set creation', async () => {
    renderView();

    const usersList = await screen.findByRole('list', { name: 'Пользователи' });
    expect(await within(usersList).findByText('Administrator')).toBeInTheDocument();

    fireEvent.change(screen.getByLabelText('Логин'), {
      target: { value: 'engineer' },
    });
    fireEvent.change(screen.getByLabelText('Отображаемое имя'), {
      target: { value: 'Engineer' },
    });
    fireEvent.change(screen.getByLabelText('Начальный пароль'), {
      target: { value: 'engineer password 123' },
    });
    fireEvent.click(screen.getByRole('button', { name: 'Создать пользователя' }));

    expect(await within(usersList).findByText('Engineer')).toBeInTheDocument();

    fireEvent.change(screen.getByLabelText('Название набора'), {
      target: { value: 'Project viewers' },
    });
    fireEvent.click(screen.getByLabelText('Просмотр (view)'));
    fireEvent.click(screen.getByRole('button', { name: 'Создать набор прав' }));

    const permissionList = screen.getByRole('list', { name: 'Наборы прав' });
    expect(await within(permissionList).findByText('Project viewers')).toBeInTheDocument();
  });
});
