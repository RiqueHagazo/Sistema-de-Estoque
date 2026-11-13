#ifndef BANCODADO_HPP
#define BANCODADO_HPP

#include <string>
#include <vector>

#include "json.hpp"

/**
 * @file bancodado.hpp
 * @brief "Banco de dados" em arquivos JSON (substitui o `bancodado.py` + MySQL do original).
 */

/** @name Nomes das tabelas (iguais aos do `script.sql` original; cada uma vira `<nome>.json`) */
///@{
constexpr const char* TABELA_CATEGORIA = "categoria";
constexpr const char* TABELA_PRODUTO = "produto";
constexpr const char* TABELA_VENDAS = "vendas";
constexpr const char* TABELA_USUARIOS = "usuarios";
///@}

/**
 * @brief Uma "tabela": arquivo JSON com as linhas e o contador de ids (o `auto_increment`).
 *
 * Papel equivalente ao do `cursor` do MySQL no projeto original. Cada operação
 * lê o arquivo, altera e grava de volta imediatamente (como um `commit`).
 *
 * Formato do arquivo:
 * @code
 * {
 *   "proximo_id": 3,
 *   "linhas": [
 *     {"id": 1, "nome": "bebidas"},
 *     {"id": 2, "nome": "limpeza"}
 *   ]
 * }
 * @endcode
 *
 * Garantias: a gravação é atômica (escreve em arquivo temporário e renomeia),
 * então uma falha no meio não deixa o arquivo pela metade; arquivo corrompido
 * gera ErroPersistencia/ErroJson e nunca é sobrescrito; ids não são reaproveitados
 * após exclusões.
 */
class Tabela {
public:
    /**
     * @param caminho caminho completo do arquivo `.json`
     * @param nome    nome lógico da tabela (para mensagens de erro)
     */
    Tabela(const std::string& caminho, const std::string& nome);

    const std::string& get_nome() const;
    const std::string& get_caminho() const;

    /**
     * @brief Todas as linhas, na ordem de inserção. Tabela sem arquivo = tabela vazia.
     * @throws ErroPersistencia se o arquivo não puder ser lido ou tiver estrutura inválida.
     * @throws ErroJson se o arquivo não for um JSON válido.
     */
    std::vector<Json> select_todas() const;

    /**
     * @brief Acrescenta uma linha, atribuindo-lhe o próximo id (campo "id").
     * @return o id atribuído
     * @throws ErroValidacao se `linha` não for um objeto JSON.
     * @throws ErroPersistencia se a leitura ou a gravação falhar.
     */
    int insere(Json linha);

    /** @brief Substitui a linha de mesmo "id". @return false se não existir (nada é gravado). */
    bool atualiza(const Json& linha);

    /** @brief Remove a linha de id informado. @return false se não existir (nada é gravado). */
    bool exclui(int id);

private:
    struct Conteudo {
        int proximo_id = 1;
        std::vector<Json> linhas;
    };

    Conteudo le() const;
    void grava(const Conteudo& conteudo) const;

    std::string caminho;
    std::string nome;
};

/**
 * @brief Pasta de dados do sistema; entrega as tabelas (o `get_conexao()` do original).
 */
class BancoDado {
public:
    /**
     * @brief Abre (e cria, se preciso) a pasta de dados.
     * @throws ErroValidacao se o caminho estiver em branco.
     * @throws ErroPersistencia se a pasta não puder ser criada.
     */
    explicit BancoDado(const std::string& diretorio);

    /**
     * @brief Tabela de nome informado (somente letras minúsculas e '_').
     * @throws ErroValidacao se o nome for inválido (impede caminhos como "../x").
     */
    Tabela get_tabela(const std::string& nome) const;

    /** @brief Caminho de um arquivo qualquer dentro da pasta de dados (ex.: o histórico). */
    std::string caminho_arquivo(const std::string& nome_arquivo) const;

    const std::string& get_diretorio() const;

private:
    std::string diretorio;
};

#endif
