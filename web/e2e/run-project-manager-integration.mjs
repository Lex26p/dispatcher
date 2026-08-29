import { spawn } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { createServer as createHttpServer } from 'node:http';
import { createConnection, createServer as createNetServer } from 'node:net';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const currentDir = dirname(fileURLToPath(import.meta.url));
const webRoot = resolve(currentDir, '..');
const repoRoot = resolve(webRoot, '..');
const buildDir = '$HOME/.cache/dispatcher/build/debug';
const hubExecutable = `${buildDir}/services/service-hub/dispatcher-service-hub`;
const projectManagerExecutable =
  `${buildDir}/services/project-manager/dispatcher-project-manager`;
const subprotocol = 'dispatcher.service-hub.v1';

function shellQuote(value) {
  return `'${String(value).replaceAll("'", "'\\''")}'`;
}

function windowsPathToWsl(path) {
  const normalized = resolve(path);
  const match = /^([A-Za-z]):[\\/](.*)$/.exec(normalized);

  if (!match) {
    throw new Error(`Unsupported Windows repository path: ${normalized}`);
  }

  const drive = match[1].toLowerCase();
  const rest = match[2].replaceAll('\\', '/');
  return `/mnt/${drive}/${rest}`;
}

function run(command, args, options = {}) {
  return new Promise((resolvePromise, reject) => {
    const child = spawn(command, args, {
      cwd: options.cwd ?? webRoot,
      env: options.env ?? process.env,
      stdio: options.stdio ?? 'inherit',
      windowsHide: true,
    });

    child.once('error', reject);
    child.once('exit', (code, signal) => {
      if (code === 0) {
        resolvePromise();
        return;
      }

      reject(
        new Error(
          `${command} exited with code ${String(code)} signal ${String(signal)}`,
        ),
      );
    });
  });
}

function quoteWindowsArgument(value) {
  const text = String(value);

  if (!/[\s&|<>^]/.test(text)) {
    return text;
  }

  return `"${text}"`;
}

function runWindowsCommand(command, args, options = {}) {
  const commandLine = [command, ...args]
    .map(quoteWindowsArgument)
    .join(' ');

  return run(
    process.env.ComSpec ?? 'cmd.exe',
    ['/d', '/s', '/c', commandLine],
    options,
  );
}

function runCurrentNpm(args, options = {}) {
  const npmExecPath = process.env.npm_execpath;

  if (npmExecPath) {
    return run(process.execPath, [npmExecPath, ...args], options);
  }

  return runWindowsCommand('npm.cmd', args, options);
}

async function getFreePort() {
  return await new Promise((resolvePromise, reject) => {
    const server = createNetServer();

    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => {
      const address = server.address();

      if (typeof address !== 'object' || address === null) {
        server.close();
        reject(new Error('Failed to allocate a local TCP port'));
        return;
      }

      const { port } = address;
      server.close((error) => {
        if (error) {
          reject(error);
          return;
        }

        resolvePromise(port);
      });
    });
  });
}

async function waitForPort(port, timeoutMs = 15_000) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    const connected = await new Promise((resolvePromise) => {
      const socket = createConnection({
        host: '127.0.0.1',
        port,
      });

      socket.once('connect', () => {
        socket.destroy();
        resolvePromise(true);
      });
      socket.once('error', () => {
        socket.destroy();
        resolvePromise(false);
      });
    });

    if (connected) {
      return;
    }

    await new Promise((resolvePromise) => setTimeout(resolvePromise, 100));
  }

  throw new Error(`Timed out waiting for Service Hub on port ${port}`);
}

