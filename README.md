# MrBombastic - Grupo 01

Repositório do projeto de um robô autônomo MicroMouse para a disciplina de Projeto Integrador 1 (PI1) da FCTE-UnB.

## Equipe

* **Arthur Guilherme Aquino** - Software
* **Arthur Luiz Silva Guedes** - Eletrônica
* **Enzo Macedo da Motta de Mello** - Estrutura
* **Felipe de Castro Quixabeira** - Eletrônica (Gerente)
* **Fernando de Melo Colli** - Energia (Gerente) e Eletrônica
* **João Igor Pereira da Costa** - Eletrônica e Software
* **Magno Luiz Vale Vieira** - Energia e Software
* **Maria Luisa Alves Rodrigues** - Eletrônica
* **Mariana Pereira Schumann** - Estrutura (Gerente)
* **Matheus de Alcantara da Silva Campos** - Software
* **Raphaela Guimarães de Araujo dos Santos** - Estrutura
* **Renan Pereira Reis** - Software (Gerente)
* **Ricardo Henrique Silva Rodrigues** - Software
* **Tiago Lemes Teixeira** - Gerente Geral
* **Vilmar Jose Fagundes dos Passos Junior** - Estrutura

## Estrutura de Pastas
* **`docs/relatorio/`**: Relatório técnico oficial (LaTeX).
* **`hw/`**: Hardware (esquemáticos e projetos de PCB).
* **`mec/`**: Mecânica (modelagem 3D e CAD).
* **`src/`**: Software (código-fonte e firmware).

---

## Padrões do Repositório

### 1. Nomenclatura de Branches
Padrão: `[ÁREA]_[NÚMERO_DA_ISSUE]-[descrição-curta]`
* **ES** - Estrutura
* **EL** - Eletrônica
* **EN** - Energia
* **SW** - Software
* *Exemplo:* `SW_12-algoritmo-mapeamento`

### 2. Mensagens de Commit
Inicie os commits com os seguintes prefixos:
* **[ADD]**: Criação de arquivos ou funcionalidades.
* **[FIX]**: Correção de bugs.
* **[DOCS]**: Atualização do relatório ou README.
* **[REF]**: Refatoração e organização de código.
* *Exemplo:* `[ADD] leitura dos sensores infravermelhos`

## Guia de Contribuição

### Tutorial: Fluxo Git (Terminal)
Siga este passo a passo para cada nova edição:
1. `git pull origin main` (Atualiza seu local)
2. `git checkout -b [NOME_DA_SUA_BRANCH]` (Cria sua ramificação)
3. Faça suas alterações.
4. `git add .`
5. `git commit -m "[PREFIXO] descrição curta"`
6. `git push origin [NOME_DA_SUA_BRANCH]`
7. Abra o **Pull Request** no site do GitHub.

### Tutorial: Relatório (Sincronização Overleaf)
Como o uso do Overleaf é manual, siga esta ordem:
1. No Overleaf, edite apenas os arquivos na pasta `editaveis/`.
2. Baixe os arquivos `.tex` alterados para sua máquina.
3. Substitua-os na pasta `docs/relatorio/editaveis/` do seu repositório local.
4. Siga o **Fluxo Git** acima para subir as mudanças.
*Observação: Não altere o `main.tex` nem envie arquivos `.pdf` no commit.*

# Como rodar o projeto?

## Rodando o Prisma na Aplicação
Como o Prisma já está configurado no projeto, utilize os comandos abaixo conforme a necessidade.

### 1. Instalar dependências (caso necessário)
```bash
npm install
```

### 2. Gerar o Prisma Client
Sempre que houver alterações no arquivo `schema.prisma`:
```bash
npx prisma generate
```

### 3. Criar uma migration
Após modificar o schema:
```bash
npx prisma migrate dev --name nome_da_migration
```

### 4. Aplicar migrations existentes
Caso esteja clonando o projeto ou atualizando o banco:
```bash
npx prisma migrate deploy
```

### 5. Sincronizar schema sem criar migration
Útil durante desenvolvimento:
```bash
npx prisma db push
```

### 6. Abrir o Prisma Studio
Interface visual para visualizar e editar dados:
```bash
npx prisma studio
```

```bash
npx prisma migrate dev
```
## Rodando o BackEnd
Acessar a pasta do projeto
```bash
cd src/backend
```
Instalar as dependências:
```bash
npm install
```
Executar a plaicação:
```bash
npm run dev
```

## Rodando o FrontEnd
Acessar a pasta do projeto
```bash
cd src/frontend
```
Instalar as dependências:
```bash
npm install
```
Executar a plaicação:
```bash
npm run dev
```
