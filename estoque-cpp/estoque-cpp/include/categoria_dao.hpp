#ifndef CATEGORIA_DAO_HPP
#define CATEGORIA_DAO_HPP

#include <memory>
#include <vector>

#include "bancodado.hpp"
#include "categoria.hpp"

/**
 * @file categoria_dao.hpp
 * @brief Acesso aos dados de categorias (parte do antigo `dao.py`).
 */

/** @brief Grava e consulta categorias na tabela `categoria`. */
class CategoriaDAO {
public:
    /** @param banco pasta de dados (deve viver mais que o DAO) */
    explicit CategoriaDAO(BancoDado& banco);

    /**
     * @brief Grava uma nova categoria e atribui o id gerado ao objeto.
     * @throws ErroRegraNegocio se a categoria já foi gravada (já tem id).
     * @throws ErroIntegridade ("categoria já existe") se houver outra com o mesmo nome (sem diferenciar maiúsculas/acentos).
     */
    void insert_categoria(Categoria& categoria);

    /** @brief Todas as categorias, em ordem alfabética (o `ORDER BY nome` do original). */
    std::vector<std::shared_ptr<Categoria>> select_todas_categorias() const;

    /** @throws ErroNaoEncontrado se não existir categoria com o id informado. */
    std::shared_ptr<Categoria> busca_categoria_id(int id) const;

private:
    BancoDado& banco;
};

#endif
