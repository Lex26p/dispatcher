import { expect, test } from '@playwright/test';

test('real Web Shell protects Project Manager route behind the user session UI', async ({
  page,
}) => {
  test.skip(
    !process.env.VITE_SERVICE_HUB_URL,
    'requires the real Project Manager integration runner',
  );

  await page.goto('/projects');

  const serviceHubStatus = page.getByRole('status', {
    name: 'Состояние Service Hub',
  });

  await expect(serviceHubStatus).toContainText('Service Hub подключен', {
    timeout: 10_000,
  });

  await expect(
    page.getByRole('heading', { name: 'Вход для работы с проектами' }),
  ).toBeVisible();
  await expect(page.getByRole('textbox', { name: 'Логин' })).toBeEnabled();
  await expect(page.getByLabel('Пароль')).toBeEnabled();
  await expect(page.getByRole('heading', { name: 'Проекты' })).toHaveCount(0);

  await page.getByRole('button', { name: 'Основное меню', exact: true }).click();
  const navigation = page.getByRole('navigation', { name: 'Глобальная навигация' });
  await expect(navigation.getByRole('link', { name: 'Проекты' })).toHaveCount(0);
  await expect(navigation.getByRole('link', { name: 'Вход' })).toBeVisible();
});
