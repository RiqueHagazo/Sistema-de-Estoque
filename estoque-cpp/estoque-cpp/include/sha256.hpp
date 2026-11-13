#ifndef SHA256_HPP
#define SHA256_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @file sha256.hpp
 * @brief SHA-256, HMAC-SHA256 e PBKDF2-HMAC-SHA256 (RFC 6234, RFC 2104 e RFC 8018).
 *
 * Primitivas usadas por crypto.hpp para guardar senhas. O módulo existe porque
 * o `bcrypt` do projeto original não faz parte da biblioteca padrão do C++.
 */

/** @brief Resultado de um SHA-256: 32 bytes. */
using Resumo256 = std::array<std::uint8_t, 32>;

/** @brief Cálculo incremental de SHA-256. A classe é copiável (copiar = "salvar o estado"). */
class Sha256 {
public:
    Sha256();

    /** @brief Acrescenta dados à mensagem. */
    void atualiza(const std::uint8_t* dados, std::size_t tamanho);
    void atualiza(const std::vector<std::uint8_t>& dados);
    void atualiza(const std::string& dados);

    /** @brief Conclui o cálculo e devolve o resumo; o objeto volta ao estado inicial. */
    Resumo256 finaliza();

private:
    void reinicia();
    void processa_bloco(const std::uint8_t* bloco);

    std::uint32_t estado[8];
    std::uint8_t buffer[64];
    std::size_t tamanho_buffer;
    std::uint64_t total_bytes;
};

/** @brief SHA-256 de um texto, em hexadecimal minúsculo (64 caracteres). */
std::string sha256_hex(const std::string& dados);

/** @brief HMAC-SHA256 com a chave "pré-processada" uma única vez (rápido em laços como o PBKDF2). */
class HmacSha256 {
public:
    explicit HmacSha256(const std::vector<std::uint8_t>& chave);
    Resumo256 calcula(const std::uint8_t* dados, std::size_t tamanho) const;
    Resumo256 calcula(const std::vector<std::uint8_t>& dados) const;

private:
    Sha256 contexto_interno;   // já alimentado com (chave XOR ipad)
    Sha256 contexto_externo;   // já alimentado com (chave XOR opad)
};

/**
 * @brief PBKDF2 com HMAC-SHA256.
 * @param senha        senha em texto
 * @param sal          sal aleatório
 * @param iteracoes    número de iterações (>= 1)
 * @param tamanho_saida bytes a derivar (>= 1)
 * @throws ErroValidacao se `iteracoes` ou `tamanho_saida` forem inválidos.
 */
std::vector<std::uint8_t> pbkdf2_hmac_sha256(const std::string& senha, const std::vector<std::uint8_t>& sal,
                                             int iteracoes, std::size_t tamanho_saida);

/** @brief Bytes em hexadecimal minúsculo. */
std::string bytes_para_hex(const std::uint8_t* dados, std::size_t tamanho);
std::string bytes_para_hex(const std::vector<std::uint8_t>& dados);

/** @brief Hexadecimal em bytes. @throws ErroValidacao se o texto não for hexadecimal válido. */
std::vector<std::uint8_t> hex_para_bytes(const std::string& hex);

#endif
