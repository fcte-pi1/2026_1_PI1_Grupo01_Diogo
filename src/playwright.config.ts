import { defineConfig } from '@playwright/test';

export default defineConfig({
  testDir: './frontend/e2e/tests',

  use: {
    baseURL: 'http://localhost:3001',
  },

  webServer: {
    command: 'cd frontend && npm run dev',
    url: 'http://localhost:3001',
    reuseExistingServer: true,
  },

  testIgnore: [
    '**/backend/**',
    '**/__tests__/**',
  ],
});