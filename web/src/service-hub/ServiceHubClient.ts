export const SERVICE_HUB_SUBPROTOCOL = 'dispatcher.service-hub.v1';

const webSocketConnectingState = 0;
const webSocketOpenState = 1;
const serviceNamePattern = /^[a-z0-9][a-z0-9._-]{0,127}$/;

export type ServiceHubConnectionState =
  | 'disconnected'
  | 'connecting'
  | 'connected'
  | 'disconnecting';

export interface ServiceHubErrorPayload {
  code: string;
  message: string;
  details?: unknown;
}

export interface ServiceHubRequestOptions {
  timeoutMs?: number;
}

export interface ServiceHubRequestHandle<TResponse = unknown> {
  readonly id: string;
  readonly response: Promise<TResponse>;
  cancel(): boolean;
}

export interface ServiceHubWebSocket {
  readonly protocol: string;
  readonly readyState: number;
  send(data: string): void;
  close(code?: number, reason?: string): void;
  addEventListener(type: 'open', listener: (event: Event) => void): void;
  addEventListener(type: 'message', listener: (event: MessageEvent<unknown>) => void): void;
  addEventListener(type: 'close', listener: (event: CloseEvent) => void): void;
  addEventListener(type: 'error', listener: (event: Event) => void): void;
}

export type ServiceHubWebSocketFactory = (
  url: string,
  subprotocol: string,
) => ServiceHubWebSocket;

export interface ServiceHubClientOptions {
  url: string;
  webSocketFactory?: ServiceHubWebSocketFactory;
}

interface PendingRequest<TResponse = unknown> {
  resolve(value: TResponse): void;
  reject(reason: unknown): void;
  cancelRequested: boolean;
}

type ResponseSuccessMessage = Record<string, unknown> & {
  type: 'response';
  id: string;
  ok: true;
  payload: unknown;
};

type ResponseErrorMessage = Record<string, unknown> & {
  type: 'response';
  id: string;
  ok: false;
  error: ServiceHubErrorPayload;
};

type ProtocolErrorMessage = Record<string, unknown> & {
  type: 'protocol_error';
  error: ServiceHubErrorPayload;
};

export class ServiceHubRequestError extends Error {
  readonly requestId: string;
  readonly code: string;
  readonly details?: unknown;

  constructor(requestId: string, error: ServiceHubErrorPayload) {
    super(error.message);
    this.name = 'ServiceHubRequestError';
    this.requestId = requestId;
    this.code = error.code;
    this.details = error.details;
  }
}

export class ServiceHubTransportError extends Error {
  readonly code: string;

  constructor(code: string, message: string, options?: ErrorOptions) {
    super(message, options);
    this.name = 'ServiceHubTransportError';
    this.code = code;
  }
}

export class ServiceHubProtocolError extends Error {
  readonly code: string;
  readonly details?: unknown;

  constructor(error: ServiceHubErrorPayload) {
    super(error.message);
    this.name = 'ServiceHubProtocolError';
    this.code = error.code;
    this.details = error.details;
  }
}

export class ServiceHubClient {
  readonly url: string;

  private readonly webSocketFactory: ServiceHubWebSocketFactory;
  private readonly stateListeners = new Set<
    (state: ServiceHubConnectionState) => void
  >();
  private readonly pendingRequests = new Map<string, PendingRequest>();

  private socket: ServiceHubWebSocket | null = null;
  private state: ServiceHubConnectionState = 'disconnected';
  private connectionPromise: Promise<void> | null = null;
  private nextRequestNumber = 1;

  constructor(options: ServiceHubClientOptions) {
    if (options.url.trim().length === 0) {
      throw new TypeError('Service Hub URL must not be empty');
    }

    this.url = options.url;
    this.webSocketFactory =
      options.webSocketFactory ??
      ((url, subprotocol) =>
        new WebSocket(url, subprotocol) as unknown as ServiceHubWebSocket);
  }

  get connectionState(): ServiceHubConnectionState {
    return this.state;
  }

  subscribeConnectionState(
    listener: (state: ServiceHubConnectionState) => void,
  ): () => void {
    this.stateListeners.add(listener);
    listener(this.state);

    return () => {
      this.stateListeners.delete(listener);
    };
  }

