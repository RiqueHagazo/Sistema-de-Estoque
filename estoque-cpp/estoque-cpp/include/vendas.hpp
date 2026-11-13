#ifndef VENDAS_HPP
#define VENDAS_HPP

#include <memory>
#include <string>

#include "datahora.hpp"
#include "entidade.hpp"
#include "produto.hpp"

/**
 * @file vendas.hpp
 * @brief Registro de uma venda/retirada (tabela `vendas` do projeto original).
 */

/** @brief Uma venda de determinado produto, feita por um usuário em certa data e hora. */
class Vendas : public Entidade {
public:
    /**
     * @param id        identificador (Entidade::ID_NAO_DEFINIDO se ainda não gravada)
     * @param usuario   nome de quem realizou a venda
     * @param produto   produto vendido (não pode ser nulo)
     * @param quantidade unidades vendidas (> 0)
     * @param valor     valor total da venda (>= 0)
     * @param data      data da venda
     * @param horario   horário da venda
     * @throws ErroValidacao se algum dado for inválido.
     */
    Vendas(int id, const std::string& usuario, const std::shared_ptr<const Produto>& produto, int quantidade,
           double valor, const Data& data, const Horario& horario);

    const std::string& get_vendedor() const;
    /** @brief Produto vendido (com a categoria, para exibir o registro). */
    const Produto& get_produto_venda() const;
    int get_quantidade_venda() const;
    double get_valor_venda() const;
    const Data& get_data_venda() const;
    const Horario& get_horario_venda() const;

    Json para_json() const override;
    std::string resumo() const override;

    /**
     * @brief Reconstrói a venda a partir de uma linha do arquivo.
     * @param produto produto já resolvido (a linha guarda apenas `produto_vendido`, o id)
     * @throws ErroPersistencia se a linha estiver incompleta ou com dados inválidos.
     */
    static std::shared_ptr<Vendas> de_json(const Json& linha, const std::shared_ptr<const Produto>& produto);

private:
    std::string vendedor;
    std::shared_ptr<const Produto> produto_venda;
    int quantidade_venda;
    double valor_venda;
    Data data_venda;
    Horario horario_venda;
};

#endif
