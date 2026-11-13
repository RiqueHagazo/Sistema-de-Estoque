#ifndef SERVICO_ESTOQUE_HPP
#define SERVICO_ESTOQUE_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "bancodado.hpp"
#include "categoria_dao.hpp"
#include "crypto.hpp"
#include "datahora.hpp"
#include "historico.hpp"
#include "produto_dao.hpp"
#include "usuario_dao.hpp"
#include "vendas_dao.hpp"

/**
 * @file servico_estoque.hpp
 * @brief Casos de uso do sistema (a lógica que no original ficava dentro das rotas do `main.py`).
 */

/**
 * @brief Fachada do sistema: login, cadastros, vendas, relatórios e alertas.
 *
 * Reúne as regras de negócio (permissões, estoque, integridade) e o registro no
 * histórico, deixando a interface (Aplicacao) apenas com a conversa com o usuário.
 * Quem faz a operação é sempre informado (`autor`/`vendedor`), e as operações
 * restritas conferem a permissão aqui, e não na tela.
 *
 * Todos os erros previstos são exceções derivadas de ErroEstoque.
 */
class ServicoEstoque {
public:
    /** @brief Produtos que vencem em até este número de dias entram nos alertas. */
    static constexpr int DIAS_ALERTA_VENCIMENTO = 7;
    /** @brief Produtos com estoque até este valor entram nos alertas de estoque baixo. */
    static constexpr int ESTOQUE_MINIMO = 5;
    static constexpr std::size_t TAMANHO_MINIMO_SENHA = 4;
    static constexpr std::size_t TAMANHO_MAXIMO_SENHA = 128;

    /**
     * @param banco       pasta de dados
     * @param historico   registro de operações
     * @param relogio     fonte de data/hora (as vendas usam a hora dele)
     * @param custo_senha iterações do hash de senha (os testes usam um valor baixo para ficarem rápidos)
     * @note Os três primeiros parâmetros devem viver mais que o serviço.
     */
    ServicoEstoque(BancoDado& banco, Historico& historico, const Relogio& relogio,
                   int custo_senha = ITERACOES_PADRAO);
    ServicoEstoque(const ServicoEstoque&) = delete;
    ServicoEstoque& operator=(const ServicoEstoque&) = delete;

    /** @brief Data de hoje, segundo o relógio do serviço. */
    Data hoje() const;

    // ############################## USUARIOS ##############################

    /** @brief true se já existe algum usuário (senão o sistema precisa do "primeiro acesso"). */
    bool existe_usuario_cadastrado() const;

    /**
     * @brief Cria o primeiro administrador, no primeiro uso do sistema.
     * @throws ErroRegraNegocio se já existirem usuários.
     * @throws ErroValidacao se o nome ou a senha forem inválidos.
     */
    std::shared_ptr<Usuario> cadastrar_primeiro_administrador(const std::string& user, const std::string& senha);

    /**
     * @brief Autentica um usuário.
     * @throws ErroAutenticacao ("login/senha inválidos") se usuário ou senha não conferirem.
     */
    std::shared_ptr<Usuario> login(const std::string& user, const std::string& senha);

    /** @brief Registra a saída do usuário no histórico. */
    void logout(const Usuario& usuario);

    /**
     * @brief Cadastra um usuário (somente administradores).
     * @param grupo Usuario::GRUPO_OPERADOR ou Usuario::GRUPO_ADMINISTRADOR
     * @throws ErroPermissao se `autor` não for administrador.
     * @throws ErroValidacao se nome, senha (4 a 128 caracteres) ou grupo forem inválidos.
     * @throws ErroIntegridade ("usuário já existe") se o nome já estiver em uso.
     */
    std::shared_ptr<Usuario> cadastrar_usuario(const Usuario& autor, const std::string& user, const std::string& senha,
                                               int grupo);

    /**
     * @brief Exclui um usuário (somente administradores).
     * @throws ErroPermissao se `autor` não for administrador.
     * @throws ErroNaoEncontrado se o usuário não existir.
     * @throws ErroRegraNegocio se for o próprio autor ou o único administrador (evita ficar sem acesso).
     */
    void excluir_usuario(const Usuario& autor, const std::string& nome_usuario);

    /** @brief Lista os usuários (somente administradores). @throws ErroPermissao */
    std::vector<std::shared_ptr<Usuario>> lista_usuarios(const Usuario& autor) const;

