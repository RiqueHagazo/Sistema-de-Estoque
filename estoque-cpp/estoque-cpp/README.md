# Sistema de Estoque (C++17)

Controle de **estoque, vendas e usuários** em C++17 orientado a objetos, com persistência em **arquivos JSON** e
interface de console. É a conversão de um sistema web feito em Python (Flask + MySQL) para o projeto da disciplina de
Programação Orientada a Objetos: encapsulamento, herança, polimorfismo, modularidade, tratamento de exceções,
programação defensiva e testes de unidade.

- 20 módulos em `include/` + `src/`, **sem bibliotecas externas** (JSON, hash de senha e framework de testes são próprios)
- **279 testes de unidade** · cobertura de linhas de `src/`: **97,8%** · limpo com AddressSanitizer/UBSan
- Documentação: [User Stories](docs/USER_STORIES.md) · [Cartões CRC](docs/CRC.md) · Doxygen (`make doc`)

## O que o sistema faz

| Área | Funções |
|---|---|
| **Acesso** | primeiro acesso (cria o administrador) · login/logout com senha oculta · perfis *operador* e *administrador* |
| **Categorias** | cadastrar e listar (ordem alfabética) |
| **Produtos** | cadastrar (perecível ou não) · listar · pesquisar por nome · excluir · **repor estoque** *(novo)* |
| **Vendas (retirada)** | registrar venda com controle de estoque · registro de vendas · pesquisa por período com total *(administrador)* |
| **Administração** | cadastrar e excluir usuários · **histórico de operações** *(novo)* *(administrador)* |
| **Alertas** *(novo)* | produtos vencidos · a vencer em até 7 dias · estoque baixo (até 5 unidades) |

## Como usar

### Requisitos
`g++` 9 ou superior (C++17) e `make`. Opcionais: `doxygen` (documentação) e `gcov` (já vem com o gcc, para a cobertura).
Desenvolvido e testado em Linux com g++ 13.3. Em Windows use o WSL.

### Comandos

```bash
make                  # compila o programa             -> build/estoque
make executar         # compila e executa (usa a pasta ./dados)
make testes           # roda os 279 testes de unidade
make cobertura        # roda os testes com gcov e mostra a cobertura de linhas por arquivo
make sanitizar        # roda os testes com AddressSanitizer + UBSan
make doc              # gera a documentação HTML do Doxygen  -> docs/doxygen/html/index.html
make demo             # executa uma sessão simulada a partir de exemplos/entrada_demo.txt
make limpar           # apaga build/ e docs/doxygen/
```

Executando diretamente: `./build/estoque [pasta_de_dados]` (padrão: `dados`, criada se não existir).

### Primeiro uso
Na primeira execução não existe nenhum usuário, então o sistema pede para **criar o administrador** (usuário, senha e
repetição da senha). Não há senha padrão. Depois, é só entrar com esse usuário; o administrador cadastra os demais em
`administrativo`.

```
==================================================================
 home                                       admin (administrador)
==================================================================
bem vindo(a) admin!

  1) produtos
  2) categoria
  3) retirada
  4) alertas
  5) registro            <- só para administradores
  6) administrativo      <- só para administradores
  0) sair
```

### Simulando entradas com um arquivo de texto
Como sugere a especificação do trabalho, dá para "digitar" as respostas a partir de um arquivo:

```bash
./build/estoque minha_pasta < exemplos/entrada_demo.txt      # ou:  make demo
```

O arquivo `exemplos/entrada_demo.txt` contém uma sessão completa (primeiro acesso, categorias, produtos, venda, erro de
estoque, usuário novo, alertas...) e `exemplos/saida_demo.txt` guarda a saída que ela produz (as datas e horas da saída
são as do dia em que foi gerada). Trecho de uma venda:

```
 retirada                                   admin (administrador)
==================================================================
digite o codigo do produto: 1

Código do produto     : 1
Categoria             : mercearia
Nome                  : arroz
Quantidade em estoque : 20
Valor                 : R$19.99
Descrição             : pacote de 5 kg
Data de chegada       : 01/10/2026
Data de validade      : -

Número de produtos vendidos: 2
>> realizada com sucesso
valor da venda: R$39.98
```

## Estrutura do projeto

