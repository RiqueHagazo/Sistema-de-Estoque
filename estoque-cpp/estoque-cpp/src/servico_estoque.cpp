#include "servico_estoque.hpp"

#include <algorithm>

#include "erros.hpp"
#include "texto.hpp"

ServicoEstoque::ServicoEstoque(BancoDado& banco, Historico& historico, const Relogio& relogio, int custo_senha)
    : categorias(banco),
      produtos(banco),
      vendas(banco, produtos),
      usuarios(banco),
      historico(historico),
      relogio(relogio),
      custo_senha(custo_senha) {}

Data ServicoEstoque::hoje() const { return relogio.hoje(); }

void ServicoEstoque::exige_administrador(const Usuario& autor) const {
    if (!autor.pode_administrar()) {
        throw ErroPermissao("somente administradores");
    }
}

std::string ServicoEstoque::cripta_senha(const std::string& senha) const {
    if (senha.size() < TAMANHO_MINIMO_SENHA || senha.size() > TAMANHO_MAXIMO_SENHA) {
        throw ErroValidacao("a senha deve ter entre " + std::to_string(TAMANHO_MINIMO_SENHA) + " e " +
                            std::to_string(TAMANHO_MAXIMO_SENHA) + " caracteres");
    }
    return cryptar(senha, custo_senha);
}

// ############################## USUARIOS ##############################

bool ServicoEstoque::existe_usuario_cadastrado() const {
    return usuarios.conta_usuarios() > 0;
}

std::shared_ptr<Usuario> ServicoEstoque::cadastrar_primeiro_administrador(const std::string& user,
                                                                           const std::string& senha) {
    if (existe_usuario_cadastrado()) {
        throw ErroRegraNegocio("já existem usuários cadastrados");
    }
    std::shared_ptr<Usuario> administrador =
        Usuario::cria(Usuario::GRUPO_ADMINISTRADOR, Entidade::ID_NAO_DEFINIDO, user, cripta_senha(senha));
    usuarios.cadastrar_usuario(*administrador);
    historico.registra("sistema", "PRIMEIRO_ACESSO", administrador->resumo());
    return administrador;
}

std::shared_ptr<Usuario> ServicoEstoque::login(const std::string& user, const std::string& senha) {
    const std::string nome = remove_espacos(user);
    const std::shared_ptr<Usuario> usuario = usuarios.verifica_login(nome, senha);
    if (!usuario) {
        historico.registra(nome.empty() ? "(vazio)" : nome, "LOGIN_FALHOU", "");   // a senha nunca vai para o histórico
        throw ErroAutenticacao("login/senha inválidos");
    }
    historico.registra(usuario->get_usuario(), "LOGIN", usuario->get_nome_grupo());
    return usuario;
}

void ServicoEstoque::logout(const Usuario& usuario) {
    historico.registra(usuario.get_usuario(), "LOGOUT", "");
}

std::shared_ptr<Usuario> ServicoEstoque::cadastrar_usuario(const Usuario& autor, const std::string& user,
                                                            const std::string& senha, int grupo) {
    exige_administrador(autor);
    std::shared_ptr<Usuario> novo = Usuario::cria(grupo, Entidade::ID_NAO_DEFINIDO, user, cripta_senha(senha));
    usuarios.cadastrar_usuario(*novo);
    historico.registra(autor.get_usuario(), "CADASTRO_USUARIO", novo->resumo());
    return novo;
}

void ServicoEstoque::excluir_usuario(const Usuario& autor, const std::string& nome_usuario) {
    exige_administrador(autor);
    const std::string nome = remove_espacos(nome_usuario);
    const std::shared_ptr<Usuario> alvo = usuarios.busca_usuario(nome);   // ErroNaoEncontrado
    if (alvo->get_usuario() == autor.get_usuario()) {
        throw ErroRegraNegocio("não é possível excluir o próprio usuário");
    }
    if (alvo->pode_administrar() && usuarios.conta_administradores() <= 1) {
        throw ErroRegraNegocio("não é possível excluir o único administrador");
    }
    usuarios.excluir_usuario(nome);
    historico.registra(autor.get_usuario(), "EXCLUSAO_USUARIO", alvo->resumo());
}

