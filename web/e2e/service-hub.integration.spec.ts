import { expect, test } from '@playwright/test';

type BrowserServiceHubClient = {
  readonly connectionState: string;
  connect(): Promise<void>;
  disconnect(): void;
  request(
    service: string,
    operation: string,
    payload: unknown,
    options?: { timeoutMs?: number },
  ): {
    readonly response: Promise<unknown>;
    cancel(): boolean;
  };
};

async function controlServiceHub(action: 'start' | 'stop') {
  const controlUrl = process.env.DISPATCHER_E2E_CONTROL_URL;

  if (!controlUrl) {
    throw new Error('DISPATCHER_E2E_CONTROL_URL is not configured');
  }

  const response = await fetch(`${controlUrl}/${action}`, {
    method: 'POST',
  });

  if (!response.ok) {
    throw new Error(
      `Service Hub integration control ${action} failed: ${response.status} ${await response.text()}`,
    );
  }
}

test('real browser client uses Service Hub request/cancel/reconnect path', async ({
  page,
}) => {
  test.skip(
    !process.env.DISPATCHER_E2E_CONTROL_URL,
    'requires the real Service Hub integration runner',
  );

  await page.goto('/');

  const status = page.getByRole('status', {
    name: 'Состояние Service Hub',
  });

  await expect(status).toContainText('Service Hub подключен', {
    timeout: 10_000,
  });

  const echoPayload = await page.evaluate(async () => {
    const client = (
      window as typeof window & {
        __dispatcherServiceHubE2EClient?: BrowserServiceHubClient;
      }
    ).__dispatcherServiceHubE2EClient;

    if (!client) {
      throw new Error('Service Hub E2E client seam is unavailable');
    }

    return await client.request(
      'test.web-shell',
      'echo',
      { text: 'browser-real-request' },
      { timeoutMs: 5_000 },
    ).response;
  });

  expect(echoPayload).toEqual({
    text: 'browser-real-request',
  });

  const parallelPayloads = await page.evaluate(async () => {
    const client = (
      window as typeof window & {
        __dispatcherServiceHubE2EClient?: BrowserServiceHubClient;
      }
    ).__dispatcherServiceHubE2EClient;

    if (!client) {
      throw new Error('Service Hub E2E client seam is unavailable');
    }

    const first = client.request('test.web-shell', 'parallel-echo', {
      order: 'first',
    });
    const second = client.request('test.web-shell', 'parallel-echo', {
      order: 'second',
    });

    return await Promise.all([first.response, second.response]);
  });

  expect(parallelPayloads).toEqual([
    { order: 'first' },
    { order: 'second' },
  ]);

  const unknownService = await page.evaluate(async () => {
    const client = (
      window as typeof window & {
        __dispatcherServiceHubE2EClient?: BrowserServiceHubClient;
      }
    ).__dispatcherServiceHubE2EClient;

    if (!client) {
      throw new Error('Service Hub E2E client seam is unavailable');
    }

    try {
      await client.request('test.missing', 'echo', null).response;
      return { ok: true, code: '' };
    } catch (error) {
      const code =
        typeof error === 'object' &&
        error !== null &&
        'code' in error
          ? String((error as { code?: unknown }).code)
          : '';

      return { ok: false, code };
    }
  });

  expect(unknownService).toEqual({
    ok: false,
    code: 'hub.unknown_service',
  });

  const cancellation = await page.evaluate(async () => {
    const client = (
      window as typeof window & {
        __dispatcherServiceHubE2EClient?: BrowserServiceHubClient;
      }
    ).__dispatcherServiceHubE2EClient;

    if (!client) {
      throw new Error('Service Hub E2E client seam is unavailable');
    }

    const request = client.request(
      'test.web-shell',
      'wait-for-cancel',
      { work: 'cancel-me' },
      { timeoutMs: 5_000 },
    );
    const cancelSent = request.cancel();

    try {
      await request.response;
      return {
        cancelSent,
        code: '',
      };
    } catch (error) {
      const code =
        typeof error === 'object' &&
        error !== null &&
        'code' in error
          ? String((error as { code?: unknown }).code)
          : '';

      return {
        cancelSent,
        code,
      };
    }
  });

  expect(cancellation).toEqual({
    cancelSent: true,
    code: 'hub.cancelled',
  });

  await expect
    .poll(
      async () =>
        await page.evaluate(async () => {
          const client = (
            window as typeof window & {
              __dispatcherServiceHubE2EClient?: BrowserServiceHubClient;
            }
          ).__dispatcherServiceHubE2EClient;

          if (!client) {
            throw new Error('Service Hub E2E client seam is unavailable');
          }

          const payload = await client.request(
            'test.web-shell',
            'cancel-count',
            null,
          ).response;

          if (
            typeof payload !== 'object' ||
            payload === null ||
            !('count' in payload)
          ) {
            return 0;
          }

          return Number((payload as { count?: unknown }).count ?? 0);
        }),
      {
        timeout: 5_000,
      },
    )
    .toBeGreaterThanOrEqual(1);

  await controlServiceHub('stop');

  await expect(status).toContainText('Service Hub недоступен', {
    timeout: 10_000,
  });

  await controlServiceHub('start');

  await expect(status).toContainText('Service Hub недоступен');

  const reconnectPayload = await page.evaluate(async () => {
    const client = (
      window as typeof window & {
        __dispatcherServiceHubE2EClient?: BrowserServiceHubClient;
      }
    ).__dispatcherServiceHubE2EClient;

    if (!client) {
      throw new Error('Service Hub E2E client seam is unavailable');
    }

    await client.connect();

    return await client.request('test.web-shell', 'echo', {
      text: 'after-explicit-reconnect',
    }).response;
  });

  expect(reconnectPayload).toEqual({
    text: 'after-explicit-reconnect',
  });

  await expect(status).toContainText('Service Hub подключен');

  await expect(
    page.getByRole('heading', { name: 'Рабочая область' }),
  ).toBeVisible();

  await page.getByRole('button', {
    name: 'Основное меню',
    exact: true,
  }).click();

  await expect(
    page.getByRole('navigation', { name: 'Глобальная навигация' }),
  ).toBeVisible();
});
