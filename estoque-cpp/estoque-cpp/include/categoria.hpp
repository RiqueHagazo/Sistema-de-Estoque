#ifndef CATEGORIA_HPP
#define CATEGORIA_HPP

#include <string>

#include "entidade.hpp"

/**
 * @file categoria.hpp
 * @brief Categoria de produtos (tabela `categoria` do projeto original).
 */

/** @brief Agrupa produtos (ex.: "bebidas", "limpeza"). */
class Categoria : public Entidade {
public:
    /**
     * @param id   identificador (Entidade::ID_NAO_DEFINIDO se ainda não gravada)
     * @param nome nome da categoria
     * @throws ErroValidacao se o nome estiver em branco, for longo demais ou tiver caracteres de controle.
     */
    Categoria(int id, const std::string& nome);

    const std::string& get_nome() const;
    /** @throws ErroValidacao pelas mesmas regras do construtor. */
    void set_nome(const std::string& nome);

    Json para_json() const override;
    std::string resumo() const override;

    /**
     * @brief Reconstrói a categoria a partir de uma linha do arquivo.
     * @throws ErroPersistencia se a linha estiver incompleta ou com dados inválidos.
     */
    static Categoria de_json(const Json& linha);

private:
    std::string nome_categoria;
};

#endif
