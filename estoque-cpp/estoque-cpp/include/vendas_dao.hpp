#ifndef VENDAS_DAO_HPP
#define VENDAS_DAO_HPP

#include <memory>
#include <vector>

#include "bancodado.hpp"
#include "produto_dao.hpp"
#include "vendas.hpp"

/**
 * @file vendas_dao.hpp
 * @brief Acesso aos dados de vendas (parte do antigo `dao.py`).
 */

/** @brief Grava e consulta vendas na tabela `vendas`, já com o produto e a categoria (o JOIN do original). */
class VendasDAO {
public:
    /**
     * @param banco    pasta de dados
     * @param produtos DAO usado para resolver o produto de cada venda (ambos devem viver mais que este objeto)
     */
    VendasDAO(BancoDado& banco, const ProdutoDAO& produtos);

    /**
     * @brief Grava uma nova venda e atribui o id gerado ao objeto.
     * @throws ErroRegraNegocio se a venda já foi gravada.
     * @throws ErroIntegridade se o produto vendido não existir.
     */
    void registra_vendas(Vendas& venda);

    /** @brief Todas as vendas, na ordem em que foram registradas. */
    std::vector<std::shared_ptr<Vendas>> select_registros() const;

    /**
     * @brief Vendas entre duas datas, inclusive (o `BETWEEN` do original).
     * @throws ErroValidacao se `data_inicial` for posterior a `data_final`.
     */
    std::vector<std::shared_ptr<Vendas>> pesquisar_periodo(const Data& data_inicial, const Data& data_final) const;

    /**
     * @brief Soma dos valores das vendas do período (0 se não houver vendas).
     * @throws ErroValidacao se `data_inicial` for posterior a `data_final`.
     */
    double valor_por_periodo(const Data& data_inicial, const Data& data_final) const;

private:
    BancoDado& banco;
    const ProdutoDAO& produtos;
};

#endif
