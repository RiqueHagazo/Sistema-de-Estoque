#include "crypto.hpp"

#include <cstdint>
#include <random>
#include <vector>

#include "erros.hpp"
#include "sha256.hpp"
#include "texto.hpp"

namespace {

const std::string PREFIXO = "pbkdf2-sha256";
constexpr std::size_t TAMANHO_SAL = 16;
constexpr std::size_t TAMANHO_HASH = 32;

struct HashInterpretado {
    int iteracoes = 0;
    std::vector<std::uint8_t> sal;
    std::vector<std::uint8_t> hash;
};

// Devolve false (sem lançar exceção) se o texto não for um hash no formato esperado.
bool interpreta_hash(const std::string& texto, HashInterpretado& resultado) {
    const std::vector<std::string> partes = divide(texto, '$');
    if (partes.size() != 4 || partes[0] != PREFIXO) {
        return false;
    }
    try {
        resultado.iteracoes = converte_inteiro(partes[1], "iteracoes");
        resultado.sal = hex_para_bytes(partes[2]);
        resultado.hash = hex_para_bytes(partes[3]);
    } catch (const ErroValidacao&) {
        return false;
    }
    return resultado.iteracoes >= 1 && resultado.iteracoes <= ITERACOES_MAXIMAS &&
           resultado.sal.size() == TAMANHO_SAL && resultado.hash.size() == TAMANHO_HASH;
}

std::vector<std::uint8_t> gera_sal() {
    std::random_device origem;
    std::vector<std::uint8_t> sal(TAMANHO_SAL);
    for (std::uint8_t& byte : sal) {
        byte = static_cast<std::uint8_t>(origem() & 0xFF);
    }
    return sal;
}

// Comparação sem "saída antecipada": o tempo não revela em que posição os bytes diferem.
bool iguais_em_tempo_constante(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    std::uint8_t diferenca = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        diferenca = static_cast<std::uint8_t>(diferenca | (a[i] ^ b[i]));
    }
    return diferenca == 0;
}

}  // namespace

std::string cryptar(const std::string& chave, int iteracoes) {
    if (iteracoes < 1 || iteracoes > ITERACOES_MAXIMAS) {
        throw ErroValidacao("número de iterações fora da faixa permitida");
    }
    const std::vector<std::uint8_t> sal = gera_sal();
    const std::vector<std::uint8_t> hash = pbkdf2_hmac_sha256(chave, sal, iteracoes, TAMANHO_HASH);
    return PREFIXO + "$" + std::to_string(iteracoes) + "$" + bytes_para_hex(sal) + "$" + bytes_para_hex(hash);
}

bool valida_senha(const std::string& chave, const std::string& hash_chave) {
    HashInterpretado esperado;
    if (!interpreta_hash(hash_chave, esperado)) {
        return false;
    }
    const std::vector<std::uint8_t> calculado =
        pbkdf2_hmac_sha256(chave, esperado.sal, esperado.iteracoes, TAMANHO_HASH);
    return iguais_em_tempo_constante(calculado, esperado.hash);
}

bool eh_hash_valido(const std::string& hash_chave) {
    HashInterpretado ignorado;
    return interpreta_hash(hash_chave, ignorado);
}
