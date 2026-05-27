# Backend

Diretório da API e servidor do projeto.

## Tecnologias
- Node.js
- Express
- TypeScript
- Prisma ORM (com SQLite)

## Como Rodar Localmente

1. Acesse a pasta e instale as dependências:
    ```bash
    cd src/backend
    npm install
    ```

2. Execute as migrações do banco de dados (se aplicável):
    ```bash
    npx prisma db push
    ```

3. Inicie o servidor de desenvolvimento:
    ```bash
    npm run dev
    ```

## Estrutura de Pastas

- api/: Controladores, rotas e lógica de negócio do servidor.
- prisma/: Esquema do banco de dados (schema.prisma) e migrações.
- Raiz do diretório: Arquivos de configuração (tsconfig.json, package.json).

> [!WARNING]
> Código de interface, componentes visuais e regras de negócio do cliente devem ficar exclusivamente na pasta src/frontend.