  connect(): Promise<void> {
    if (this.state === 'connected') {
      return Promise.resolve();
    }

    if (this.state === 'connecting' && this.connectionPromise) {
      return this.connectionPromise;
    }

    if (this.state === 'disconnecting') {
      return Promise.reject(
        new ServiceHubTransportError(
          'transport.disconnecting',
          'Service Hub connection is disconnecting',
        ),
      );
    }

    let socket: ServiceHubWebSocket;

    try {
      socket = this.webSocketFactory(this.url, SERVICE_HUB_SUBPROTOCOL);
    } catch (error) {
      return Promise.reject(
        new ServiceHubTransportError(
          'transport.connect_failed',
          'Failed to create Service Hub WebSocket',
          { cause: error },
        ),
      );
    }

    this.socket = socket;
    this.setState('connecting');

    let resolveConnection: () => void;
    let rejectConnection: (reason: unknown) => void;

    const promise = new Promise<void>((resolve, reject) => {
      resolveConnection = resolve;
      rejectConnection = reject;
    });

    this.connectionPromise = promise;
    let connectionSettled = false;

    const settleConnected = () => {
      if (connectionSettled) {
        return;
      }

      connectionSettled = true;
      this.connectionPromise = null;
      resolveConnection();
    };

    const settleConnectFailed = (error: unknown) => {
      if (connectionSettled) {
        return;
      }

      connectionSettled = true;
      this.connectionPromise = null;
      rejectConnection(error);
    };

    socket.addEventListener('open', () => {
      if (this.socket !== socket) {
        return;
      }

      if (socket.protocol !== SERVICE_HUB_SUBPROTOCOL) {
        const error = new ServiceHubProtocolError({
          code: 'hub.protocol_error',
          message: `Service Hub negotiated unexpected WebSocket subprotocol: ${socket.protocol || '<none>'}`,
        });

        settleConnectFailed(error);
        this.failAllPending(error);
        this.setState('disconnecting');
        socket.close(1002, 'Unexpected Service Hub subprotocol');
        return;
      }

      this.setState('connected');
      settleConnected();
    });

    socket.addEventListener('message', (event) => {
      if (this.socket !== socket) {
        return;
      }

      this.handleMessage(socket, event.data);
    });

    socket.addEventListener('close', (event) => {
      if (this.socket !== socket) {
        return;
      }

      const error = new ServiceHubTransportError(
        'transport.closed',
        formatCloseMessage(event.code, event.reason),
      );

      settleConnectFailed(error);
      this.failAllPending(error);
      this.socket = null;
      this.setState('disconnected');
    });

    socket.addEventListener('error', () => {
      if (this.socket !== socket) {
        return;
      }

      if (socket.readyState === webSocketConnectingState) {
        settleConnectFailed(
          new ServiceHubTransportError(
            'transport.connect_failed',
            'Service Hub WebSocket connection failed',
          ),
        );
      }
    });

    return promise;
  }

  disconnect(): void {
    const socket = this.socket;

    if (!socket) {
      this.setState('disconnected');
      return;
    }

    if (this.state === 'disconnecting') {
      return;
    }

    this.setState('disconnecting');
    socket.close(1000, 'Client disconnect');
  }

  request<TResponse = unknown>(
    service: string,
    operation: string,
    payload: unknown,
    options: ServiceHubRequestOptions = {},
  ): ServiceHubRequestHandle<TResponse> {
    const socket = this.requireConnectedSocket();
    validateServiceName('service', service);
    validateServiceName('operation', operation);
    validateTimeout(options.timeoutMs);

    const id = this.createRequestId();
    const message: Record<string, unknown> = {
      type: 'request',
      id,
      service,
      operation,
      payload,
    };

    if (options.timeoutMs !== undefined) {
      message.timeout_ms = options.timeoutMs;
    }

    let serialized: string;

    try {
      serialized = JSON.stringify(message);
    } catch (error) {
      throw new TypeError('Service Hub request payload must be JSON-serializable', {
        cause: error,
      });
    }

    let resolveResponse!: (value: TResponse) => void;
    let rejectResponse!: (reason: unknown) => void;

    const response = new Promise<TResponse>((resolve, reject) => {
      resolveResponse = resolve;
      rejectResponse = reject;
    });

    this.pendingRequests.set(id, {
      resolve: resolveResponse as (value: unknown) => void,
      reject: rejectResponse,
      cancelRequested: false,
    });

    try {
      socket.send(serialized);
    } catch (error) {
      this.pendingRequests.delete(id);
      throw new ServiceHubTransportError(
        'transport.send_failed',
        'Failed to send Service Hub request',
        { cause: error },
      );
    }

    return {
      id,
      response,
      cancel: () => this.cancel(id),
    };
  }

  cancel(requestId: string): boolean {
    const pending = this.pendingRequests.get(requestId);

    if (!pending || pending.cancelRequested) {
      return false;
    }

    const socket = this.requireConnectedSocket();
    const serialized = JSON.stringify({
      type: 'cancel',
      id: requestId,
    });

    try {
      socket.send(serialized);
    } catch (error) {
      throw new ServiceHubTransportError(
        'transport.send_failed',
        'Failed to send Service Hub cancellation',
        { cause: error },
      );
    }

    pending.cancelRequested = true;
    return true;
  }

  private requireConnectedSocket(): ServiceHubWebSocket {
    if (
      this.state !== 'connected' ||
      !this.socket ||
      this.socket.readyState !== webSocketOpenState
    ) {
      throw new ServiceHubTransportError(
        'transport.not_connected',
        'Service Hub client is not connected',
      );
    }

    return this.socket;
  }