std::vector<std::shared_ptr<Usuario>> ServicoEstoque::lista_usuarios(const Usuario& autor) const {
    exige_administrador(autor);
    return usuarios.select_usuarios();
}

// ############################## CATEGORIAS ##############################

std::shared_ptr<Categoria> ServicoEstoque::cadastrar_categoria(const Usuario& autor, const std::string& nome) {
    std::shared_ptr<Categoria> categoria = std::make_shared<Categoria>(Entidade::ID_NAO_DEFINIDO, nome);
    categorias.insert_categoria(*categoria);
    historico.registra(autor.get_usuario(), "CADASTRO_CATEGORIA", categoria->resumo());
    return categoria;
}

std::vector<std::shared_ptr<Categoria>> ServicoEstoque::lista_categorias() const {
    return categorias.select_todas_categorias();
}

// ############################## PRODUTOS ##############################

std::shared_ptr<Produto> ServicoEstoque::cadastrar_produto(const Usuario& autor, int id_categoria,
                                                            const std::string& nome, int quantidade, double valor,
                                                            const Data& chegada, const std::optional<Data>& validade,
                                                            const std::string& descricao) {
    std::shared_ptr<Categoria> categoria;
    try {
        categoria = categorias.busca_categoria_id(id_categoria);
    } catch (const ErroNaoEncontrado&) {
        throw ErroValidacao("categoria inválida");   // mesma mensagem do original
    }
    std::shared_ptr<Produto> produto =
        Produto::cria(Entidade::ID_NAO_DEFINIDO, nome, quantidade, valor, chegada, validade, descricao, *categoria);
    produtos.insere_produto(*produto);
    historico.registra(autor.get_usuario(), "CADASTRO_PRODUTO",
                       produto->resumo() + ", quantidade " + std::to_string(produto->get_quantidade_produto()));
    return produto;
}

std::vector<std::shared_ptr<Produto>> ServicoEstoque::lista_produtos() const {
    return produtos.select_todos_produtos();
}

std::vector<std::shared_ptr<Produto>> ServicoEstoque::busca_produto(const std::string& nome) const {
    return produtos.busca_produto(nome);
}

std::shared_ptr<Produto> ServicoEstoque::busca_produto_id(int id) const {
    return produtos.busca_produto_id(id);
}

void ServicoEstoque::excluir_produto(const Usuario& autor, int id_produto) {
    const std::shared_ptr<Produto> produto = produtos.busca_produto_id(id_produto);   // ErroNaoEncontrado
    produtos.excluir_produto(id_produto);                                                // ErroIntegridade
    historico.registra(autor.get_usuario(), "EXCLUSAO_PRODUTO", produto->resumo());
}

void ServicoEstoque::repoe_estoque(const Usuario& autor, int id_produto, int quantidade) {
    const std::shared_ptr<Produto> produto = produtos.busca_produto_id(id_produto);
    produto->repoe_estoque(quantidade);   // valida a quantidade e o limite
    produtos.atualiza_quantidade(id_produto, produto->get_quantidade_produto());
    historico.registra(autor.get_usuario(), "REPOSICAO",
                       produto->resumo() + ", +" + std::to_string(quantidade) + " (estoque: " +
                           std::to_string(produto->get_quantidade_produto()) + ")");
}

// ############################## VENDAS ##############################

std::shared_ptr<Vendas> ServicoEstoque::realiza_venda(const Usuario& vendedor, int id_produto, int quantidade_vendida) {
    // estoque e preço vêm sempre do cadastro, nunca da tela
    const std::shared_ptr<Produto> produto = produtos.busca_produto_id(id_produto);
    const int quantidade_anterior = produto->get_quantidade_produto();

    produto->baixa_estoque(quantidade_vendida);   // lança ErroEstoqueInsuficiente sem alterar o produto
    const double valor_venda = produto->calcula_valor(quantidade_vendida);
    std::shared_ptr<Vendas> venda = std::make_shared<Vendas>(Entidade::ID_NAO_DEFINIDO, vendedor.get_usuario(), produto,
                                                             quantidade_vendida, valor_venda, relogio.hoje(),
                                                             relogio.agora());

    produtos.atualiza_quantidade(id_produto, produto->get_quantidade_produto());
    try {
        vendas.registra_vendas(*venda);
    } catch (...) {
        // a venda não foi gravada: devolve o estoque para não "sumir" mercadoria
        try {
            produtos.atualiza_quantidade(id_produto, quantidade_anterior);
        } catch (...) {
            // melhor esforço: o erro original é o que importa para quem chamou
        }
        throw;
    }
    historico.registra(vendedor.get_usuario(), "VENDA",
                       venda->resumo() + ", total " + formata_valor(valor_venda));
    return venda;
}

