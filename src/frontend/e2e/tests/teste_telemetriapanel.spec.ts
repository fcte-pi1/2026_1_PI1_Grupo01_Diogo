import { test, expect } from '@playwright/test';

test('deve exibir informações da telemetria', async ({ page }) => {
  await page.goto('http://localhost:3001/runs/corrida-padrao');

  await expect(
    page.getByRole('heading', { name: 'Informações' })
  ).toBeVisible();

  await expect(
    page.getByText('Estado Atual')
  ).toBeVisible();

  await expect(
    page.getByText('Bateria')
  ).toBeVisible();

  await expect(
    page.getByText('Tempo')
  ).toBeVisible();

  await expect(
    page.getByText('Posição')
  ).toBeVisible();

  await expect(
    page.getByText('Direção')
  ).toBeVisible();

  await expect(
    page.getByText('Sensores')
  ).toBeVisible();
});