# User Stories e requisitos — Sistema de Estoque

Levantamento dos requisitos do sistema (etapa C5). O ponto de partida foi o projeto original em Python
(Flask + MySQL): as histórias descrevem o que ele já fazia e, marcadas com **(novo)**, o que foi acrescentado
nesta versão em C++.

**Papéis**

| Papel | Quem é |
|---|---|
| **Operador** | usuário comum (grupo 0): consulta produtos, cadastra categorias e produtos, registra vendas |
| **Administrador** | usuário com acesso total (grupo 1): tudo o que o operador faz, mais registro de vendas e gestão de usuários |

Cada história traz os critérios de aceitação, as classes que a implementam e os testes que a verificam
(`tests/`, 279 testes automatizados).

---

## Requisitos funcionais

### Acesso

#### US01 — Primeiro acesso **(novo)**
**Como** responsável pela instalação, **quero** criar o administrador na primeira vez que o sistema for aberto,
**para** não depender de uma senha padrão conhecida por todos.
- Se não há nenhum usuário, o sistema pede usuário, senha e a repetição da senha e cria um administrador.
- Senhas diferentes: mostra `as senhas não conferem` e pede de novo.
- Senha com menos de 4 ou mais de 128 caracteres é recusada.
- Depois que existe um usuário, essa tela não aparece mais (e o serviço recusa uma segunda criação).

*Implementação:* `Aplicacao::primeiro_acesso`, `ServicoEstoque::cadastrar_primeiro_administrador`.
*Testes:* `aplicacao_primeiro_acesso_*`, `servico_primeiro_administrador_*`.

#### US02 — Entrar no sistema
**Como** usuário, **quero** entrar com meu usuário e senha, **para** acessar as funções do sistema.
- Dados corretos: mostra `bem vindo(a) <usuário>!` e o menu principal.
- Dados incorretos: mostra `login/senha inválidos` (sem dizer qual dos dois errou) e pede de novo.
- O nome de usuário diferencia maiúsculas de minúsculas (`Maria` ≠ `maria`).
- A senha **não aparece na tela** enquanto é digitada.
- Usuário em branco encerra o programa.

*Implementação:* `Aplicacao::login`, `ServicoEstoque::login`, `UsuarioDAO::verifica_login`, `cryptar`/`valida_senha`, `EcoDesativado`.
*Testes:* `aplicacao_login_*`, `aplicacao_nunca_mostra_a_senha_digitada`, `servico_login_*`, `usuario_dao_login_*`, `terminal_*`, `crypto_*`.

#### US03 — Sair do sistema
**Como** usuário, **quero** sair da minha conta, **para** que outra pessoa possa entrar.
- A opção `0) sair` do menu principal volta à tela de login.
- O logout é registrado no histórico.

*Implementação:* `Aplicacao::logout`, `ServicoEstoque::logout`. *Testes:* `aplicacao_registra_login_e_logout_no_historico`.

#### US04 — Ver só o que o meu perfil permite
**Como** operador, **quero** que o menu mostre apenas as telas que posso usar, **para** não me confundir com opções bloqueadas.
- Operador não vê `registro` nem `administrativo`; se digitar o número delas, recebe `somente administradores`.
- Administrador vê todas as opções.
- A decisão de quem pode o quê é do próprio usuário (`pode_administrar()`), sem `if grupo == 1` espalhado pelo código.

*Implementação:* `Usuario::pode_administrar` (polimórfico), `Operador`, `Administrador`, `Aplicacao::index`, `ServicoEstoque::exige_administrador`.
*Testes:* `aplicacao_operador_nao_ve_as_telas_de_administrador`, `usuario_polimorfismo_pela_classe_base`.

### Categorias

#### US05 — Cadastrar categoria
**Como** usuário, **quero** cadastrar categorias de produtos, **para** organizar o estoque.
- O nome é obrigatório (até 255 caracteres, sem caracteres de controle).
- Não aceita categoria repetida, ignorando maiúsculas e acentos (`Pão` = `pao`): `categoria já existe`.
- Sucesso: mostra `salvo`.

*Implementação:* `ServicoEstoque::cadastrar_categoria`, `CategoriaDAO::insert_categoria`, `Categoria`.
*Testes:* `categoria_*`, `categoria_dao_*`, `servico_cadastra_categoria_*`, `aplicacao_cadastra_categoria_*`.

#### US06 — Listar categorias
**Como** usuário, **quero** ver as categorias cadastradas em ordem alfabética, **para** escolher uma ao cadastrar produtos.
- Mostra código e nome; a ordem ignora maiúsculas e acentos (`Açúcar`, `água`, `bebidas`, `Zebra`).

