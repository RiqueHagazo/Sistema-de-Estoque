#ifndef ENTIDADE_HPP
#define ENTIDADE_HPP

#include <cstddef>
#include <string>

#include "json.hpp"

/**
 * @file entidade.hpp
 * @brief Classe base abstrata de tudo o que é gravado nos arquivos de dados.
 */

/**
 * @brief Abstração comum a Categoria, Produto, Vendas e Usuario.
 *
 * Guarda o identificador (o `id integer primary key auto_increment` do banco
 * original) e obriga cada entidade a saber se converter em JSON e se resumir em
 * uma frase. É a base do polimorfismo usado pelas camadas de persistência e de log.
 */
class Entidade {
public:
    /** @brief Valor do id de uma entidade que ainda não foi gravada. */
    static constexpr int ID_NAO_DEFINIDO = 0;
    /** @brief Tamanho máximo dos campos de texto (o `varchar(255)` do esquema original). */
    static constexpr std::size_t TAMANHO_MAXIMO_TEXTO = 255;

    explicit Entidade(int id = ID_NAO_DEFINIDO);
    virtual ~Entidade() = default;

    int get_id() const;
    /** @throws ErroValidacao se `id` for negativo. */
    void set_id(int id);
    /** @brief true se a entidade já recebeu um id (já foi gravada). */
    bool foi_persistida() const;

    /** @brief Linha JSON que representa a entidade na sua "tabela". */
    virtual Json para_json() const = 0;
    /** @brief Frase curta que identifica a entidade (usada no histórico de operações). */
    virtual std::string resumo() const = 0;

protected:
    /**
     * @brief Valida e limpa um campo de texto.
     * @param campo       nome do campo (usado na mensagem de erro)
     * @param obrigatorio se true, rejeita texto em branco
     * @return texto sem espaços nas pontas
     * @throws ErroValidacao se estiver em branco (quando obrigatório), tiver mais de
     *         TAMANHO_MAXIMO_TEXTO caracteres ou contiver caracteres de controle.
     */
    static std::string valida_texto(const std::string& valor, const std::string& campo, bool obrigatorio);

private:
    int id;
};

#endif
