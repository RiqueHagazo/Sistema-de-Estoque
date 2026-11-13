#ifndef APLICACAO_HPP
#define APLICACAO_HPP

#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "servico_estoque.hpp"

/**
 * @file aplicacao.hpp
 * @brief Interface de console: no projeto original, o `main.py` (Flask) e os templates HTML.
 */

/** @brief Lançada quando a entrada acaba (Ctrl+D ou fim do arquivo): encerra o programa sem erro. */
class FimDeEntrada : public std::runtime_error {
public:
    FimDeEntrada() : std::runtime_error("fim da entrada") {}
};

/**
 * @brief Menus do sistema, na ordem e com os textos das páginas do original.
 *
 * Cada "rota" do Flask virou um método (`login`, `index`, `administrativo`, `produtos`,
 * `categoria`, `vendas`, `registro_vendas`, ...). A classe só conversa com o usuário:
 * lê o que foi digitado, chama o ServicoEstoque e mostra o resultado. As regras de
 * negócio ficam no serviço.
 *
 * Para simular entradas (como sugere a especificação do trabalho), basta redirecionar
 * um arquivo de texto: `./estoque < exemplos/entrada_demo.txt`. Os testes usam
 * `std::istringstream` e `std::ostringstream` no lugar do teclado e da tela.
 */
class Aplicacao {
public:
    /**
     * @param servico serviço com as regras do sistema (deve viver mais que a Aplicacao)
     * @param entrada de onde vêm as respostas (normalmente std::cin)
     * @param saida   para onde vai o texto (normalmente std::cout)
     */
    Aplicacao(ServicoEstoque& servico, std::istream& entrada, std::ostream& saida);

    /**
     * @brief Executa o programa: primeiro acesso (se preciso), login e menus, até o usuário encerrar.
     * @return código de saída do programa (0 = encerrou normalmente)
     */
    int executar();

private:
    // ############################## ENTRADA E SAIDA ##############################
    std::string pergunta(const std::string& rotulo);
    std::string pergunta_senha(const std::string& rotulo);
    int pergunta_inteiro(const std::string& rotulo, const std::string& campo);
    bool confirma(const std::string& texto);
    void mensagem(const std::string& texto);
    void titulo(const std::string& texto);
    void imprime_tabela(const std::vector<std::string>& cabecalhos, const std::vector<std::vector<std::string>>& linhas);
    void trata_erros(const std::function<void()>& acao);

    // ############################## TELAS ##############################
    void exibe_produtos(const std::vector<std::shared_ptr<Produto>>& lista, const std::string& se_vazia);
    void exibe_produto_detalhado(const Produto& produto);
    void exibe_categorias(const std::vector<std::shared_ptr<Categoria>>& lista);
    void exibe_vendas(const std::vector<std::shared_ptr<Vendas>>& lista);
    void exibe_usuarios(const std::vector<std::shared_ptr<Usuario>>& lista);

    // ############################## LOGIN ##############################
    void primeiro_acesso();
    bool login();
    void logout();

    // ############################## INDEX ##############################
    void index();

    // ############################## ADMINISTRATIVO ##############################
    void administrativo();
    void cadastrar_usuario();
    void excluir_usuario();
    void historico();

    // ############################## PRODUTO ##############################
    void produtos();
    void cadastro_produtos();
    void excluir_produto();
    void buscar_produto();
    void repor_estoque();

    // ############################## CATEGORIA ##############################
    void categoria();
    void cadastrar_categoria();

    // ############################## VENDAS ##############################
    void vendas();
    void registro_vendas();
    void pesquisa_periodo();

    // ############################## ALERTAS ##############################
    void alertas();

    ServicoEstoque& servico;
    std::istream& entrada;
    std::ostream& saida;
    bool entrada_interativa;                 ///< true se uma pessoa está digitando no terminal
    std::shared_ptr<Usuario> usuario_logado; ///< substitui `session['username']` e `grupo_logado`
};

#endif
