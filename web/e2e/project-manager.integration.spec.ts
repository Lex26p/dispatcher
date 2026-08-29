import { expect, test } from '@playwright/test';

type BrowserServiceHubClient = {
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

interface StoredProject {
  id: string;
  name: string;
  description: string;
}

async function controlProjectManager(action: 'start' | 'stop') {
  const controlUrl = process.env.DISPATCHER_PROJECT_MANAGER_E2E_CONTROL_URL;

  if (!controlUrl) {
    throw new Error(
      'DISPATCHER_PROJECT_MANAGER_E2E_CONTROL_URL is not configured',
    );
  }

  const response = await fetch(`${controlUrl}/project-manager/${action}`, {
    method: 'POST',
  });

  if (!response.ok) {
    throw new Error(
      `Project Manager integration control ${action} failed: ` +
        `${response.status} ${await response.text()}`,
    );
  }
}


test('real Web Shell uses Project Manager with durable restart recovery', async ({
  page,
}) => {
  test.skip(
    !process.env.DISPATCHER_PROJECT_MANAGER_E2E_CONTROL_URL,
    'requires the real Project Manager integration runner',
  );

  await page.goto('/projects');

  const serviceHubStatus = page.getByRole('status', {
    name: 'Состояние Service Hub',
  });
  const projectContext = page.getByRole('group', {
    name: 'Контекст проекта',
  });

  await expect(serviceHubStatus).toContainText('Service Hub подключен', {
    timeout: 10_000,
  });
  await expect(
    page.getByRole('heading', { name: 'Проекты' }),
  ).toBeVisible();
  await expect(
    page.getByRole('heading', { name: 'Проектов пока нет' }),
  ).toBeVisible();

  await page.getByRole('button', { name: 'Создать проект' }).click();
  await page.getByRole('textbox', { name: 'Название' }).fill(
    'Интеграционный проект',
  );
  await page.getByRole('textbox', { name: 'Описание' }).fill(
    'Создан через реальный Project Manager',
  );
  await page.getByRole('button', { name: 'Сохранить' }).click();

  let projectEntry = page
    .getByRole('listitem')
    .filter({ hasText: 'Интеграционный проект' });

  await expect(projectEntry).toBeVisible();

  await projectEntry.getByRole('button', {
    name: 'Выбрать Интеграционный проект как текущий контекст',
  }).click();

  await expect(projectContext).toContainText('Интеграционный проект');

  const storedProject = await page.evaluate(() => {
    const raw = window.sessionStorage.getItem('dispatcher.project-context.v1');

    if (raw === null) {
      throw new Error('Selected project was not persisted in sessionStorage');
    }

    return JSON.parse(raw) as StoredProject;
  });

  expect(storedProject.id).not.toHaveLength(0);

  await projectEntry.locator('button.project-list__item').click();

  await page.getByRole('textbox', { name: 'Название' }).fill(
    'Интеграционный проект обновлён',
  );
  await page.getByRole('button', { name: 'Сохранить' }).click();

  projectEntry = page
    .getByRole('listitem')
    .filter({ hasText: 'Интеграционный проект обновлён' });

  await expect(projectEntry).toBeVisible();
  await expect(projectContext).toContainText('Интеграционный проект обновлён');

  const parallelPayloads = await page.evaluate(async (projectId) => {
    const client = (
      window as typeof window & {
        __dispatcherServiceHubE2EClient?: BrowserServiceHubClient;
      }
    ).__dispatcherServiceHubE2EClient;

    if (!client) {
      throw new Error('Service Hub E2E client seam is unavailable');
    }

    const list = client.request(
      'project-manager.v1',
      'list-projects',
      {},
      { timeoutMs: 5_000 },
    );
    const get = client.request(
      'project-manager.v1',
      'get-project',
      { id: projectId },
      { timeoutMs: 5_000 },
    );

    return await Promise.all([list.response, get.response]);
  }, storedProject.id);

  expect(parallelPayloads[0]).toMatchObject({
    projects: [
      {
        id: storedProject.id,
        name: 'Интеграционный проект обновлён',
      },
    ],
  });
  expect(parallelPayloads[1]).toMatchObject({
    project: {
      id: storedProject.id,
      name: 'Интеграционный проект обновлён',
    },
  });

  await controlProjectManager('stop');

  await expect(serviceHubStatus).toContainText('Service Hub подключен');
  await expect(projectContext).toContainText('Интеграционный проект обновлён');

  const menuTrigger = page.getByRole('button', {
    name: 'Основное меню',
    exact: true,
  });

  await menuTrigger.click();
  await page.getByRole('navigation', {
    name: 'Глобальная навигация',
  }).getByRole('link', { name: 'Рабочая область' }).click();

  await expect(
    page.getByRole('heading', { name: 'Рабочая область' }),
  ).toBeVisible();
  await expect(projectContext).toContainText('Интеграционный проект обновлён');

  await menuTrigger.click();
  await page.getByRole('navigation', {
    name: 'Глобальная навигация',
  }).getByRole('link', { name: 'Проекты' }).click();

  await expect(page.getByRole('alert')).toContainText(
    'Project Manager недоступен.',
    { timeout: 10_000 },
  );
  await expect(serviceHubStatus).toContainText('Service Hub подключен');
  await expect(projectContext).toContainText('Интеграционный проект обновлён');

  await controlProjectManager('start');

  await page.getByRole('button', { name: 'Повторить' }).click();

  projectEntry = page
    .getByRole('listitem')
    .filter({ hasText: 'Интеграционный проект обновлён' });
  await expect(projectEntry).toBeVisible({
    timeout: 10_000,
  });

  await page.reload();

  await expect(serviceHubStatus).toContainText('Service Hub подключен', {
    timeout: 10_000,
  });
  await expect(projectContext).toContainText(
    'Интеграционный проект обновлён',
    { timeout: 10_000 },
  );

  projectEntry = page
    .getByRole('listitem')
    .filter({ hasText: 'Интеграционный проект обновлён' });
  await expect(projectEntry).toBeVisible({
    timeout: 10_000,
  });

  const restoredProject = await page.evaluate(() => {
    const raw = window.sessionStorage.getItem('dispatcher.project-context.v1');

    if (raw === null) {
      throw new Error('Project context disappeared after Project Manager restart');
    }

    return JSON.parse(raw) as StoredProject;
  });

  expect(restoredProject.id).toBe(storedProject.id);

  await projectEntry.locator('button.project-list__item').click();
  await page.getByRole('textbox', { name: 'Название' }).fill(
    'Интеграционный проект после рестарта',
  );
  await page.getByRole('button', { name: 'Сохранить' }).click();

  await expect(projectContext).toContainText(
    'Интеграционный проект после рестарта',
  );
  await expect(
    page
      .getByRole('listitem')
      .filter({ hasText: 'Интеграционный проект после рестарта' }),
  ).toBeVisible();
});
