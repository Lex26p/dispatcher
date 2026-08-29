import { expect, test } from '@playwright/test';

test('production build opens the Web Shell bootstrap page', async ({ page }) => {
  await page.goto('/');

  await expect(
    page.getByRole('heading', { name: 'Диспетчер Web Shell' }),
  ).toBeVisible();

  await expect(page.getByText('CORE-003 / Step 1')).toBeVisible();
});
