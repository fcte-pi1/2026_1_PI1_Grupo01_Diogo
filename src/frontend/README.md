# Frontend

Diretório da interface de usuário (Dashboard) do projeto.

## Tecnologias
- Next.js (App Router)
- TypeScript
- Tailwind CSS

## Como Rodar Localmente

1. Acesse a pasta e instale as dependências:
    ```bash
    cd src/frontend
    npm install
    ```

2. Inicie o servidor de desenvolvimento:
    ```bash
    npm run dev
    ```

3. Acesse http://localhost:3000 no navegador.

## Estrutura de Pastas

- app/: Páginas, rotas e componentes da interface.
- public/: Arquivos estáticos como imagens e ícones.
- Raiz do diretório: Arquivos de configuração (tailwind.config.ts, tsconfig.json).

> [!WARNING]
> Código de API, rotas de banco de dados e regras de negócio do servidor devem ficar exclusivamente na pasta src/backend.