# Backend

Diretório da API e servidor do projeto do Micromouse.

## Tecnologias
- Node.js (v20+ recomendado para ambiente WSL/Linux)
- Express
- TypeScript
- Prisma ORM v5 (com SQLite)

## Como Rodar Localmente

1. Acesse a pasta e instale as dependências:
    ```bash
    cd src/backend
    npm install
    ```

2. Sincronize o banco de dados e gere o Prisma Client:
    ```bash
    npx prisma db push
    npx prisma generate
    ```

3. Inicie o servidor de desenvolvimento:
    ```bash
    npm run dev
    # ou diretamente: npx ts-node api/server.ts
    ```

## Estrutura de Pastas (Arquitetura MSC)

- `api/`
  - `routes/`: Definição dos endpoints da API (`/api/telemetry`).
  - `controllers/`: Gerenciamento das requisições e respostas HTTP.
  - `services/`: Regras de negócio e comunicação direta com o banco via Prisma.
  - `server.ts`: Arquivo de inicialização do servidor Express.
- `prisma/`: Esquema do banco de dados (`schema.prisma`) e arquivo local `dev.db`.

> [!TIP]
> Para testar o envio de dados em tempo real sem o robô físico, utilize o script de simulação em `/mocks/teste_stress_api.js`:
> ```bash
> node mocks/teste_stress_api.js              # ~tempo real (300ms entre envios)
> INTERVALO_MS=1 node mocks/teste_stress_api.js   # rajada (teste de estresse)
> ```
> Fluxo de demonstração: na web clique em **Iniciar Gravação**, rode o script acima e veja o caminho ser construído no minimapa; o robô encerra a corrida sozinho ao enviar `OBJETIVO_ENCONTRADO`.

> [!WARNING]
> Código de interface, componentes visuais e regras de negócio do cliente devem ficar exclusivamente na pasta `src/frontend`.