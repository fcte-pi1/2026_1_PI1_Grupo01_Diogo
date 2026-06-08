import { test, expect } from '@playwright/test';

test('abre modal de nova corrida', async ({ page }) => {
  await page.goto('http://localhost:3001');

  await expect(
    page.getByRole('link', { name: 'MrBombastic' })
  ).toBeVisible();

  const botao = page.getByRole('button', {
    name: 'Nova Corrida'
  });

  await expect(botao).toBeVisible();

  await botao.click();

  await expect(
    page.getByText('Iniciar uma nova corrida?')
  ).toBeVisible();
});