function startWslProcess(label, command) {
  const bashCommand = `echo "__DISPATCHER_PID=$$"; exec ${command}`;
  const child = spawn('wsl.exe', ['bash', '-lc', bashCommand], {
    cwd: webRoot,
    stdio: ['ignore', 'pipe', 'pipe'],
    windowsHide: true,
  });

  child.stdout.setEncoding('utf8');
  child.stderr.setEncoding('utf8');

  let stdoutBuffer = '';
  let pidResolved = false;
  let resolvePid;
  let rejectPid;
  const lines = [];
  const lineWaiters = new Set();

  const pidPromise = new Promise((resolvePromise, reject) => {
    resolvePid = resolvePromise;
    rejectPid = reject;
  });

  const publishLine = (line) => {
    lines.push(line);
    console.log(`[${label}] ${line}`);

    for (const waiter of [...lineWaiters]) {
      if (waiter.pattern.test(line)) {
        lineWaiters.delete(waiter);
        clearTimeout(waiter.timeout);
        waiter.resolve(line);
      }
    }
  };

  child.stdout.on('data', (chunk) => {
    stdoutBuffer += chunk;
    const completeLines = stdoutBuffer.split(/\r?\n/);
    stdoutBuffer = completeLines.pop() ?? '';

    for (const rawLine of completeLines) {
      const line = rawLine.trimEnd();
      const match = /^__DISPATCHER_PID=(\d+)$/.exec(line.trim());

      if (match && !pidResolved) {
        pidResolved = true;
        resolvePid(Number(match[1]));
        continue;
      }

      if (line.length > 0) {
        publishLine(line);
      }
    }
  });

  child.stderr.on('data', (chunk) => {
    const text = String(chunk).trimEnd();

    if (text.length > 0) {
      for (const line of text.split(/\r?\n/)) {
        console.error(`[${label}] ${line}`);
      }
    }
  });

  child.once('error', (error) => {
    if (!pidResolved) {
      pidResolved = true;
      rejectPid(error);
    }
  });

  child.once('exit', (code, signal) => {
    if (!pidResolved) {
      pidResolved = true;
      rejectPid(
        new Error(
          `${label} exited before reporting PID: code=${String(code)} signal=${String(signal)}`,
        ),
      );
    }

    for (const waiter of [...lineWaiters]) {
      lineWaiters.delete(waiter);
      clearTimeout(waiter.timeout);
      waiter.reject(
        new Error(
          `${label} exited before output matched ${String(waiter.pattern)}: ` +
            `code=${String(code)} signal=${String(signal)}`,
        ),
      );
    }
  });

  return {
    child,
    pidPromise,
    waitForLine(pattern, timeoutMs = 15_000) {
      const existing = lines.find((line) => pattern.test(line));
      if (existing !== undefined) {
        return Promise.resolve(existing);
      }

      return new Promise((resolvePromise, reject) => {
        const waiter = {
          pattern,
          resolve: resolvePromise,
          reject,
          timeout: setTimeout(() => {
            lineWaiters.delete(waiter);
            reject(
              new Error(
                `Timed out waiting for ${label} output matching ${String(pattern)}`,
              ),
            );
          }, timeoutMs),
        };

        lineWaiters.add(waiter);
      });
    },
  };
}

async function stopLinuxProcess(pid, child) {
  if (!pid) {
    return;
  }

  await run(
    'wsl.exe',
    ['bash', '-lc', `kill -TERM ${pid} 2>/dev/null || true`],
    { stdio: 'ignore' },
  );

  const exited = await Promise.race([
    new Promise((resolvePromise) => {
      if (child.exitCode !== null) {
        resolvePromise(true);
        return;
      }

      child.once('exit', () => resolvePromise(true));
    }),
    new Promise((resolvePromise) =>
      setTimeout(() => resolvePromise(false), 5_000),
    ),
  ]);

  if (!exited) {
    await run(
      'wsl.exe',
      ['bash', '-lc', `kill -KILL ${pid} 2>/dev/null || true`],
      { stdio: 'ignore' },
    );
  }
}

