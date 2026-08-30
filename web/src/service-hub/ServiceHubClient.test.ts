import {
  SERVICE_HUB_SUBPROTOCOL,
  ServiceHubClient,
  ServiceHubProtocolError,
  ServiceHubRequestError,
  ServiceHubTransportError,
  type ServiceHubWebSocket,
} from './ServiceHubClient';

class FakeWebSocket extends EventTarget {
  protocol = '';
  readyState = 0;
  readonly sent: string[] = [];
  readonly closeCalls: Array<{ code?: number; reason?: string }> = [];

  constructor(
    readonly url: string,
    readonly requestedSubprotocol: string,
  ) {
    super();
  }

  send(data: string): void {
    if (this.readyState !== 1) {
      throw new Error('socket is not open');
    }

    this.sent.push(data);
  }

  close(code?: number, reason?: string): void {
    this.closeCalls.push({ code, reason });
    this.readyState = 2;
  }

  open(protocol = SERVICE_HUB_SUBPROTOCOL): void {
    this.protocol = protocol;
    this.readyState = 1;
    this.dispatchEvent(new Event('open'));
  }

  receive(message: unknown): void {
    this.dispatchEvent(
      new MessageEvent('message', {
        data: typeof message === 'string' ? message : JSON.stringify(message),
      }),
    );
  }

  finishClose(code = 1000, reason = ''): void {
    this.readyState = 3;
    this.dispatchEvent(
      new CloseEvent('close', {
        code,
        reason,
        wasClean: code === 1000,
      }),
    );
  }
}

class FakeWebSocketFactory {
  readonly sockets: FakeWebSocket[] = [];

  create = (url: string, subprotocol: string): ServiceHubWebSocket => {
    const socket = new FakeWebSocket(url, subprotocol);
    this.sockets.push(socket);
    return socket as unknown as ServiceHubWebSocket;
  };

  latest(): FakeWebSocket {
    const socket = this.sockets.at(-1);
    if (!socket) {
      throw new Error('no fake WebSocket was created');
    }
    return socket;
  }
}

async function connectedClient() {
  const factory = new FakeWebSocketFactory();
  const client = new ServiceHubClient({
    url: 'ws://127.0.0.1:8080/v1/ws',
    webSocketFactory: factory.create,
  });

  const connection = client.connect();
  const socket = factory.latest();
  socket.open();
  await connection;

  return { client, factory, socket };
}

