# Backend

Diretório da API e servidor do projeto.

## Tecnologias
- Node.js
- Next.js
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
    cp .env.example .env
    npx prisma migrate dev
    npx prisma generate
    ```

3. Inicie o servidor de desenvolvimento:
    ```bash
    npm run dev
    ```

## Como Rodar com Docker

1. Acesse a pasta do backend:
    ```bash
    cd src/backend
    ```

2. Suba o container:
    ```bash
    docker compose up --build
    ```

    Caso sua instalacao use o Compose legado:
    ```bash
    docker-compose -f compose.yml up --build
    ```

O Dockerfile instala as dependencias, gera o Prisma Client e aplica as migrations na inicializacao do container. O SQLite fica persistido no volume `backend-sqlite-data`.

## Estrutura de Pastas

- app/: Rotas HTTP e estrutura Next.js.
- app/api/: Endpoints da API.
- services/: Camada de servico e regras de negocio.
- lib/: Clientes e adaptadores compartilhados, como Prisma.
- prisma/: Esquema do banco de dados (schema.prisma) e migrações.
- Raiz do diretório: Arquivos de configuração (tsconfig.json, package.json, Dockerfile e compose.yml).

## API de Telemetria

## Documentacao da API

Com o backend rodando, acesse:

```text
http://localhost:3000/docs
```

A pagina usa Swagger UI e permite visualizar os endpoints, payloads e executar requisicoes. A especificacao OpenAPI tambem fica disponivel em:

```text
http://localhost:3000/api/openapi
```

### `POST /api/telemetry`

Recebe JSON com os dados enviados pelo robo e registra o payload no banco.

Exemplo:
```json
{
  "robotId": "micromouse-01",
  "sessionId": "teste-labirinto-01",
  "sequence": 1,
  "batteryLevel": 87.5,
  "positionX": 2.4,
  "positionY": 1.2,
  "headingDegrees": 90,
  "linearVelocity": 0.35,
  "angularVelocity": 0.02
}
```

Respostas:
- `201 Created`: telemetria registrada.
- `400 Bad Request`: corpo da requisicao nao contem JSON valido.
- `422 Unprocessable Entity`: JSON valido, mas com campos em formato incorreto.

### `GET /api/telemetry`

Lista os ultimos registros de telemetria. Use `?limit=10` para limitar a quantidade retornada.

## Banco de Dados

O backend usa SQLite com Prisma ORM. Foi configurada a conexao local e a migration inicial para criar as tabelas `navigation_sessions` e `navigation_events`, usadas para armazenar historico de navegacao e telemetria.

> [!WARNING]
> Código de interface, componentes visuais e regras de negócio do cliente devem ficar exclusivamente na pasta src/frontend.
