# PI1 - MicroMouse(Grupo 1) - Diogo

Esse é o _template_ de repositório para ser utilizado pelos grupos de PI1 para organizar seu projeto. O _template_ é dividido em pastas, onde cada parte do projeto deve ser armazenada. Os arquivos a serem armazenados incluem documentação, código-fonte, arquivos de CAD, esquemáticos, arquivos de simulação de circuitos, e dados.

A organização e a correta utilização do repositório do projeto serão considerados na avaliação do grupo. Dessa forma, recomenda-se que *todos os membros* do grupo leiam as instruções deste repositório, aprendam a a utilizar o git (caso ainda não saibam) e também que o grupo combine uma estratégia de como irão utilizar o repositório em conjunto. Dessa forma não deixem de utilizar todas as ferramentas que o GitHub oferece, incluindo branchs, PRs, revisões, issues, calendários, dentre outros.

Lembrem sempre de evitar enviar arquivos muito grandes (>5MB). No caso de vídeos e outros arquivos pesados que são necessários, armazenar o arquivo em outra plataforma e colocar aqui apenas o _link_.

> [!IMPORTANT]
> A estrutura de pastas do projeto não reflete a divisão de equipes. Os membros podem e devem trabalhar nas diferentes pastas a depender da necessidade do projeto.

## Utilização

1. Crie o repositório do projeto utilizando a nomenclatura padrão no formato: `<ano>.<semestre>_PI1_Grupo<n>_<professor>`. Como um exemplo, um nome formado corretamente seria `2026.1_PI1_Grupo1_Diogo`. 

2. Crie uma equipe do projeto com a mesma nomenclatura do repositório porém com o sufixo `_Equipe`, como `2026.1_PI1_Grupo1_Diogo_Equipe`, e solicite, caso necessário, que a equipe tenha permissão de escrita no repositório do projeto.

## Tecnologia para documentação

A geração do site de documentação é realizada utilizando o [Docsify](https://docsify.js.org/), uma ferramenta que permite criar sites dinâmicos a partir de arquivos Markdown.

```shell
"Docsify generates your documentation website on the fly. Unlike GitBook, it does not generate static html files. Instead, it smartly loads and parses your Markdown files and displays them as a website. To start using it, all you need to do is create an index.html and deploy it on GitHub Pages."
```

### Instalando o docsify

Para instalar o Docsify globalmente, execute o seguinte comando:

```shell
npm i docsify-cli -g
```

### Executando localmente

Para iniciar o servidor local e visualizar a documentação, utilize o comando:

```shell
docsify serve ./docs
```