*Testes:* `categoria_dao_lista_em_ordem_alfabetica_ignorando_caixa_e_acentos`.

### Produtos

#### US07 — Cadastrar produto
**Como** usuário, **quero** cadastrar um produto com categoria, nome, quantidade, valor, descrição, data de chegada
e (opcionalmente) validade, **para** manter o estoque atualizado.
- A categoria precisa existir; senão: `categoria inválida`.
- Quantidade entre 0 e 1.000.000.000; valor entre 0 e 1.000.000.000, aceitando vírgula ou ponto (`19,99`).
- Data de chegada em branco = hoje. Datas inexistentes (`31/02/2026`) são recusadas.
- Com validade informada o produto é **perecível**; sem validade é **não perecível**.
- A validade não pode ser anterior à chegada.
- Entradas inválidas mostram a mensagem do problema e **não derrubam o programa**.

*Implementação:* `ServicoEstoque::cadastrar_produto`, `Produto::cria`, `ProdutoPerecivel`, `ProdutoNaoPerecivel`, `ProdutoDAO::insere_produto`.
*Testes:* `produto_*`, `produto_dao_*`, `servico_cadastr*`, `aplicacao_entradas_invalidas_nao_derrubam_o_programa`.

#### US08 — Listar produtos
**Como** usuário, **quero** ver todos os produtos em uma tabela, **para** conhecer o estoque.
- Colunas: código, nome, quantidade, valor (`R$1,234.50`), descrição, chegada, validade (`-` se não houver), categoria.
- As colunas ficam alinhadas mesmo com acentos.

*Testes:* `produto_dao_lista_com_categoria_*`, `aplicacao_tabelas_ficam_alinhadas_mesmo_com_acentos`.

#### US09 — Pesquisar produto por nome
**Como** usuário, **quero** pesquisar produtos pelo nome, **para** achar um item rapidamente.
- Compara o nome inteiro, ignorando maiúsculas, acentos e espaços nas pontas (como no sistema original).
- Sem resultados: `nenhum produto encontrado`.

*Implementação:* `ProdutoDAO::busca_produto`. *Testes:* `produto_dao_busca_por_nome_*`, `aplicacao_pesquisa_produto_por_nome`.

#### US10 — Excluir produto
**Como** usuário, **quero** excluir um produto cadastrado por engano, **para** manter o cadastro limpo.
- O sistema pede confirmação (`s/n`); `n` cancela com `operação cancelada`.
- Produto que já tem vendas **não pode** ser excluído (preserva o registro de vendas): mostra o motivo.
- Produto inexistente: `produto não encontrado`.

*Implementação:* `ServicoEstoque::excluir_produto`, `ProdutoDAO::excluir_produto`.
*Testes:* `produto_dao_nao_exclui_produto_que_tem_vendas`, `aplicacao_exclusao_de_produto_*`.

#### US11 — Repor estoque **(novo)**
**Como** usuário, **quero** acrescentar unidades a um produto existente, **para** registrar a chegada de mercadoria sem recadastrar o produto.
- A quantidade acrescentada deve ser maior que zero e o total não pode passar de 1.000.000.000.

*Implementação:* `Produto::repoe_estoque`, `ServicoEstoque::repoe_estoque`. *Testes:* `produto_repoe_estoque`, `servico_repoe_estoque`, `aplicacao_repoe_estoque`.

### Vendas

#### US12 — Registrar venda (retirada)
**Como** usuário, **quero** registrar a venda/retirada de uma quantidade de um produto, **para** manter o estoque correto e ter o histórico de vendas.
- Informo o código do produto, vejo seus dados e informo a quantidade.
- A quantidade deve ser maior que zero e não maior que o estoque; senão: `produtos insuficientes ou igual a zero` e **nada é alterado**.
- O estoque é descontado; o valor da venda é *preço × quantidade* (arredondado em centavos), usando o **preço e o estoque do cadastro** — nunca valores vindos da tela.
- A venda guarda o vendedor, a data e a hora; mostra `realizada com sucesso` e o valor.
- Se não for possível gravar a venda, o estoque volta ao valor anterior.
- Quando quem vende é administrador, o registro de vendas é exibido em seguida (como no sistema original).

*Implementação:* `ServicoEstoque::realiza_venda`, `Produto::baixa_estoque`, `VendasDAO::registra_vendas`.
*Testes:* `servico_venda_*`, `produto_baixa_estoque_*`, `aplicacao_venda_*`, `aplicacao_administrador_vende_*`, `aplicacao_operador_vende_*`.

