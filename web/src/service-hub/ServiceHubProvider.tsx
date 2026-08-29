import {
  createContext,
  type ReactNode,
  useContext,
  useEffect,
  useMemo,
  useState,
} from 'react';

import type {
  ServiceHubConnectionState,
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from './ServiceHubClient';

export interface ServiceHubClientAccess {
  readonly connectionState: ServiceHubConnectionState;

  subscribeConnectionState(
    listener: (state: ServiceHubConnectionState) => void,
  ): () => void;

  connect(): Promise<void>;
  disconnect(): void;

  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse>;

  cancel(requestId: string): boolean;
}

export interface ServiceHubContextValue {
  readonly client: ServiceHubClientAccess;
  readonly connectionState: ServiceHubConnectionState;
}

interface ServiceHubProviderProps {
  readonly client: ServiceHubClientAccess;
  readonly children: ReactNode;
}

const ServiceHubContext = createContext<ServiceHubContextValue | null>(null);

export function ServiceHubProvider({ client, children }: ServiceHubProviderProps) {
  const [connectionState, setConnectionState] = useState<ServiceHubConnectionState>(
    client.connectionState,
  );

  useEffect(() => {
    const unsubscribe = client.subscribeConnectionState(setConnectionState);

    void client.connect().catch(() => {
      // Connection failure is represented by the client's connection state.
      // Step 5 deliberately keeps the shell usable while Service Hub is unavailable.
    });

    return () => {
      unsubscribe();
      client.disconnect();
    };
  }, [client]);

  const value = useMemo<ServiceHubContextValue>(
    () => ({ client, connectionState }),
    [client, connectionState],
  );

  return <ServiceHubContext.Provider value={value}>{children}</ServiceHubContext.Provider>;
}

export function useServiceHub(): ServiceHubContextValue {
  const value = useContext(ServiceHubContext);

  if (value === null) {
    throw new Error('useServiceHub must be used inside ServiceHubProvider');
  }

  return value;
}