function waitForEvent(target, type, timeoutMs = 2_000) {
  return new Promise((resolvePromise, reject) => {
    const timeout = setTimeout(() => {
      cleanup();
      reject(new Error(`Timed out waiting for WebSocket ${type}`));
    }, timeoutMs);

    const handleEvent = (event) => {
      cleanup();
      resolvePromise(event);
    };

    const handleClose = () => {
      if (type === 'close') {
        return;
      }

      cleanup();
      reject(new Error(`WebSocket closed while waiting for ${type}`));
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

async function probeProjectManager(serviceHubUrl) {
  if (typeof WebSocket !== 'function') {
    throw new Error('Node.js WebSocket global is unavailable');
  }

  const requestId = `probe-${randomUUID()}`;
  const socket = new WebSocket(serviceHubUrl, subprotocol);

  try {
    await waitForEvent(socket, 'open');

    if (socket.protocol !== subprotocol) {
      return false;
    }

    socket.send(
      JSON.stringify({
        type: 'request',
        id: requestId,
        service: 'project-manager.v1',
        operation: 'list-projects',
        payload: {},
        timeout_ms: 1_000,
      }),
    );

    const event = await waitForEvent(socket, 'message');

    if (typeof event.data !== 'string') {
      return false;
    }

    const message = JSON.parse(event.data);
    return (
      message?.type === 'response' &&
      message.id === requestId &&
      message.ok === true &&
      Array.isArray(message.payload?.projects)
    );
  } catch {
    return false;
  } finally {
    if (socket.readyState < 2) {
      socket.close(1000, 'Project Manager readiness probe complete');
    }
  }
}

async function waitForProjectManager(serviceHubUrl, timeoutMs = 15_000) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    if (await probeProjectManager(serviceHubUrl)) {
      return;
    }

    await new Promise((resolvePromise) => setTimeout(resolvePromise, 150));
  }

  throw new Error('Timed out waiting for project-manager.v1 registration');
}

async function createControlServer(lifecycle) {
  const token = randomUUID();
  const port = await getFreePort();

  const server = createHttpServer(async (request, response) => {
    const pathname = new URL(
      request.url ?? '/',
      `http://127.0.0.1:${port}`,
    ).pathname;

    if (request.method !== 'POST') {
      response.writeHead(405).end('Method not allowed');
      return;
    }

    try {
      if (pathname === `/${token}/project-manager/stop`) {
        await lifecycle.stopProjectManager();
      } else if (pathname === `/${token}/project-manager/start`) {
        await lifecycle.startProjectManager();
      } else {
        response.writeHead(404).end('Not found');
        return;
      }

      response.writeHead(204).end();
    } catch (error) {
      console.error(error);
      response.writeHead(500).end(String(error));
    }
  });

  await new Promise((resolvePromise, reject) => {
    server.once('error', reject);
    server.listen(port, '127.0.0.1', resolvePromise);
  });

  return {
    url: `http://127.0.0.1:${port}/${token}`,
    async close() {
      await new Promise((resolvePromise, reject) => {
        server.close((error) => {
          if (error) {
            reject(error);
            return;
          }

          resolvePromise();
        });
      });
    },
  };
}

if (process.platform !== 'win32') {
  throw new Error(
    'Project Manager browser integration runner requires the Windows + WSL development workflow',
  );
}

const wslRepoRoot = windowsPathToWsl(repoRoot);
const databasePath =
  `/tmp/dispatcher-project-manager-web-${randomUUID()}.db`;

await run('wsl.exe', [
  'bash',
  '-lc',
  `cd ${shellQuote(wslRepoRoot)} && ` +
    `cmake -S . -B "${buildDir}" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON && ` +
    `cmake --build "${buildDir}" --target dispatcher_service_hub dispatcher_project_manager`,
]);

const hubPort = await getFreePort();
const serviceHubUrl = `ws://127.0.0.1:${hubPort}/v1/ws`;

let hub = null;
let hubPid = null;
let projectManager = null;
let projectManagerPid = null;
let transition = Promise.resolve();

const lifecycle = {
  startProjectManager() {
    transition = transition.catch(() => undefined).then(async () => {
      if (projectManager !== null) {
        return;
      }

      const serviceHubAddress = `127.0.0.1:${hubPort}`;
      projectManager = startWslProcess(
        'project-manager',
        `"${projectManagerExecutable}" ` +
          `${shellQuote(databasePath)} ${shellQuote(serviceHubAddress)}`,
      );
      projectManagerPid = await projectManager.pidPromise;
      await projectManager.waitForLine(/Dispatcher Project Manager started/);
      await waitForProjectManager(serviceHubUrl);
    });

    return transition;
  },

  stopProjectManager() {
    transition = transition.catch(() => undefined).then(async () => {
      if (projectManager === null) {
        return;
      }

      const current = projectManager;
      const currentPid = projectManagerPid;
      projectManager = null;
      projectManagerPid = null;
      await stopLinuxProcess(currentPid, current.child);
    });

    return transition;
  },
};

let control = null;

try {
  hub = startWslProcess(
    'service-hub',
    `"${hubExecutable}" 127.0.0.1:${hubPort}`,
  );
  hubPid = await hub.pidPromise;
  await waitForPort(hubPort);
  await lifecycle.startProjectManager();

  control = await createControlServer(lifecycle);

  const integrationEnv = {
    ...process.env,
    VITE_SERVICE_HUB_URL: serviceHubUrl,
    VITE_SERVICE_HUB_E2E: '1',
    DISPATCHER_PROJECT_MANAGER_E2E_CONTROL_URL: control.url,
  };

  await runCurrentNpm(['run', 'build'], {
    cwd: webRoot,
    env: integrationEnv,
  });

  const playwright = join(webRoot, 'node_modules', '.bin', 'playwright.cmd');

  await runWindowsCommand(
    playwright,
    ['test', 'e2e/project-manager.integration.spec.ts'],
    {
      cwd: webRoot,
      env: integrationEnv,
    },
  );
} finally {
  if (control) {
    await control.close().catch((error) => console.error(error));
  }

  await lifecycle
    .stopProjectManager()
    .catch((error) => console.error(error));

  if (hub !== null) {
    await stopLinuxProcess(hubPid, hub.child)
      .catch((error) => console.error(error));
  }

  await run(
    'wsl.exe',
    ['bash', '-lc', `rm -f ${shellQuote(databasePath)}`],
    { stdio: 'ignore' },
  ).catch((error) => console.error(error));
}