#### US13 — Consultar o registro de vendas
**Como** administrador, **quero** ver todas as vendas, **para** acompanhar o movimento.
- Tabela com produto, categoria, vendedor, data e horário da venda, quantidade e valor.
- Operador recebe `somente administradores`.

*Implementação:* `ServicoEstoque::registro_vendas`, `VendasDAO::select_registros`.
*Testes:* `vendas_dao_*`, `servico_relatorios_de_vendas_sao_so_para_administradores`.

#### US14 — Pesquisar vendas por período e ver o total
**Como** administrador, **quero** filtrar as vendas entre duas datas e ver a soma, **para** fechar relatórios.
- Os limites do período são inclusivos.
- Mostra `valor por periodo` com a soma das vendas filtradas.
- Data inicial posterior à final, data em branco ou inexistente: mensagem de erro, sem derrubar o programa.

*Implementação:* `VendasDAO::pesquisar_periodo`, `VendasDAO::valor_por_periodo`.
*Testes:* `vendas_dao_pesquisa_por_periodo_*`, `vendas_dao_soma_*`, `aplicacao_registro_*`.

### Administração

#### US15 — Cadastrar usuário
**Como** administrador, **quero** cadastrar usuários operadores ou administradores, **para** dar acesso à equipe.
- Informo usuário, senha e grupo (0 = operador, 1 = administrador).
- Usuário repetido: `usuário já existe`. Senha fora de 4–128 caracteres ou grupo inexistente: erro.
- Sucesso: `cadastrado com sucesso`.
- A senha é guardada somente como hash.

*Implementação:* `ServicoEstoque::cadastrar_usuario`, `UsuarioDAO::cadastrar_usuario`, `cryptar`.
*Testes:* `servico_so_administrador_cadastra_usuarios`, `servico_cadastro_de_usuario_valida_os_dados`, `usuario_dao_*`.

#### US16 — Listar e excluir usuários
**Como** administrador, **quero** ver os usuários e excluir quem não deve mais ter acesso.
- A lista mostra usuário e grupo, em ordem alfabética.
- A exclusão pede confirmação e mostra `excluido`.
- **(novo)** Não é possível excluir o próprio usuário nem o único administrador (evita deixar o sistema sem acesso).

*Implementação:* `ServicoEstoque::lista_usuarios`, `ServicoEstoque::excluir_usuario`.
*Testes:* `servico_nao_exclui_*`, `servico_nao_deixa_o_sistema_sem_administrador`, `aplicacao_administrativo_cadastra_e_exclui_usuarios`.

#### US17 — Consultar o histórico de operações **(novo)**
**Como** administrador, **quero** ver quem fez o quê e quando, **para** auditar o sistema.
- São registrados: login (e tentativas falhas), logout, cadastros, vendas, reposições e exclusões — **nunca a senha**.
- `ver histórico` mostra as últimas 20 operações; sem registros: `nenhuma operação registrada`.
- Se o histórico não puder ser gravado, a operação principal continua normalmente.

*Implementação:* `Historico`, `ServicoEstoque::ultimas_operacoes`.
*Testes:* `historico_*`, `servico_registra_as_operacoes_no_historico`, `servico_continua_funcionando_se_o_historico_nao_puder_ser_gravado`.

### Funcionalidades adicionais

#### US18 — Alertas de vencimento e de estoque baixo **(novo)**
**Como** usuário, **quero** ver os produtos vencidos, os que vencem em breve e os que estão acabando, **para** agir antes de perder mercadoria.
- *Vencidos:* perecíveis com a validade já passada e ainda em estoque (o produto vence **depois** do dia da validade), do mais antigo para o mais recente.
- *A vencer:* perecíveis que vencem hoje ou nos próximos 7 dias.
- *Estoque baixo:* produtos com até 5 unidades, dos mais escassos para os menos.
- Produtos não perecíveis nunca aparecem nos alertas de vencimento (polimorfismo: `esta_vencido`, `dias_para_vencer`).

*Implementação:* `ServicoEstoque::produtos_vencidos`, `produtos_a_vencer`, `produtos_estoque_baixo`.
*Testes:* `servico_alerta_*`, `aplicacao_alertas_*`.