describe('ServiceHubClient', () => {
  it('connects with the v1 subprotocol and publishes connection state', async () => {
    const factory = new FakeWebSocketFactory();
    const client = new ServiceHubClient({
      url: 'ws://dispatcher.local/v1/ws',
      webSocketFactory: factory.create,
    });
    const states: string[] = [];
    const unsubscribe = client.subscribeConnectionState((state) => states.push(state));

    const connection = client.connect();
    const socket = factory.latest();

    expect(socket.url).toBe('ws://dispatcher.local/v1/ws');
    expect(socket.requestedSubprotocol).toBe(SERVICE_HUB_SUBPROTOCOL);
    expect(client.connectionState).toBe('connecting');

    socket.open();
    await expect(connection).resolves.toBeUndefined();

    expect(client.connectionState).toBe('connected');
    expect(states).toEqual(['disconnected', 'connecting', 'connected']);

    unsubscribe();
  });

  it('correlates parallel responses even when they arrive out of order', async () => {
    const { client, socket } = await connectedClient();

    const first = client.request<{ order: number }>(
      'test.echo',
      'echo',
      { order: 1 },
      { timeoutMs: 2500 },
    );
    const second = client.request<{ order: number }>('test.echo', 'echo', {
      order: 2,
    });

    expect(first.id).not.toBe(second.id);
    expect(JSON.parse(socket.sent[0] ?? '')).toEqual({
      type: 'request',
      id: first.id,
      service: 'test.echo',
      operation: 'echo',
      payload: { order: 1 },
      timeout_ms: 2500,
    });
    expect(JSON.parse(socket.sent[1] ?? '')).toEqual({
      type: 'request',
      id: second.id,
      service: 'test.echo',
      operation: 'echo',
      payload: { order: 2 },
    });

    socket.receive({
      type: 'response',
      id: second.id,
      ok: true,
      payload: { order: 2 },
    });
    socket.receive({
      type: 'response',
      id: first.id,
      ok: true,
      payload: { order: 1 },
    });

    await expect(second.response).resolves.toEqual({ order: 2 });
    await expect(first.response).resolves.toEqual({ order: 1 });
  });


  it('adds validated session authentication without changing the business payload', async () => {
    const { client, socket } = await connectedClient();
    const token = 'a'.repeat(64);

    const request = client.request(
      'users-access.v1',
      'current-session',
      {},
      {
        auth: {
          type: 'session',
          token,
        },
      },
    );

    expect(JSON.parse(socket.sent[0] ?? '')).toEqual({
      type: 'request',
      id: request.id,
      service: 'users-access.v1',
      operation: 'current-session',
      payload: {},
      auth: {
        type: 'session',
        token,
      },
    });

    expect(() =>
      client.request('users-access.v1', 'current-session', {}, {
        auth: {
          type: 'session',
          token: 'not-a-session-token',
        },
      }),
    ).toThrow(TypeError);
  });

  it('distinguishes Hub request errors from transport failures', async () => {
    const { client, socket } = await connectedClient();
    const request = client.request('missing.service', 'read', null);

    socket.receive({
      type: 'response',
      id: request.id,
      ok: false,
      error: {
        code: 'hub.unknown_service',
        message: 'No active provider is registered for the requested service',
        details: { service: 'missing.service' },
      },
    });

    await expect(request.response).rejects.toMatchObject({
      name: 'ServiceHubRequestError',
      code: 'hub.unknown_service',
      requestId: request.id,
      details: { service: 'missing.service' },
    });
    await expect(request.response).rejects.toBeInstanceOf(ServiceHubRequestError);

    const pending = client.request('test.echo', 'echo', { text: 'pending' });
    socket.finishClose(1006, 'connection lost');

    await expect(pending.response).rejects.toBeInstanceOf(ServiceHubTransportError);
    await expect(pending.response).rejects.toMatchObject({
      code: 'transport.closed',
    });
    expect(client.connectionState).toBe('disconnected');
  });

  it('sends cancel once and resolves cancellation from the Hub response', async () => {
    const { client, socket } = await connectedClient();
    const request = client.request('test.echo', 'slow', { delay: 1000 });

    expect(request.cancel()).toBe(true);
    expect(request.cancel()).toBe(false);
    expect(JSON.parse(socket.sent[1] ?? '')).toEqual({
      type: 'cancel',
      id: request.id,
    });

    socket.receive({
      type: 'response',
      id: request.id,
      ok: false,
      error: {
        code: 'hub.cancelled',
        message: 'Request was cancelled',
      },
    });

    await expect(request.response).rejects.toMatchObject({
      name: 'ServiceHubRequestError',
      code: 'hub.cancelled',
    });
  });

  it('rejects requests while disconnected and closes on invalid protocol messages', async () => {
    const factory = new FakeWebSocketFactory();
    const client = new ServiceHubClient({
      url: 'ws://dispatcher.local/v1/ws',
      webSocketFactory: factory.create,
    });

    expect(() => client.request('test.echo', 'echo', null)).toThrow(
      ServiceHubTransportError,
    );

    const connection = client.connect();
    const socket = factory.latest();
    socket.open();
    await connection;

    const pending = client.request('test.echo', 'echo', null);
    socket.receive({ type: 'unexpected' });

    await expect(pending.response).rejects.toBeInstanceOf(ServiceHubProtocolError);
    expect(client.connectionState).toBe('disconnecting');
    expect(socket.closeCalls.at(-1)).toEqual({
      code: 1002,
      reason: 'Service Hub protocol error',
    });
  });
});
