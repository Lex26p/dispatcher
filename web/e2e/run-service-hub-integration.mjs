import { spawn } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { createServer as createHttpServer } from 'node:http';
import { createServer as createNetServer, createConnection } from 'node:net';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

import { startServiceHubTestProvider } from './support/service-hub-test-provider.mjs';

const currentDir = dirname(fileURLToPath(import.meta.url));
const webRoot = resolve(currentDir, '..');
const repoRoot = resolve(webRoot, '..');
const buildDir = '$HOME/.cache/dispatcher/build/debug';
const hubExecutable = `${buildDir}/services/service-hub/dispatcher-service-hub`;

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

function startWslHub(port) {
  const bashCommand =
    `echo "__DISPATCHER_PID=$$"; ` +
    `exec "${hubExecutable}" 127.0.0.1:${port}`;

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

  const pidPromise = new Promise((resolvePromise, reject) => {
    resolvePid = resolvePromise;
    rejectPid = reject;
  });

  child.stdout.on('data', (chunk) => {
    stdoutBuffer += chunk;

    const lines = stdoutBuffer.split(/\r?\n/);
    stdoutBuffer = lines.pop() ?? '';

    for (const line of lines) {
      const match = /^__DISPATCHER_PID=(\d+)$/.exec(line.trim());

      if (match && !pidResolved) {
        pidResolved = true;
        resolvePid(Number(match[1]));
        continue;
      }

      if (line.length > 0) {
        console.log(`[service-hub] ${line}`);
      }
    }
  });

  child.stderr.on('data', (chunk) => {
    const text = String(chunk).trimEnd();

    if (text.length > 0) {
      console.error(`[service-hub] ${text}`);
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
          `Service Hub exited before reporting PID: code=${String(code)} signal=${String(signal)}`,
        ),
      );
    }
  });

  return {
    child,
    pidPromise,
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
      if (pathname === `/${token}/stop`) {
        await lifecycle.stop();
      } else if (pathname === `/${token}/start`) {
        await lifecycle.start();
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
    'Service Hub browser integration runner currently requires the Windows + WSL development workflow',
  );
}

const wslRepoRoot = windowsPathToWsl(repoRoot);

await run('wsl.exe', [
  'bash',
  '-lc',
  `cd ${shellQuote(wslRepoRoot)} && ` +
    `cmake -S . -B "${buildDir}" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON && ` +
    `cmake --build "${buildDir}" --target dispatcher_service_hub`,
]);

const hubPort = await getFreePort();
const serviceHubUrl = `ws://127.0.0.1:${hubPort}/v1/ws`;

let hub = null;
let hubPid = null;
let provider = null;
let transition = Promise.resolve();

const lifecycle = {
  start() {
    transition = transition.catch(() => undefined).then(async () => {
      if (hub !== null) {
        return;
      }

      hub = startWslHub(hubPort);
      hubPid = await hub.pidPromise;
      await waitForPort(hubPort);

      provider = await startServiceHubTestProvider(serviceHubUrl);
      console.log(`Web Shell test provider registered as ${provider.service}`);
    });

    return transition;
  },

  stop() {
    transition = transition.catch(() => undefined).then(async () => {
      if (hub === null) {
        return;
      }

      const currentHub = hub;
      const currentPid = hubPid;
      const currentProvider = provider;

      hub = null;
      hubPid = null;
      provider = null;

      await stopLinuxProcess(currentPid, currentHub.child);

      if (currentProvider) {
        await Promise.race([
          currentProvider.closed,
          new Promise((resolvePromise) => setTimeout(resolvePromise, 2_000)),
        ]);
      }
    });

    return transition;
  },
};

let control = null;

try {
  await lifecycle.start();
  control = await createControlServer(lifecycle);

  const integrationEnv = {
    ...process.env,
    VITE_SERVICE_HUB_URL: serviceHubUrl,
    VITE_SERVICE_HUB_E2E: '1',
    DISPATCHER_E2E_CONTROL_URL: control.url,
  };

  await runWindowsCommand('npm.cmd', ['run', 'build'], {
    cwd: webRoot,
    env: integrationEnv,
  });

  const playwright = join(webRoot, 'node_modules', '.bin', 'playwright.cmd');

  await runWindowsCommand(
    playwright,
    ['test', 'e2e/service-hub.integration.spec.ts'],
    {
      cwd: webRoot,
      env: integrationEnv,
    },
  );
} finally {
  if (control) {
    await control.close().catch((error) => console.error(error));
  }

  await lifecycle.stop().catch((error) => console.error(error));
}