```
.
├── Makefile              automação: compilar, testar, cobertura, sanitizers, doc, demo
├── Doxyfile              configuração do Doxygen
├── README.md             este arquivo (também é a página inicial do Doxygen)
├── include/              CONTRATOS (.hpp), todos comentados em Doxygen
├── src/                  IMPLEMENTAÇÃO (.cpp)  + main.cpp
├── tests/                testes de unidade (framework próprio em teste.hpp)
├── docs/
│   ├── USER_STORIES.md   requisitos funcionais e não funcionais
│   └── CRC.md            cartões Classe-Responsabilidade-Colaborador + diagrama de classes
└── exemplos/             entrada_demo.txt (sessão simulada) e saida_demo.txt
```

| Módulo (`include/` + `src/`) | Papel |
|---|---|
| `erros` | hierarquia de exceções (`ErroEstoque` e derivadas) |
| `json` | valor JSON com leitura e escrita |
| `datahora` | `Data`, `Horario`, `Relogio` (interface) e `RelogioSistema` |
| `texto` | limpeza, comparação sem acentos, formatação e **conversão validada** de entradas |
| `sha256`, `crypto` | SHA-256/HMAC/PBKDF2 e as funções `cryptar` / `valida_senha` |
| `entidade`, `categoria`, `produto`, `vendas`, `usuario` | **modelo**: entidades e suas subclasses |
| `bancodado` | `BancoDado` e `Tabela`: arquivos JSON com gravação atômica |
| `categoria_dao`, `produto_dao`, `vendas_dao`, `usuario_dao` | acesso aos dados + integridade |
| `historico` | histórico de operações em arquivo de texto |
| `servico_estoque` | casos de uso e regras de negócio (fachada) |
| `terminal`, `aplicacao`, `main` | interface de console |

## Arquitetura

```
  Aplicacao            conversa com o usuário (menus, tabelas, leitura e conversão de dados)
      |
  ServicoEstoque       regras de negócio, permissões, histórico, alertas
      |
  *DAO                 gravar/consultar entidades; integridade (chave estrangeira, unicidade)
      |
  BancoDado / Tabela   arquivos JSON (leitura, gravação atômica)       + Json, Data, Horario, crypto
```

Cada camada só conhece a de baixo. O "mundo externo" é **injetado**: o relógio (`Relogio`), o teclado e a tela
(`std::istream` / `std::ostream`) e a pasta de dados (`BancoDado`) chegam pelo construtor, e é por isso que os testes
são determinísticos. Diagrama de classes e cartões CRC em [docs/CRC.md](docs/CRC.md).

## Conceitos de orientação a objetos aplicados

| Conceito | Onde aparece |
|---|---|
| **Abstração** | `Entidade` (o que toda entidade gravada sabe fazer), `Produto` e `Usuario` (classes abstratas), `Relogio` (interface), `ServicoEstoque` (fachada que esconde DAOs, histórico e regras) |
| **Encapsulamento** | atributos sempre privados; invariantes garantidos no construtor e nos setters (não existe `Produto` com quantidade negativa nem `Data` inexistente); a única forma de tirar unidades do estoque é `Produto::baixa_estoque`, que valida; só `Tabela` conhece o formato dos arquivos; só `crypto.cpp` conhece o algoritmo de hash |
| **Herança** | `Entidade` → `Categoria`, `Produto`, `Vendas`, `Usuario` · `Produto` → `ProdutoPerecivel`, `ProdutoNaoPerecivel` · `Usuario` → `Operador`, `Administrador` · `Relogio` → `RelogioSistema` · `ErroEstoque` → 9 tipos de erro |
| **Polimorfismo** | `produto->esta_vencido(hoje)` e `dias_para_vencer(hoje)` (o alerta de vencimento percorre uma lista mista de produtos sem `if` por tipo) · `usuario->pode_administrar()` (o menu e as permissões não testam `grupo == 1`) · `Entidade::para_json()` / `resumo()` · `Relogio` real ou fixo · `catch (const ErroEstoque&)` captura qualquer erro do sistema |
| **Modularidade e reuso** | `Json`, `Data`, `Horario`, `texto`, `sha256`, `crypto` e `Tabela` não dependem do domínio e podem ser usados em outros projetos |
| **Boas práticas** | RAII (`EcoDesativado`, ponteiros inteligentes; nenhum `new`/`delete` no código) · `const`, `override`, `explicit` · fábricas (`Produto::cria`, `Usuario::cria`) · constantes nomeadas · injeção de dependências · funções curtas e com um só motivo para mudar |

