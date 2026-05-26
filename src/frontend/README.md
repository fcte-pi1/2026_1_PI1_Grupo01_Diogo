Frontend
Diretório da interface de usuário (Dashboard) do projeto.

Tecnologias
Next.js (App Router)

TypeScript

Tailwind CSS

Como Rodar Localmente
Acesse a pasta e instale as dependências:

Bash
cd src/frontend
npm install
Inicie o servidor de desenvolvimento:

Bash
npm run dev
Acesse http://localhost:3000 no navegador.

Estrutura de Pastas
app/: Páginas, rotas e componentes da interface.

public/: Arquivos estáticos como imagens e ícones.

Raiz do diretório: Arquivos de configuração (tailwind.config.ts, tsconfig.json).

[!WARNING]
Código de API, rotas de banco de dados e regras de negócio do servidor devem ficar exclusivamente na pasta src/backend.