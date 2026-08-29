import { act, render, screen } from '@testing-library/react';

import type {
  ServiceHubConnectionState,
  ServiceHubRequestHandle,
  ServiceHubRequestOptions,
} from './ServiceHubClient';
import {
  type ServiceHubClientAccess,
  ServiceHubProvider,
  useServiceHub,
} from './ServiceHubProvider';

class TestServiceHubClient implements ServiceHubClientAccess {
  connectionState: ServiceHubConnectionState = 'disconnected';
  connectCalls = 0;
  disconnectCalls = 0;

  private readonly listeners = new Set<
    (state: ServiceHubConnectionState) => void
  >();

  subscribeConnectionState(
    listener: (state: ServiceHubConnectionState) => void,
  ): () => void {
    this.listeners.add(listener);
    listener(this.connectionState);

    return () => {
      this.listeners.delete(listener);
    };
  }

  connect(): Promise<void> {
    this.connectCalls += 1;
    return Promise.resolve();
  }

  disconnect(): void {
    this.disconnectCalls += 1;
  }

  request<TResponse = unknown>(
    _service: string,
    _operation: string,
    _payload: unknown,
    _options?: ServiceHubRequestOptions,
  ): ServiceHubRequestHandle<TResponse> {
    throw new Error('request is not used by this test client');
  }

  cancel(_requestId: string): boolean {
    return false;
  }

  publishState(state: ServiceHubConnectionState): void {
    this.connectionState = state;

    for (const listener of this.listeners) {
      listener(state);
    }
  }
}

function Probe() {
  const { client, connectionState } = useServiceHub();

  return (
    <div>
      <span>{connectionState}</span>
      <span>{client instanceof TestServiceHubClient ? 'shared-client' : 'other-client'}</span>
    </div>
  );
}

describe('ServiceHubProvider', () => {
  it('owns the shared client lifecycle and publishes connection state', () => {
    const client = new TestServiceHubClient();
    const view = render(
      <ServiceHubProvider client={client}>
        <Probe />
      </ServiceHubProvider>,
    );

    expect(client.connectCalls).toBe(1);
    expect(screen.getByText('disconnected')).toBeInTheDocument();
    expect(screen.getByText('shared-client')).toBeInTheDocument();

    act(() => {
      client.publishState('connecting');
    });
    expect(screen.getByText('connecting')).toBeInTheDocument();

    act(() => {
      client.publishState('connected');
    });
    expect(screen.getByText('connected')).toBeInTheDocument();

    view.unmount();
    expect(client.disconnectCalls).toBe(1);
  });
});
