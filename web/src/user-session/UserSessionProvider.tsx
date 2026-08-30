import {
  createContext,
  type ReactNode,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
} from 'react';

import {
  ServiceHubRequestError,
  ServiceHubTransportError,
} from '../service-hub/ServiceHubClient';
import { useServiceHub } from '../service-hub/ServiceHubProvider';
import {
  type CapabilityName,
  type SessionSummary,
  UsersAccessClient,
  UsersAccessClientResponseError,
} from '../users-access/UsersAccessClient';
import { BrowserSessionStore } from './BrowserSessionTransport';

export type UserSessionStatus =
  | 'unauthenticated'
  | 'restoring'
  | 'authenticated';

export interface UserSessionContextValue {
  readonly status: UserSessionStatus;
  readonly session: SessionSummary | null;
  readonly globalCapabilities: CapabilityName[];
  readonly restorationError: string | null;
  readonly sessionStore: BrowserSessionStore;
  login(login: string, password: string): Promise<void>;
  logout(): Promise<void>;
  forgetSession(): void;
  retryRestoration(): void;
  hasGlobalCapability(capability: CapabilityName): boolean;
}

interface UserSessionProviderProps {
  readonly sessionStore: BrowserSessionStore;
  readonly children: ReactNode;
}

const UserSessionContext = createContext<UserSessionContextValue | null>(null);

export function UserSessionProvider({
  sessionStore,
  children,
}: UserSessionProviderProps) {
  const { client, connectionState } = useServiceHub();
  const usersAccess = useMemo(() => new UsersAccessClient(client), [client]);
  const initialToken = sessionStore.getToken();
  const [token, setToken] = useState<string | null>(initialToken);
  const [status, setStatus] = useState<UserSessionStatus>(
    initialToken === null ? 'unauthenticated' : 'restoring',
  );
  const [session, setSession] = useState<SessionSummary | null>(null);
  const [globalCapabilities, setGlobalCapabilities] = useState<CapabilityName[]>([]);
  const [restorationError, setRestorationError] = useState<string | null>(null);
  const [restoreNumber, setRestoreNumber] = useState(0);

  const clearLocalSession = useCallback(() => {
    setSession(null);
    setGlobalCapabilities([]);
    setRestorationError(null);
    setStatus('unauthenticated');
    sessionStore.setToken(null);
  }, [sessionStore]);

  useEffect(() => sessionStore.subscribe((nextToken) => {
    setToken(nextToken);
    if (nextToken === null) {
      setSession(null);
      setGlobalCapabilities([]);
      setStatus('unauthenticated');
    } else {
      setStatus((current) =>
        current === 'authenticated' ? current : 'restoring',
      );
    }
  }), [sessionStore]);

  useEffect(() => {
    if (token === null) {
      setRestorationError(null);
      setStatus('unauthenticated');
      return;
    }

    if (connectionState !== 'connected') {
      return;
    }

    let active = true;
    const requests: Array<{ cancel(): boolean }> = [];
    const currentSessionRequest = usersAccess.currentSession();
    requests.push(currentSessionRequest);

    setStatus('restoring');
    setRestorationError(null);

    void currentSessionRequest.response
      .then((restoredSession) => {
        if (!active) {
          return;
        }

        setSession(restoredSession);
        setStatus('authenticated');

        const accessRequest = usersAccess.evaluateAccess({ kind: 'global' }, 'admin');
        requests.push(accessRequest);
        void accessRequest.response
          .then((evaluation) => {
            if (active) {
              setGlobalCapabilities(evaluation.effectiveCapabilities);
            }
          })
          .catch((error: unknown) => {
            if (!active) {
              return;
            }
            if (isInvalidSessionError(error)) {
              clearLocalSession();
              return;
            }
            setGlobalCapabilities([]);
          });
      })
      .catch((error: unknown) => {
        if (!active) {
          return;
        }

        if (isInvalidSessionError(error)) {
          clearLocalSession();
          return;
        }

        setStatus('restoring');
        setRestorationError(userSessionErrorMessage(error));
      });

    return () => {
      active = false;
      for (const request of requests) {
        try {
          request.cancel();
        } catch {
          // Connection teardown already resolves pending transport state.
        }
      }
    };
  }, [
    clearLocalSession,
    connectionState,
    restoreNumber,
    token,
    usersAccess,
  ]);

  const login = useCallback(
    async (loginValue: string, password: string) => {
      const request = usersAccess.login(loginValue, password);
      const result = await request.response;

      sessionStore.setToken(result.sessionToken);
      setSession(result.session);
      setStatus('authenticated');
      setRestorationError(null);

      try {
        const accessRequest = usersAccess.evaluateAccess({ kind: 'global' }, 'admin');
        const evaluation = await accessRequest.response;
        setGlobalCapabilities(evaluation.effectiveCapabilities);
      } catch (error) {
        if (isInvalidSessionError(error)) {
          clearLocalSession();
          throw error;
        }
        setGlobalCapabilities([]);
      }
    },
    [clearLocalSession, sessionStore, usersAccess],
  );

  const logout = useCallback(async () => {
    const hadToken = sessionStore.getToken() !== null;
    let failure: unknown = null;

    if (hadToken && connectionState === 'connected') {
      try {
        const request = usersAccess.logout();
        await request.response;
      } catch (error) {
        failure = error;
      }
    }

    clearLocalSession();

    if (failure !== null && !isInvalidSessionError(failure)) {
      throw failure;
    }
  }, [clearLocalSession, connectionState, sessionStore, usersAccess]);

  const forgetSession = useCallback(() => {
    clearLocalSession();
  }, [clearLocalSession]);

  const retryRestoration = useCallback(() => {
    if (sessionStore.getToken() !== null) {
      setRestoreNumber((value) => value + 1);
    }
  }, [sessionStore]);

  const hasGlobalCapability = useCallback(
    (capability: CapabilityName) => globalCapabilities.includes(capability),
    [globalCapabilities],
  );

  const value = useMemo<UserSessionContextValue>(
    () => ({
      status,
      session,
      globalCapabilities,
      restorationError,
      sessionStore,
      login,
      logout,
      forgetSession,
      retryRestoration,
      hasGlobalCapability,
    }),
    [
      forgetSession,
      globalCapabilities,
      hasGlobalCapability,
      login,
      logout,
      restorationError,
      retryRestoration,
      session,
      sessionStore,
      status,
    ],
  );

  return <UserSessionContext.Provider value={value}>{children}</UserSessionContext.Provider>;
}