    // ############################## CATEGORIAS ##############################

    /** @throws ErroValidacao se o nome for inválido; ErroIntegridade se a categoria já existir. */
    std::shared_ptr<Categoria> cadastrar_categoria(const Usuario& autor, const std::string& nome);
    std::vector<std::shared_ptr<Categoria>> lista_categorias() const;

    // ############################## PRODUTOS ##############################

    /**
     * @brief Cadastra um produto; é perecível se `validade` for informada.
     * @throws ErroValidacao ("categoria inválida") se a categoria não existir, ou se qualquer dado for inválido.
     */
    std::shared_ptr<Produto> cadastrar_produto(const Usuario& autor, int id_categoria, const std::string& nome,
                                               int quantidade, double valor, const Data& chegada,
                                               const std::optional<Data>& validade, const std::string& descricao);

    std::vector<std::shared_ptr<Produto>> lista_produtos() const;
    std::vector<std::shared_ptr<Produto>> busca_produto(const std::string& nome) const;
    /** @throws ErroNaoEncontrado */
    std::shared_ptr<Produto> busca_produto_id(int id) const;

    /** @throws ErroNaoEncontrado se não existir; ErroIntegridade se o produto já tiver vendas. */
    void excluir_produto(const Usuario& autor, int id_produto);

    /** @brief Acrescenta unidades ao estoque. @throws ErroNaoEncontrado, ErroValidacao, ErroRegraNegocio */
    void repoe_estoque(const Usuario& autor, int id_produto, int quantidade);

    // ############################## VENDAS ##############################

    /**
     * @brief Registra a retirada/venda de um produto.
     *
     * Confere o estoque no cadastro (nunca confia em valores vindos da tela), desconta a
     * quantidade, calcula o valor total e grava a venda com a data e hora do relógio. Se a
     * gravação da venda falhar, o estoque é devolvido ao valor anterior.
     *
     * @throws ErroNaoEncontrado se o produto não existir.
     * @throws ErroEstoqueInsuficiente ("produtos insuficientes ou igual a zero") se a quantidade for <= 0 ou maior que o estoque.
     */
    std::shared_ptr<Vendas> realiza_venda(const Usuario& vendedor, int id_produto, int quantidade_vendida);

    /** @brief Todas as vendas (somente administradores). @throws ErroPermissao */
    std::vector<std::shared_ptr<Vendas>> registro_vendas(const Usuario& autor) const;

    /** @brief Vendas de um período (somente administradores). @throws ErroPermissao, ErroValidacao */
    std::vector<std::shared_ptr<Vendas>> pesquisa_periodo(const Usuario& autor, const Data& data_inicial,
                                                          const Data& data_final) const;

    /** @brief Soma das vendas de um período (somente administradores). @throws ErroPermissao, ErroValidacao */
    double valor_por_periodo(const Usuario& autor, const Data& data_inicial, const Data& data_final) const;

    // ############################## ALERTAS (funcionalidade adicional) ##############################

    /** @brief Produtos perecíveis já vencidos e ainda em estoque, do mais antigo para o mais recente. */
    std::vector<std::shared_ptr<Produto>> produtos_vencidos() const;

    /**
     * @brief Produtos que vencem hoje ou nos próximos `dias` dias (e ainda estão em estoque).
     * @throws ErroValidacao se `dias` for negativo.
     */
    std::vector<std::shared_ptr<Produto>> produtos_a_vencer(int dias) const;

    /**
     * @brief Produtos com estoque menor ou igual a `limite`, dos mais escassos para os menos.
     * @throws ErroValidacao se `limite` for negativo.
     */
    std::vector<std::shared_ptr<Produto>> produtos_estoque_baixo(int limite) const;

    // ############################## HISTORICO ##############################

    /** @brief Últimas operações registradas (somente administradores). @throws ErroPermissao */
    std::vector<std::string> ultimas_operacoes(const Usuario& autor, std::size_t quantidade) const;

private:
    void exige_administrador(const Usuario& autor) const;
    std::string cripta_senha(const std::string& senha) const;

    CategoriaDAO categorias;
    ProdutoDAO produtos;
    VendasDAO vendas;
    UsuarioDAO usuarios;
    Historico& historico;
    const Relogio& relogio;
    int custo_senha;
};

#endif
