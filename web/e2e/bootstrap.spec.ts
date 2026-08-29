import { expect, test } from '@playwright/test';

test('production build keeps the App Shell layout and global navigation usable', async ({
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
  await expect(
    page.getByRole('heading', { name: 'Рабочая область' }),
  ).toBeVisible();

  expect(
    await header.evaluate((element) => Math.round(element.getBoundingClientRect().height)),
  ).toBe(48);

  expect(
    await workspace.evaluate(
      (element) => element.getBoundingClientRect().height >= window.innerHeight - 64,
    ),
  ).toBe(true);

  await menuTrigger.click();

  const navigation = page.getByRole('navigation', {
    name: 'Глобальная навигация',
  });
  const workspaceLink = navigation.getByRole('link', {
    name: 'Рабочая область',
  });

  await expect(navigation).toBeVisible();
  await expect(navigation.getByRole('link')).toHaveCount(1);
  await expect(workspaceLink).toHaveAttribute('aria-current', 'page');
  await expect(workspaceLink).toBeFocused();

  await page.keyboard.press('Escape');

  await expect(navigation).toBeHidden();
  await expect(menuTrigger).toHaveAttribute('aria-expanded', 'false');
  await expect(menuTrigger).toBeFocused();

  await menuTrigger.click();
  await menuTrigger.click();
  await expect(navigation).toBeHidden();

  expect(
    await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth),
  ).toBe(true);

  await page.setViewportSize({ width: 375, height: 667 });

  await expect(header).toBeVisible();
  await expect(
    page.getByRole('heading', { name: 'Рабочая область' }),
  ).toBeVisible();

  await menuTrigger.click();
  await expect(navigation).toBeVisible();

  expect(
    await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth),
  ).toBe(true);
});

test('production build provides a fallback for an unknown shell route', async ({
  page,
}) => {
  await page.goto('/missing');

  await expect(
    page.getByRole('heading', { name: 'Страница не найдена' }),
  ).toBeVisible();

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
  await expect(
    page.getByRole('heading', { name: 'Рабочая область' }),
  ).toBeVisible();
  await expect(navigation).toBeHidden();
});