export function useUserSession(): UserSessionContextValue {
  const value = useContext(UserSessionContext);

  if (value === null) {
    throw new Error('useUserSession must be used inside UserSessionProvider');
  }

  return value;
}

export function useOptionalUserSession(): UserSessionContextValue | null {
  return useContext(UserSessionContext);
}

export function userSessionErrorMessage(error: unknown): string {
  if (error instanceof UsersAccessClientResponseError) {
    return 'Users & Access вернул некорректный ответ.';
  }

  if (error instanceof ServiceHubTransportError) {
    return 'Service Hub недоступен. Не удалось выполнить запрос Users & Access.';
  }

  if (error instanceof ServiceHubRequestError) {
    switch (error.code) {
      case 'auth.invalid_credentials':
        return 'Неверный логин или пароль.';
      case 'auth.invalid_session':
        return 'Сессия недействительна. Выполните вход снова.';
      case 'auth.session_expired':
        return 'Сессия истекла. Выполните вход снова.';
      case 'access.forbidden':
        return 'Недостаточно прав для этого действия.';
      case 'hub.unknown_service':
      case 'hub.provider_unavailable':
        return 'Users & Access недоступен.';
      case 'hub.timeout':
        return 'Users & Access не ответил вовремя.';
      case 'access.invalid_request':
        return 'Users & Access отклонил некорректный запрос.';
      case 'access.conflict':
        return 'Users & Access отклонил изменение из-за конфликта данных.';
      case 'access.user_not_found':
        return 'Пользователь больше не существует.';
      case 'access.permission_set_not_found':
        return 'Набор прав больше не существует.';
      case 'access.storage_error':
        return 'Users & Access не смог сохранить данные.';
      default:
        return `Users & Access вернул ошибку: ${error.code}.`;
    }
  }

  return 'Не удалось выполнить запрос Users & Access.';
}

function isInvalidSessionError(error: unknown): boolean {
  return (
    error instanceof ServiceHubRequestError &&
    (error.code === 'auth.invalid_session' || error.code === 'auth.session_expired')
  );
}
