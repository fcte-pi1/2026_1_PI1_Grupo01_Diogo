import "@testing-library/jest-dom/vitest";
import { cleanup } from "@testing-library/react";
import { afterEach } from "vitest";

// Desmonta os componentes renderizados após cada teste para evitar
// vazamento de estado entre um teste e outro.
afterEach(() => {
  cleanup();
});
