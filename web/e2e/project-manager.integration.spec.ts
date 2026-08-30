import { expect, test } from '@playwright/test';

test('real Web Shell observes Project Manager authenticated boundary', async ({
  page,
}) => {
  await page.goto('/projects');

  const serviceHubStatus = page.getByRole('status', {
    name: 'Состояние Service Hub',
  });

  await expect(serviceHubStatus).toContainText('Service Hub подключен', {
    timeout: 10_000,
  });

  await expect(
    page.getByRole('heading', { name: 'Проекты' }),
  ).toBeVisible();

  await expect(page.getByRole('alert')).toContainText(
    'Project Manager вернул ошибку: auth.invalid_session.',
    { timeout: 10_000 },
  );

  await page.getByRole('button', { name: 'Создать проект' }).click();
  await page.getByRole('textbox', { name: 'Название' }).fill(
    'Неаутентифицированный проект',
  );
  await page.getByRole('button', { name: 'Сохранить' }).click();

  await expect(page.getByRole('alert')).toContainText(
    'Project Manager вернул ошибку: auth.invalid_session.',
  );
});