#### US19 — Simular uma sessão por arquivo de texto **(novo)**
**Como** professor ou desenvolvedor, **quero** executar o sistema a partir de um arquivo com as respostas, **para** demonstrar e testar sem digitar tudo.
- `./build/estoque <pasta> < exemplos/entrada_demo.txt` (ou `make demo`) roda uma sessão completa.
- A entrada vem "ecoada" na saída, e as senhas aparecem como `(oculta)`.
- Se o arquivo acabar no meio de uma tela, o programa encerra sem erro (código 0).

*Implementação:* `Aplicacao` (recebe `std::istream`/`std::ostream`), `FimDeEntrada`.
*Testes:* `aplicacao_sessao_simulada_completa_por_arquivo_de_texto`, `aplicacao_encerra_sem_erro_quando_a_entrada_acaba`.

---

## Requisitos não funcionais

| ID | Requisito | Como é atendido / verificado |
|---|---|---|
| **RNF01** | **Persistência em arquivos de texto**, sem banco de dados externo | Cada tabela é um arquivo JSON legível em `dados/` (`categoria.json`, `produto.json`, `vendas.json`, `usuarios.json`), com as mesmas tabelas e colunas do `script.sql`; o histórico é `historico.log`. Módulos `Json`, `Tabela`, `BancoDado` |
| **RNF02** | **Segurança das senhas** | Hash PBKDF2-HMAC-SHA256 (100.000 iterações, sal aleatório de 16 bytes); senha nunca gravada em texto puro (nem nos arquivos, nem no histórico); comparação em tempo constante; o login leva tempo parecido para usuário inexistente e senha errada; senha oculta na digitação e eco do terminal restaurado até com Ctrl+C. Testes `crypto_*`, `sha256_*`, `usuario_dao_*`, `terminal_*`; SHA-256/HMAC/PBKDF2 conferidos contra a biblioteca do Python |
| **RNF03** | **Robustez**: entrada inválida ou arquivo ruim não derruba o programa | Validação nas fronteiras (`converte_inteiro`, `converte_real`, `Data::de_texto`), invariantes nas entidades, hierarquia de exceções `ErroEstoque`, `try/catch` por ação de menu e em `main`. Fim da entrada encerra com código 0; erros de sistema, 1; uso incorreto, 2; erro inesperado, 3 |
| **RNF04** | **Integridade dos dados** | Gravação atômica (arquivo temporário + renomeação); arquivo corrompido nunca é sobrescrito; ids nunca reaproveitados; as regras de chave estrangeira e unicidade do banco original são aplicadas pelos DAOs; venda devolve o estoque se falhar. Testes `tabela_*`, `*_dao_*`, `servico_venda_devolve_o_estoque_*` |
| **RNF05** | **Manutenibilidade e extensibilidade** | POO com abstração (`Entidade`, `Produto`, `Usuario`, `Relogio`), encapsulamento (atributos privados + invariantes), herança e polimorfismo (`ProdutoPerecivel`/`ProdutoNaoPerecivel`, `Operador`/`Administrador`); camadas Aplicacao → ServicoEstoque → DAO → Tabela; um tipo novo de produto ou de usuário é uma nova subclasse mais uma linha na fábrica |
| **RNF06** | **Testabilidade** | 279 testes de unidade automatizados (`make testes`), sem dependências externas. O "mundo externo" é injetado — relógio (`Relogio`), teclado e tela (`std::istream`/`std::ostream`), pasta de dados — então os testes são determinísticos. Cobertura de linhas de `src/` medida com `gcov`: **97,8%** (`make cobertura`). Também roda limpo com AddressSanitizer e UBSan (`make sanitizar`) |
| **RNF07** | **Portabilidade** | C++17 padrão, `make` e g++ ≥ 9; nenhuma biblioteca externa (JSON, hash e testes são próprios). Testado em Linux com g++ 13.3. O ocultamento da senha usa a API POSIX (Linux/macOS/WSL) e é desativado automaticamente onde não existe |
| **RNF08** | **Desempenho** adequado a um pequeno comércio | Medido (g++ -O2): com 1.000 produtos e 1.000 vendas, registrar uma venda leva ≈ 28 ms, listar os produtos ≈ 8 ms e o registro de vendas ≈ 18 ms. Cada operação lê e grava o arquivo inteiro, então o custo cresce linearmente: serve para milhares de registros, não para centenas de milhares |
| **RNF09** | **Usabilidade** | Mensagens em português, as mesmas do sistema original; menus numerados; tabelas alinhadas por caracteres (não bytes), com texto longo cortado em `...`; confirmação antes de excluir; senha oculta |
| **RNF10** | **Documentação** | Comentários Doxygen em todos os contratos (`include/*.hpp`), `Doxyfile` pronto (`make doc`), README, cartões CRC e este documento |
