const SERVICE_HUB_SUBPROTOCOL = 'dispatcher.service-hub.v1';
const SERVICE_NAME = 'test.web-shell';

function waitForEvent(target, type, timeoutMs = 10_000) {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      cleanup();
      reject(new Error(`Timed out waiting for WebSocket ${type}`));
    }, timeoutMs);

    const handleEvent = (event) => {
      cleanup();
      resolve(event);
    };

    const handleClose = (event) => {
      if (type === 'close') {
        return;
      }

      cleanup();
      reject(
        new Error(
          `WebSocket closed while waiting for ${type}: ${event.code} ${event.reason}`,
        ),
      );
    };

    const cleanup = () => {
      clearTimeout(timeout);
      target.removeEventListener(type, handleEvent);
      target.removeEventListener('close', handleClose);
    };

    target.addEventListener(type, handleEvent, { once: true });

    if (type !== 'close') {
      target.addEventListener('close', handleClose, { once: true });
    }
  });
}

function waitForMessage(socket, predicate, timeoutMs = 10_000) {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      cleanup();
      reject(new Error('Timed out waiting for Service Hub provider message'));
    }, timeoutMs);

    const handleMessage = (event) => {
      if (typeof event.data !== 'string') {
        return;
      }

      let message;

      try {
        message = JSON.parse(event.data);
      } catch {
        return;
      }

      if (!predicate(message)) {
        return;
      }

      cleanup();
      resolve(message);
    };

    const handleClose = (event) => {
      cleanup();
      reject(
        new Error(
          `Service Hub provider connection closed: ${event.code} ${event.reason}`,
        ),
      );
    };

    const cleanup = () => {
      clearTimeout(timeout);
      socket.removeEventListener('message', handleMessage);
      socket.removeEventListener('close', handleClose);
    };

    socket.addEventListener('message', handleMessage);
    socket.addEventListener('close', handleClose, { once: true });
  });
}

function sendJson(socket, message) {
  socket.send(JSON.stringify(message));
}

function responseFor(request) {
  return {
    type: 'response',
    id: request.id,
    ok: true,
    payload: request.payload,
  };
}

export async function startServiceHubTestProvider(url) {
  if (typeof WebSocket !== 'function') {
    throw new Error('Node.js WebSocket global is unavailable');
  }

  const socket = new WebSocket(url, SERVICE_HUB_SUBPROTOCOL);
  await waitForEvent(socket, 'open');

  if (socket.protocol !== SERVICE_HUB_SUBPROTOCOL) {
    socket.close(1002, 'Unexpected Service Hub subprotocol');
    throw new Error(
      `Service Hub negotiated unexpected subprotocol: ${socket.protocol || '<none>'}`,
    );
  }

  sendJson(socket, {
    type: 'register',
    service: SERVICE_NAME,
  });

  const registration = await waitForMessage(
    socket,
    (message) =>
      message?.type === 'registered' || message?.type === 'protocol_error',
  );

  if (registration.type !== 'registered' || registration.service !== SERVICE_NAME) {
    socket.close(1002, 'Provider registration failed');
    throw new Error(
      `Service Hub test provider registration failed: ${JSON.stringify(registration)}`,
    );
  }

  let cancelCount = 0;
  const waitingForCancel = new Set();
  const parallelRequests = [];

  const handleMessage = (event) => {
    if (typeof event.data !== 'string') {
      return;
    }

    let message;

    try {
      message = JSON.parse(event.data);
    } catch {
      return;
    }

    if (message?.type === 'cancel' && typeof message.id === 'string') {
      if (waitingForCancel.delete(message.id)) {
        cancelCount += 1;
      }
      return;
    }

    if (
      message?.type !== 'request' ||
      typeof message.id !== 'string' ||
      message.service !== SERVICE_NAME ||
      typeof message.operation !== 'string'
    ) {
      return;
    }

    switch (message.operation) {
      case 'echo':
        sendJson(socket, responseFor(message));
        return;

      case 'parallel-echo':
        parallelRequests.push(message);

        if (parallelRequests.length === 2) {
          const [first, second] = parallelRequests.splice(0, 2);
          sendJson(socket, responseFor(second));
          sendJson(socket, responseFor(first));
        }
        return;

      case 'wait-for-cancel':
        waitingForCancel.add(message.id);
        return;

      case 'cancel-count':
        sendJson(socket, {
          type: 'response',
          id: message.id,
          ok: true,
          payload: {
            count: cancelCount,
          },
        });
        return;

      default:
        sendJson(socket, {
          type: 'response',
          id: message.id,
          ok: false,
          error: {
            code: 'test.unknown_operation',
            message: `Unknown Web Shell test operation: ${message.operation}`,
          },
        });
    }
  };

  socket.addEventListener('message', handleMessage);

  const closed = waitForEvent(socket, 'close').catch(() => undefined);

  return {
    service: SERVICE_NAME,
    closed,
    async close() {
      socket.removeEventListener('message', handleMessage);

      if (socket.readyState < 2) {
        socket.close(1000, 'Web Shell integration provider shutdown');
      }

      await closed;
    },
  };
}
