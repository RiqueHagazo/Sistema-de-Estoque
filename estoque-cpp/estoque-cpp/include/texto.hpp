#ifndef TEXTO_HPP
#define TEXTO_HPP

#include <cstddef>
#include <string>
#include <vector>

/**
 * @file texto.hpp
 * @brief Funções utilitárias de texto: limpeza, comparação, formatação e conversão segura.
 *
 * Reúne o que no projeto original eram os filtros do template (`formata_valor`)
 * e as conversões implícitas do Python (`int()`, `float()`), agora com validação.
 */

/** @brief Remove espaços (e tabs/quebras de linha) do início e do fim. */
std::string remove_espacos(const std::string& texto);

/**
 * @brief Texto "comparável": minúsculo e sem acentos do alfabeto latino.
 *
 * Reproduz o comportamento da collation padrão do MySQL usada no projeto
 * original (sem diferenciar maiúsculas/minúsculas nem acentos).
 */
std::string normaliza(const std::string& texto);

/** @brief Igualdade entre textos após normaliza(). */
bool iguais_normalizados(const std::string& a, const std::string& b);

/** @brief Quantidade de caracteres (e não de bytes) de um texto UTF-8. */
std::size_t tamanho_utf8(const std::string& texto);

/** @brief Corta o texto em `largura` caracteres, trocando o excesso por "...". */
std::string limita_texto(const std::string& texto, std::size_t largura);

/** @brief Completa com espaços à direita até `largura` caracteres (UTF-8). */
std::string preenche(const std::string& texto, std::size_t largura);

/**
 * @brief Formata valores monetários como o filtro `formata_valor` do original (ex.: 1234.5 -> "1,234.50").
 * @return texto formatado; vazio se o valor não for finito.
 */
std::string formata_valor(double valor);

/**
 * @brief Converte texto em inteiro.
 * @param campo nome do campo, usado na mensagem de erro.
 * @throws ErroValidacao se o texto não for um inteiro válido que caiba em `int`.
 */
int converte_inteiro(const std::string& texto, const std::string& campo);

/**
 * @brief Converte texto em número real; aceita vírgula ou ponto como separador decimal.
 * @throws ErroValidacao se o texto não for um número válido.
 */
double converte_real(const std::string& texto, const std::string& campo);

/** @brief Divide o texto em partes separadas por `separador` (partes vazias são mantidas). */
std::vector<std::string> divide(const std::string& texto, char separador);

#endif
