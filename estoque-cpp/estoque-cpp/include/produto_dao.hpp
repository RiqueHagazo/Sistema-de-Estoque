#ifndef PRODUTO_DAO_HPP
#define PRODUTO_DAO_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bancodado.hpp"
#include "produto.hpp"

/**
 * @file produto_dao.hpp
 * @brief Acesso aos dados de produtos (parte do antigo `dao.py`).
 */

/**
 * @brief Grava e consulta produtos na tabela `produto`.
 *
 * Faz o papel das chaves estrangeiras do banco original: não aceita produto de
 * categoria inexistente e não deixa excluir produto que já tem vendas.
 */
class ProdutoDAO {
public:
    explicit ProdutoDAO(BancoDado& banco);

    /**
     * @brief Grava um novo produto e atribui o id gerado ao objeto.
     * @throws ErroRegraNegocio se o produto já foi gravado.
     * @throws ErroIntegridade ("categoria inválida") se a categoria do produto não existir.
     */
    void insere_produto(Produto& produto);

    /** @brief Todos os produtos, na ordem de cadastro, já com a categoria (o antigo JOIN). */
    std::vector<std::shared_ptr<Produto>> select_todos_produtos() const;

    /** @brief Produtos com o nome informado (ignora maiúsculas e acentos, como a collation do MySQL). */
    std::vector<std::shared_ptr<Produto>> busca_produto(const std::string& nome_p) const;

    /** @throws ErroNaoEncontrado se não existir produto com o id informado. */
    std::shared_ptr<Produto> busca_produto_id(int id) const;

    /**
     * @brief Exclui um produto.
     * @throws ErroNaoEncontrado se não existir.
     * @throws ErroIntegridade se o produto possuir vendas registradas.
     */
    void excluir_produto(int id_produto);

    /**
     * @brief Define a quantidade em estoque de um produto.
     * @throws ErroValidacao se a quantidade estiver fora de 0..Produto::QUANTIDADE_MAXIMA.
     * @throws ErroNaoEncontrado se o produto não existir.
     */
    void atualiza_quantidade(int id, int quantidade);

private:
    std::map<int, Categoria> carrega_categorias() const;
    std::shared_ptr<Produto> converte(const Json& linha, const std::map<int, Categoria>& categorias) const;
    bool possui_vendas(int id_produto) const;

    BancoDado& banco;
};

#endif
