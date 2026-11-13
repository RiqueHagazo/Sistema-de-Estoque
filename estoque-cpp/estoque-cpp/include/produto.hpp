#ifndef PRODUTO_HPP
#define PRODUTO_HPP

#include <memory>
#include <optional>
#include <string>

#include "categoria.hpp"
#include "datahora.hpp"
#include "entidade.hpp"

/**
 * @file produto.hpp
 * @brief Produtos do estoque: classe base abstrata e as duas especializações.
 *
 * No projeto original a validade era um campo opcional (`validade date` aceitando
 * NULL) tratado com `if` espalhados. Aqui a diferença virou herança: produtos
 * perecíveis têm validade e sabem se estão vencidos; os não perecíveis nunca vencem.
 */

/**
 * @brief Produto do estoque (tabela `produto`).
 *
 * Classe abstrata: os comportamentos que dependem de existir validade são
 * virtuais puros e implementados por ProdutoPerecivel e ProdutoNaoPerecivel.
 * Use Produto::cria() ou Produto::de_json() para obter a classe concreta certa.
 */
class Produto : public Entidade {
public:
    static constexpr int QUANTIDADE_MAXIMA = 1000000000;
    static constexpr double VALOR_MAXIMO = 1000000000.0;

    /**
     * @param id         identificador (Entidade::ID_NAO_DEFINIDO se ainda não gravado)
     * @param nome       nome do produto
     * @param quantidade unidades em estoque (0..QUANTIDADE_MAXIMA)
     * @param valor      preço unitário (0..VALOR_MAXIMO)
     * @param chegada    data de chegada ao estoque
     * @param descricao  descrição (opcional, pode ser vazia)
     * @param categoria  categoria à qual o produto pertence
     * @throws ErroValidacao se algum dado for inválido.
     */
    Produto(int id, const std::string& nome, int quantidade, double valor, const Data& chegada,
            const std::string& descricao, const Categoria& categoria);
    ~Produto() override = default;

    const std::string& get_nome_produto() const;
    int get_quantidade_produto() const;
    double get_valor_produto() const;
    const Data& get_chegada_produto() const;
    const std::string& get_descricao_produto() const;
    const Categoria& get_categoria_produto() const;

    void set_nome_produto(const std::string& nome);
    void set_quantidade_produto(int quantidade);
    void set_valor_produto(double valor);
    void set_descricao_produto(const std::string& descricao);

    /**
     * @brief Retira unidades do estoque (venda/retirada).
     * @throws ErroEstoqueInsuficiente ("produtos insuficientes ou igual a zero") se
     *         `quantidade_vendida` for <= 0 ou maior que o estoque; o estoque não é alterado.
     */
    void baixa_estoque(int quantidade_vendida);

    /**
     * @brief Acrescenta unidades ao estoque (reposição).
     * @throws ErroValidacao se `quantidade` for <= 0.
     * @throws ErroRegraNegocio se o total passar de QUANTIDADE_MAXIMA.
     */
    void repoe_estoque(int quantidade);

    /** @brief Valor de `quantidade` unidades (preço x quantidade, arredondado para centavos). */
    double calcula_valor(int quantidade) const;

    /** @name Comportamento polimórfico (depende de o produto ter validade) */
    ///@{
    /** @brief Validade do produto; vazio se não perecível. */
    virtual std::optional<Data> get_validade_produto() const = 0;
    /** @brief true se a validade já passou em relação a `hoje`. */
    virtual bool esta_vencido(const Data& hoje) const = 0;
    /** @brief Dias até vencer (negativo se já venceu); vazio se não perecível. */
    virtual std::optional<int> dias_para_vencer(const Data& hoje) const = 0;
    /** @brief "perecivel" ou "nao perecivel". */
    virtual std::string get_tipo() const = 0;
    ///@}

    /** @brief Validade pronta para exibir: "dd/mm/aaaa" ou "-" (como na tabela do template original). */
    std::string validade_formatada() const;

    Json para_json() const override;
    std::string resumo() const override;

    /**
     * @brief Fábrica: cria ProdutoPerecivel se houver validade, senão ProdutoNaoPerecivel.
     * @throws ErroValidacao se algum dado for inválido (inclusive validade anterior à chegada).
     */
    static std::shared_ptr<Produto> cria(int id, const std::string& nome, int quantidade, double valor,
                                         const Data& chegada, const std::optional<Data>& validade,
                                         const std::string& descricao, const Categoria& categoria);

    /**
     * @brief Reconstrói o produto a partir de uma linha do arquivo.
     * @param categoria categoria já resolvida (a linha guarda apenas o `id_categoria`)
     * @throws ErroPersistencia se a linha estiver incompleta ou com dados inválidos.
     */
    static std::shared_ptr<Produto> de_json(const Json& linha, const Categoria& categoria);

private:
    std::string nome_produto;
    int quantidade_produto;
    double valor_produto;
    Data chegada_produto;
    std::string descricao_produto;
    Categoria categoria_produto;
};

/** @brief Produto sem prazo de validade: nunca vence. */
class ProdutoNaoPerecivel : public Produto {
public:
    ProdutoNaoPerecivel(int id, const std::string& nome, int quantidade, double valor, const Data& chegada,
                        const std::string& descricao, const Categoria& categoria);

    std::optional<Data> get_validade_produto() const override;
    bool esta_vencido(const Data& hoje) const override;
    std::optional<int> dias_para_vencer(const Data& hoje) const override;
    std::string get_tipo() const override;
};

/** @brief Produto com prazo de validade. */
class ProdutoPerecivel : public Produto {
public:
    /** @throws ErroValidacao se `validade` for anterior a `chegada`, além das regras de Produto. */
    ProdutoPerecivel(int id, const std::string& nome, int quantidade, double valor, const Data& chegada,
                     const Data& validade, const std::string& descricao, const Categoria& categoria);

    std::optional<Data> get_validade_produto() const override;
    bool esta_vencido(const Data& hoje) const override;
    std::optional<int> dias_para_vencer(const Data& hoje) const override;
    std::string get_tipo() const override;

private:
    Data validade_produto;
};

#endif
