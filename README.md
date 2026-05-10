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