**Como estender:** *novo tipo de produto* → subclasse de `Produto` implementando os métodos virtuais e uma linha em
`Produto::cria`; *novo perfil de usuário* → subclasse de `Usuario` e um `case` em `Usuario::cria`; *outro formato de
arquivo* → só `Tabela::le`/`Tabela::grava` mudam; *bcrypt/argon2* → só o módulo `crypto` muda.

## Programação defensiva e tratamento de exceções

Falhas previstas viram exceções tipadas (`include/erros.hpp`), tratadas na tela correspondente: o programa mostra
`>> erro: <mensagem>` e continua, sem *crash*.

```
ErroEstoque
 |-- ErroValidacao          dado inválido (campo vazio, número fora da faixa, data inexistente...)
 |-- ErroPersistencia       falha de leitura/gravação (ErroJson: JSON malformado)
 |-- ErroNaoEncontrado      registro inexistente
 |-- ErroIntegridade        chave estrangeira / unicidade (categoria inexistente, usuário repetido...)
 |-- ErroRegraNegocio       regra do domínio (ErroEstoqueInsuficiente: venda acima do estoque)
 |-- ErroAutenticacao       login/senha inválidos
 `-- ErroPermissao          somente administradores
```

Principais proteções:

- **Entradas**: tudo que o usuário digita passa por `converte_inteiro`, `converte_real` ou `Data::de_texto` (mensagem clara,
  sem `std::invalid_argument` vazando); formulários param no primeiro campo inválido.
- **Entidades**: os construtores validam e lançam `ErroValidacao`; não existe objeto inválido.
- **Arquivos**: gravação atômica (arquivo temporário + renomeação); a estrutura é validada na leitura; **arquivo
  corrompido nunca é sobrescrito**; ids nunca são reaproveitados; JSON com aninhamento excessivo é recusado.
- **Integridade referencial**: os DAOs reproduzem as chaves estrangeiras do MySQL (produto de categoria inexistente,
  venda de produto inexistente, exclusão de produto com vendas).
- **Falha no meio da venda**: se a venda não puder ser gravada, o estoque volta ao valor anterior.
- **Segurança**: senhas só como hash com sal (PBKDF2-HMAC-SHA256), comparação em tempo constante, login que não revela
  se o usuário existe, senha oculta na digitação (o eco volta mesmo com Ctrl+C), hash adulterado com custo absurdo é
  ignorado. A venda usa o **preço e o estoque do cadastro**, nunca valores vindos da tela (no projeto original eles
  viajavam em campos escondidos do formulário e podiam ser adulterados).
- **Robustez geral**: fim da entrada (Ctrl+D) encerra com código 0; arquivos de entrada com `\r\n` funcionam; o histórico
  nunca derruba a operação principal (`noexcept`); `main` captura `ErroEstoque`, `std::exception` e `...`.

## Persistência: arquivos JSON

Cada tabela do `script.sql` virou um arquivo em `dados/`, com os mesmos nomes de tabela e de coluna:

```
dados/
├── categoria.json     (id, nome)
├── produto.json       (id, nome_produto, quantidade, valor, chegada, validade, descricao, id_categoria)
├── vendas.json        (id, produto_vendido, vendedor, data_venda, horario_venda, quantidade_venda, valor_venda)
├── usuarios.json      (id, usuario, senha, grupo)
└── historico.log      uma linha por operação
```

```json
{
  "proximo_id": 3,
  "linhas": [
    {"id": 1, "nome": "mercearia"},
    {"id": 2, "nome": "laticínios"}
  ]
}
```

`proximo_id` faz o papel do `auto_increment`. O produto sem validade grava `"validade": null` (como o `NULL` do banco);
a senha é gravada como `pbkdf2-sha256$<iterações>$<sal>$<hash>`; o histórico tem linhas como
`2026-10-30 14:03:22 | maria | VENDA | venda 1 de 2 x arroz, total 39.98`.

## Do Python para o C++

Os nomes de classes, métodos, tabelas, colunas e mensagens do projeto original foram mantidos sempre que possível.

| Projeto original (Python) | Versão C++ |
|---|---|
| `bancodado.py` — `BancoDado.get_conexao()` (MySQL) | `bancodado` — `BancoDado::get_tabela()` e `Tabela` (JSON) |
| `script.sql` | arquivos `*.json` (mesmas tabelas e colunas) |
| `crypto.py` — `cryptar`, `valida_senha` (bcrypt) | `crypto` — **mesmos nomes**, PBKDF2-HMAC-SHA256 |
| `dao.py` — classes `Categoria`, `Produto`, `Vendas`, `Usuario` (modelo + SQL juntos) | modelo em `categoria`, `produto`, `vendas`, `usuario`; acesso a dados em `*_dao` |
| `insert_categoria`, `select_todas_categorias` | `CategoriaDAO::insert_categoria`, `select_todas_categorias` |
| `insere_produto`, `select_todos_produtos`, `busca_produto`, `busca_produto_id`, `excluir_produto`, `atualiza_quantidade` | `ProdutoDAO::` com os mesmos nomes |
| `registra_vendas`, `select_registros`, `pesquisar_periodo`, `valor_por_periodo` | `VendasDAO::` com os mesmos nomes |
| `cadastrar_usuario`, `verifica_login`, `select_usuarios`, `excluir_usuario` | `UsuarioDAO::` com os mesmos nomes |
| `main.py` — rotas Flask (`login`, `index`, `administrativo`, `produtos`, `categoria`, `vendas`...) | `Aplicacao` — métodos com os mesmos nomes; regras em `ServicoEstoque` |
| `templates/*.html`, `style.css` | telas de console em `Aplicacao` (mesmos títulos, colunas e mensagens) |
| filtros `formata_valor`, `formata_data` | `formata_valor()` (`texto`), `formata_data()` (`datahora`) |
| `session['username']`, `grupo_logado` | `Aplicacao::usuario_logado` (um `Usuario` polimórfico) |

**O que mudou, e por quê**

| Mudança | Motivo |
|---|---|
| Web (Flask) → console | C++ não tem Flask; menus equivalentes às rotas. A especificação também pede simular entradas por arquivo |
| Caixa de pesquisa da barra de navegação → `produtos` > `pesquisar por nome` | no console não há barra fixa; a pesquisa continua sendo por nome exato |
| MySQL → JSON | requisito do trabalho (persistência simples em arquivos de texto) |
| bcrypt → PBKDF2-HMAC-SHA256 próprio | o bcrypt não faz parte da biblioteca padrão; a interface (`cryptar`/`valida_senha`) é a mesma, e o resultado foi conferido contra a `hashlib` do Python. É didático: em produção, use uma biblioteca de bcrypt/argon2 auditada |
| `grupo == 1` → `Usuario::pode_administrar()` | exigência de polimorfismo e elimina `if`s espalhados |
| `validade` opcional → `ProdutoPerecivel` / `ProdutoNaoPerecivel` | idem; o `NULL` do banco virou tipo |
| Modelo e SQL misturados → entidade + DAO + serviço | responsabilidades bem definidas |
| Venda lê preço e estoque do cadastro | no original vinham de `<input hidden>`: um cliente podia mandar qualquer preço |
| Excluir produto com vendas dava erro 500 (FK) | agora é uma mensagem; o produto é preservado |
| Categoria repetida bloqueada · usuário não exclui a si mesmo nem o único administrador · primeiro acesso sem senha padrão | regras defensivas |
| Todas as entradas validadas | o original confiava na validação do navegador; no servidor, um valor inesperado causava erro 500 ou uma mensagem enganosa (toda falha ao cadastrar produto virava `categoria inválida`) |

> Observação sobre o projeto original: o `script.sql` não tinha a coluna `grupo` da tabela `usuarios` (usada pelo
> `dao.py`) e o `unique (nome)` apontava para uma coluna inexistente; a modelagem foi feita a partir do `dao.py`.

## Testes

```bash
make testes                       # roda tudo
./build/testes -v                 # lista cada teste
./build/testes servico_venda      # roda só os testes cujo nome contém "servico_venda"
make cobertura                    # cobertura de linhas com gcov
make sanitizar                    # AddressSanitizer + UBSan
```

Framework próprio e mínimo (`tests/teste.hpp`): `TESTE(nome)`, `VERIFICA`, `VERIFICA_IGUAL`, `VERIFICA_PROXIMO`,
`VERIFICA_EXCECAO`, `VERIFICA_EXCECAO_MSG`, `VERIFICA_NAO_LANCA`. Há um arquivo de teste por módulo
(`tests/test_<módulo>.cpp`), com casos válidos, inválidos e de borda.

| Camada | Exemplos do que é testado |
|---|---|
| `json`, `texto`, `datahora` | escapes e *unicode*, números reais com ida e volta exata, documentos malformados, anos bissextos, datas impossíveis |
| `sha256`, `crypto` | vetores oficiais (NIST, RFC 4231) e valores da `hashlib` do Python; hash adulterado ou malformado; custo absurdo |
| modelo | invariantes, polimorfismo por ponteiro da classe base, ida e volta em JSON, linhas inválidas |
| `bancodado` | ids sem reaproveitamento, gravação atômica, arquivo corrompido preservado, disco cheio (`/dev/full`) |
| DAOs | integridade (FK e unicidade), ordenação sem acentos, dados órfãos |
| `servico_estoque` | permissões, venda com *rollback*, alertas, histórico |
| `aplicacao` | sessões simuladas completas, entradas inválidas, fim de entrada, arquivo corrompido, alinhamento de tabelas com acentos |
| `terminal` | eco do terminal desligado/religado em um pseudoterminal real, inclusive quando o processo é interrompido por sinal |

**Cobertura** (`make cobertura`): 97,8% das 1.959 linhas de `src/`. As linhas descobertas são, quase todas, ramos
defensivos que só se provocam com falha real do sistema operacional (erro de `open`/`rename`, `bad_alloc`, `localtime`
nulo) e o tratador de sinal do terminal, que só roda em processos-filho encerrados por sinal (os testes verificam o
comportamento, mas esse processo não consegue gravar os contadores do `gcov`).

**TDD ao evoluir o projeto:** (1) escreva um teste que falhe para o comportamento novo (`make testes` fica vermelho);
(2) implemente o mínimo para passar; (3) refatore com os testes verdes; (4) confira `make cobertura`.

## Documentação

- **Doxygen**: `make doc` → `docs/doxygen/html/index.html` (instale com `sudo apt install doxygen`; com Graphviz, mude
  `HAVE_DOT` para `YES` no `Doxyfile` e saem também os diagramas de classes). Todos os contratos em `include/` têm
  comentários Doxygen (`@brief`, `@param`, `@return`, `@throws`).
- [docs/USER_STORIES.md](docs/USER_STORIES.md): 19 histórias com critérios de aceitação + 10 requisitos não funcionais.
- [docs/CRC.md](docs/CRC.md): cartões CRC de todas as classes e diagrama de classes.

## Roteiro por etapas da disciplina

| Etapa | Entregável | Arquivos | Sugestão de commit |
|---|---|---|---|
| **C5** Requisitos e modelagem | User Stories + cartões CRC | `docs/USER_STORIES.md`, `docs/CRC.md` | `docs: requisitos (user stories) e modelagem (CRC)` |
| **C6** Contratos e documentação | `.hpp` + README + Doxygen | `include/`, `README.md`, `Doxyfile` | `feat: contratos (.hpp) com Doxygen e README` |
| **C7** Testes e Makefile | testes de unidade + automação | `tests/`, `Makefile` | `test: testes de unidade e Makefile` |
| **C8** Implementação | comportamentos | `src/`, `exemplos/` | `feat: implementação (.cpp) e sessão de demonstração` |
| **C9** Defensiva e exceções | robustez | `include/erros.hpp` e as validações/ramos de erro em `src/`, com seus testes | `feat: programação defensiva e tratamento de exceções` |

## Limitações conhecidas

- Uso por **um processo de cada vez**: não há controle de concorrência entre duas instâncias gravando os mesmos arquivos.
- Cada operação lê e regrava o arquivo inteiro: serve para milhares de registros (com 1.000 produtos e 1.000 vendas, uma
  venda leva cerca de 30 ms), não para centenas de milhares.
- O hash de senha é uma implementação própria e didática; para uso real, troque por uma biblioteca de bcrypt/argon2.
- O ocultamento da senha usa a API POSIX (Linux, macOS, WSL); onde ela não existe, a senha fica visível.
- Como no original, a busca por nome é por nome **exato** (sem diferenciar maiúsculas e acentos) e não há edição de
  produto (apenas reposição de estoque e exclusão).
- O módulo JSON implementa o necessário para este projeto; não valida UTF-8.
