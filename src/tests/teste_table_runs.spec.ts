import { test, expect } from '@playwright/test';

test('fluxo completo de gerenciamento de corridas', async ({ page }) => {

  // Acessa a página
  await page.goto('http://localhost:3001');

  // Aguarda a tabela carregar
  await expect(
    page.getByRole('table')
  ).toBeVisible();


  const primeiraLinha = page.locator('tbody tr').first();

  await expect(primeiraLinha).toBeVisible();

  
  const urlOriginal = page.url();


  await primeiraLinha.click();

  // Deve navegar para os detalhes
  await expect(page).not.toHaveURL(urlOriginal);

  await expect(page).toHaveURL(/\/runs\/.+/);
});