std::vector<std::shared_ptr<Vendas>> ServicoEstoque::registro_vendas(const Usuario& autor) const {
    exige_administrador(autor);
    return vendas.select_registros();
}

std::vector<std::shared_ptr<Vendas>> ServicoEstoque::pesquisa_periodo(const Usuario& autor, const Data& data_inicial,
                                                                       const Data& data_final) const {
    exige_administrador(autor);
    return vendas.pesquisar_periodo(data_inicial, data_final);
}

double ServicoEstoque::valor_por_periodo(const Usuario& autor, const Data& data_inicial, const Data& data_final) const {
    exige_administrador(autor);
    return vendas.valor_por_periodo(data_inicial, data_final);
}

// ############################## ALERTAS ##############################

std::vector<std::shared_ptr<Produto>> ServicoEstoque::produtos_vencidos() const {
    const Data hoje = relogio.hoje();
    std::vector<std::shared_ptr<Produto>> vencidos;
    for (const std::shared_ptr<Produto>& produto : produtos.select_todos_produtos()) {
        if (produto->get_quantidade_produto() > 0 && produto->esta_vencido(hoje)) {   // polimorfismo
            vencidos.push_back(produto);
        }
    }
    std::stable_sort(vencidos.begin(), vencidos.end(), [](const std::shared_ptr<Produto>& a, const std::shared_ptr<Produto>& b) {
        return *a->get_validade_produto() < *b->get_validade_produto();
    });
    return vencidos;
}

std::vector<std::shared_ptr<Produto>> ServicoEstoque::produtos_a_vencer(int dias) const {
    if (dias < 0) {
        throw ErroValidacao("o número de dias não pode ser negativo");
    }
    const Data hoje = relogio.hoje();
    std::vector<std::shared_ptr<Produto>> a_vencer;
    for (const std::shared_ptr<Produto>& produto : produtos.select_todos_produtos()) {
        const std::optional<int> faltam = produto->dias_para_vencer(hoje);   // polimorfismo
        if (produto->get_quantidade_produto() > 0 && faltam && *faltam >= 0 && *faltam <= dias) {
            a_vencer.push_back(produto);
        }
    }
    std::stable_sort(a_vencer.begin(), a_vencer.end(), [&hoje](const std::shared_ptr<Produto>& a, const std::shared_ptr<Produto>& b) {
        return *a->dias_para_vencer(hoje) < *b->dias_para_vencer(hoje);
    });
    return a_vencer;
}

std::vector<std::shared_ptr<Produto>> ServicoEstoque::produtos_estoque_baixo(int limite) const {
    if (limite < 0) {
        throw ErroValidacao("o limite de estoque não pode ser negativo");
    }
    std::vector<std::shared_ptr<Produto>> baixos;
    for (const std::shared_ptr<Produto>& produto : produtos.select_todos_produtos()) {
        if (produto->get_quantidade_produto() <= limite) {
            baixos.push_back(produto);
        }
    }
    std::stable_sort(baixos.begin(), baixos.end(), [](const std::shared_ptr<Produto>& a, const std::shared_ptr<Produto>& b) {
        return a->get_quantidade_produto() < b->get_quantidade_produto();
    });
    return baixos;
}

// ############################## HISTORICO ##############################

std::vector<std::string> ServicoEstoque::ultimas_operacoes(const Usuario& autor, std::size_t quantidade) const {
    exige_administrador(autor);
    return historico.ultimas_linhas(quantidade);
}
