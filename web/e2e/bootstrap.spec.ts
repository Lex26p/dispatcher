import { expect, test } from '@playwright/test';

test('production build renders the base App Shell on desktop and narrow viewports', async ({
  page,
}) => {
  await page.goto('/');

  const header = page.getByRole('banner');
  const workspace = page.getByRole('main');

  await expect(header).toBeVisible();
  await expect(page.getByText('Диспетчер')).toBeVisible();
  await expect(page.getByRole('button', { name: 'Основное меню' })).toBeDisabled();
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

  expect(
    await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth),
  ).toBe(true);

  await page.setViewportSize({ width: 375, height: 667 });

  await expect(header).toBeVisible();
  await expect(
    page.getByRole('heading', { name: 'Рабочая область' }),
  ).toBeVisible();

  expect(
    await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth),
  ).toBe(true);
});
