#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <string>

/**
 * @file crypto.hpp
 * @brief Armazenamento seguro de senhas (equivalente ao `crypto.py` do projeto original).
 *
 * As duas funções públicas mantêm os nomes do original (`cryptar` e
 * `valida_senha`). Por dentro, usam PBKDF2-HMAC-SHA256 com sal aleatório, no
 * lugar do bcrypt (que não existe na biblioteca padrão do C++). Como o resto do
 * sistema só enxerga estas duas funções, trocar o algoritmo por uma biblioteca
 * de bcrypt exige alterar apenas `crypto.cpp`.
 *
 * Formato do hash: `pbkdf2-sha256$<iteracoes>$<sal em hex>$<hash em hex>`
 */

/** @brief Iterações usadas por padrão ao gerar um novo hash. */
constexpr int ITERACOES_PADRAO = 100000;

/** @brief Maior número de iterações aceito ao validar (protege contra arquivos adulterados). */
constexpr int ITERACOES_MAXIMAS = 5000000;

/**
 * @brief Gera o hash de uma senha (com sal aleatório: a mesma senha gera hashes diferentes).
 * @param chave     senha em texto puro
 * @param iteracoes custo do hash; quanto maior, mais lento para quem tenta adivinhar senhas
 * @throws ErroValidacao se `iteracoes` estiver fora da faixa 1..ITERACOES_MAXIMAS.
 */
std::string cryptar(const std::string& chave, int iteracoes = ITERACOES_PADRAO);

/**
 * @brief Confere uma senha contra um hash gerado por cryptar().
 * @return true se a senha confere; false se não confere ou se `hash_chave` estiver malformado.
 * @note A comparação leva o mesmo tempo independentemente de onde os textos diferem.
 */
bool valida_senha(const std::string& chave, const std::string& hash_chave);

/** @brief Informa se o texto tem o formato de um hash produzido por cryptar(). */
bool eh_hash_valido(const std::string& hash_chave);

#endif
