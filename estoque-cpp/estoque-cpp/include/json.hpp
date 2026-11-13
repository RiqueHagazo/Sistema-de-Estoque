#ifndef JSON_HPP
#define JSON_HPP

#include <cstddef>
#include <string>
#include <vector>

/**
 * @file json.hpp
 * @brief Valor JSON mínimo, com leitura (parser) e escrita em texto.
 */

/**
 * @brief Valor JSON: nulo, booleano, inteiro, real, texto, array ou objeto.
 *
 * Substitui o banco de dados do projeto original: cada "tabela" é gravada em um
 * arquivo de texto neste formato. Os objetos preservam a ordem em que os campos
 * foram definidos, o que deixa os arquivos legíveis. Falhas de leitura ou de
 * tipo lançam ErroJson.
 */
class Json {
public:
    /** @brief Tipo do valor armazenado. */
    enum class Tipo { NULO, BOOLEANO, INTEIRO, REAL, TEXTO, ARRAY, OBJETO };

    Json() = default;                       ///< valor nulo
    Json(std::nullptr_t) {}                 ///< valor nulo
    Json(bool valor);
    Json(int valor);
    Json(long long valor);
    Json(double valor);
    Json(const char* valor);
    Json(const std::string& valor);

    /** @brief Cria um array vazio. */
    static Json array();
    /** @brief Cria um objeto vazio. */
    static Json objeto();
    /**
     * @brief Interpreta um texto JSON.
     * @throws ErroJson se o texto for malformado (a mensagem informa linha e coluna).
     */
    static Json de_texto(const std::string& texto);

    Tipo get_tipo() const;
    bool eh_nulo() const;
    bool eh_texto() const;
    bool eh_numero() const;
    bool eh_array() const;
    bool eh_objeto() const;

    /** @name Leitura tipada (lançam ErroJson se o tipo não confere) */
    ///@{
    bool como_booleano() const;
    long long como_inteiro() const;
    /** @brief Inteiro que cabe em `int`; lança ErroJson se estiver fora da faixa. */
    int como_int() const;
    /** @brief Aceita número inteiro ou real. */
    double como_real() const;
    const std::string& como_texto() const;
    ///@}

    /** @name Arrays */
    ///@{
    /** @brief Acrescenta um item ao array; retorna *this para encadear chamadas. */
    Json& adiciona(const Json& valor);
    std::size_t tamanho() const;                    ///< nº de itens (array) ou de campos (objeto)
    const Json& item(std::size_t indice) const;
    const std::vector<Json>& itens() const;         ///< itens do array (ou valores do objeto)
    ///@}

    /** @name Objetos */
    ///@{
    /** @brief Define (ou substitui) um campo; retorna *this para encadear chamadas. */
    Json& define(const std::string& chave, const Json& valor);
    bool tem_campo(const std::string& chave) const;
    /** @throws ErroJson se o campo não existir. */
    const Json& campo(const std::string& chave) const;
    const std::vector<std::string>& chaves() const;
    ///@}

    /**
     * @brief Converte para texto JSON.
     * @param indentacao espaços por nível; valor negativo gera texto compacto.
     * @throws ErroJson se houver número não finito (NaN/infinito).
     */
    std::string para_texto(int indentacao = 2) const;

    /** @brief Comparação profunda (inteiro e real são tipos diferentes). */
    bool operator==(const Json& outro) const;
    bool operator!=(const Json& outro) const { return !(*this == outro); }

private:
    void exige(Tipo esperado, const char* nome) const;

    Tipo tipo = Tipo::NULO;
    bool booleano = false;
    long long inteiro = 0;
    double real = 0.0;
    std::string texto;
    std::vector<Json> itens_;           // itens do array ou valores do objeto
    std::vector<std::string> chaves_;   // chaves do objeto (mesma ordem de itens_)
};

#endif
