import { expect, test } from '@playwright/test';

test('production build keeps the public App Shell layout and login navigation usable', async ({
  page,
}) => {
  await page.goto('/');

  const header = page.getByRole('banner');
  const workspace = page.getByRole('main');
  const menuTrigger = page.getByRole('button', {
    name: 'Основное меню',
    exact: true,
  });

  await expect(header).toBeVisible();
  await expect(page.getByText('Диспетчер')).toBeVisible();
  await expect(menuTrigger).toBeEnabled();
  await expect(menuTrigger).toHaveAttribute('aria-expanded', 'false');
  await expect(page.getByRole('heading', { name: 'Рабочая область' })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Открыть вход' })).toBeVisible();

  const serviceHubStatus = page.getByRole('status', {
    name: 'Состояние Service Hub',
  });
  const projectContext = page.getByRole('group', {
    name: 'Контекст проекта',
  });
  await expect(serviceHubStatus).toBeVisible();
  await expect(projectContext).toContainText('Глобальный');
  await expect(serviceHubStatus).toContainText('Service Hub недоступен', {
    timeout: 10_000,
  });

  expect(
    await header.evaluate((element) => Math.round(element.getBoundingClientRect().height)),
  ).toBe(48);
  expect(
    await workspace.evaluate(
      (element) => element.getBoundingClientRect().height >= window.innerHeight - 64,
    ),
  ).toBe(true);

  await menuTrigger.click();
  const navigation = page.getByRole('navigation', { name: 'Глобальная навигация' });
  const workspaceLink = navigation.getByRole('link', { name: 'Рабочая область' });
  const loginLink = navigation.getByRole('link', { name: 'Вход' });

  await expect(navigation.getByRole('link')).toHaveCount(2);
  await expect(workspaceLink).toHaveAttribute('aria-current', 'page');
  await expect(loginLink).not.toHaveAttribute('aria-current', 'page');
  await expect(workspaceLink).toBeFocused();
  await expect(navigation.getByRole('link', { name: 'Проекты' })).toHaveCount(0);

  await page.keyboard.press('Escape');
  await expect(navigation).toBeHidden();
  await expect(menuTrigger).toBeFocused();

  await menuTrigger.click();
  await loginLink.click();
  await expect(page).toHaveURL(/\/login$/);
  await expect(page.getByRole('heading', { name: 'Вход' })).toBeVisible();
  await expect(page.getByRole('textbox', { name: 'Логин' })).toBeEnabled();
  await expect(page.getByRole('button', { name: 'Войти' })).toBeDisabled();
  await expect(page.getByRole('alert')).toContainText('Service Hub недоступен');

  expect(
    await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth),
  ).toBe(true);

  await page.setViewportSize({ width: 375, height: 667 });
  await expect(header).toBeVisible();
  await expect(page.getByRole('heading', { name: 'Вход' })).toBeVisible();
  await expect(projectContext).toBeVisible();

  expect(
    await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth),
  ).toBe(true);
});

test('production build provides a fallback for an unknown shell route', async ({
  page,
}) => {
  await page.goto('/missing');

  await expect(page.getByRole('heading', { name: 'Страница не найдена' })).toBeVisible();

  const menuTrigger = page.getByRole('button', {
    name: 'Основное меню',
    exact: true,
  });
  await menuTrigger.click();

  const navigation = page.getByRole('navigation', {
    name: 'Глобальная навигация',
  });
  const workspaceLink = navigation.getByRole('link', {
    name: 'Рабочая область',
  });

  await expect(workspaceLink).not.toHaveAttribute('aria-current', 'page');
  await workspaceLink.click();

  await expect(page).toHaveURL(/\/$/);
  await expect(page.getByRole('heading', { name: 'Рабочая область' })).toBeVisible();
  await expect(navigation).toBeHidden();
});