  private createRequestId(): string {
    const id = `web-${this.nextRequestNumber.toString(36)}`;
    this.nextRequestNumber += 1;
    return id;
  }

  private handleMessage(socket: ServiceHubWebSocket, data: unknown): void {
    if (typeof data !== 'string') {
      this.handleProtocolFailure(socket, {
        code: 'hub.protocol_error',
        message: 'Service Hub sent a non-text application message',
      });
      return;
    }

    let parsed: unknown;

    try {
      parsed = JSON.parse(data);
    } catch {
      this.handleProtocolFailure(socket, {
        code: 'hub.protocol_error',
        message: 'Service Hub sent invalid JSON',
      });
      return;
    }

    if (!isRecord(parsed) || typeof parsed.type !== 'string') {
      this.handleProtocolFailure(socket, {
        code: 'hub.protocol_error',
        message: 'Service Hub sent an invalid message envelope',
      });
      return;
    }

    if (parsed.type === 'response') {
      this.handleResponse(socket, parsed);
      return;
    }

    if (parsed.type === 'protocol_error' && isProtocolErrorMessage(parsed)) {
      this.handleProtocolFailure(socket, parsed.error);
      return;
    }

    this.handleProtocolFailure(socket, {
      code: 'hub.protocol_error',
      message: `Unexpected Service Hub message type: ${parsed.type}`,
    });
  }

  private handleResponse(
    socket: ServiceHubWebSocket,
    message: Record<string, unknown>,
  ): void {
    if (typeof message.id !== 'string') {
      this.handleProtocolFailure(socket, {
        code: 'hub.protocol_error',
        message: 'Service Hub response is missing a valid request id',
      });
      return;
    }

    const pending = this.pendingRequests.get(message.id);

    if (!pending) {
      this.handleProtocolFailure(socket, {
        code: 'hub.protocol_error',
        message: `Service Hub response references unknown request id: ${message.id}`,
      });
      return;
    }

    if (isResponseSuccessMessage(message)) {
      this.pendingRequests.delete(message.id);
      pending.resolve(message.payload);
      return;
    }

    if (isResponseErrorMessage(message)) {
      this.pendingRequests.delete(message.id);
      pending.reject(new ServiceHubRequestError(message.id, message.error));
      return;
    }

    this.handleProtocolFailure(socket, {
      code: 'hub.protocol_error',
      message: `Service Hub response for ${message.id} has an invalid shape`,
    });
  }

  private handleProtocolFailure(
    socket: ServiceHubWebSocket,
    errorPayload: ServiceHubErrorPayload,
  ): void {
    const error = new ServiceHubProtocolError(errorPayload);
    this.failAllPending(error);

    if (this.socket === socket) {
      this.setState('disconnecting');
      socket.close(1002, 'Service Hub protocol error');
    }
  }

  private failAllPending(error: unknown): void {
    if (this.pendingRequests.size === 0) {
      return;
    }

    const pending = [...this.pendingRequests.values()];
    this.pendingRequests.clear();

    for (const request of pending) {
      request.reject(error);
    }
  }

  private setState(state: ServiceHubConnectionState): void {
    if (this.state === state) {
      return;
    }

    this.state = state;

    for (const listener of this.stateListeners) {
      listener(state);
    }
  }
}

function validateServiceName(label: string, value: string): void {
  if (!serviceNamePattern.test(value)) {
    throw new TypeError(
      `${label} must match the Service Hub v1 name pattern and be 1..128 characters`,
    );
  }
}

function validateTimeout(timeoutMs: number | undefined): void {
  if (timeoutMs === undefined) {
    return;
  }

  if (!Number.isInteger(timeoutMs) || timeoutMs < 1 || timeoutMs > 60_000) {
    throw new TypeError('timeoutMs must be an integer in the Service Hub v1 range 1..60000');
  }
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function isErrorPayload(value: unknown): value is ServiceHubErrorPayload {
  return (
    isRecord(value) &&
    typeof value.code === 'string' &&
    typeof value.message === 'string'
  );
}

function isResponseSuccessMessage(
  value: Record<string, unknown>,
): value is ResponseSuccessMessage {
  return value.type === 'response' && value.ok === true && 'payload' in value;
}

function isResponseErrorMessage(
  value: Record<string, unknown>,
): value is ResponseErrorMessage {
  return value.type === 'response' && value.ok === false && isErrorPayload(value.error);
}

function isProtocolErrorMessage(
  value: Record<string, unknown>,
): value is ProtocolErrorMessage {
  return value.type === 'protocol_error' && isErrorPayload(value.error);
}

function formatCloseMessage(code: number, reason: string): string {
  const suffix = reason.length > 0 ? `: ${reason}` : '';
  return `Service Hub connection closed with WebSocket code ${code}${suffix}`;
}
