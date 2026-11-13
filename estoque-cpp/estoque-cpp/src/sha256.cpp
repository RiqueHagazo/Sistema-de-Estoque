#include "sha256.hpp"

#include <cstring>

#include "erros.hpp"

namespace {

const std::uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

inline std::uint32_t gira_direita(std::uint32_t x, unsigned int n) {
    return (x >> n) | (x << (32 - n));
}

constexpr std::size_t TAMANHO_BLOCO_HMAC = 64;

}  // namespace

// ############################## SHA-256 ##############################

Sha256::Sha256() : tamanho_buffer(0), total_bytes(0) {
    reinicia();
}

void Sha256::reinicia() {
    estado[0] = 0x6a09e667; estado[1] = 0xbb67ae85; estado[2] = 0x3c6ef372; estado[3] = 0xa54ff53a;
    estado[4] = 0x510e527f; estado[5] = 0x9b05688c; estado[6] = 0x1f83d9ab; estado[7] = 0x5be0cd19;
    std::memset(buffer, 0, sizeof(buffer));
    tamanho_buffer = 0;
    total_bytes = 0;
}

void Sha256::processa_bloco(const std::uint8_t* bloco) {
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(bloco[i * 4]) << 24) | (static_cast<std::uint32_t>(bloco[i * 4 + 1]) << 16) |
               (static_cast<std::uint32_t>(bloco[i * 4 + 2]) << 8) | static_cast<std::uint32_t>(bloco[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        const std::uint32_t s0 = gira_direita(w[i - 15], 7) ^ gira_direita(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const std::uint32_t s1 = gira_direita(w[i - 2], 17) ^ gira_direita(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    std::uint32_t a = estado[0], b = estado[1], c = estado[2], d = estado[3];
    std::uint32_t e = estado[4], f = estado[5], g = estado[6], h = estado[7];
    for (int i = 0; i < 64; ++i) {
        const std::uint32_t S1 = gira_direita(e, 6) ^ gira_direita(e, 11) ^ gira_direita(e, 25);
        const std::uint32_t ch = (e & f) ^ (~e & g);
        const std::uint32_t temp1 = h + S1 + ch + K[i] + w[i];
        const std::uint32_t S0 = gira_direita(a, 2) ^ gira_direita(a, 13) ^ gira_direita(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temp2 = S0 + maj;
        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    estado[0] += a; estado[1] += b; estado[2] += c; estado[3] += d;
    estado[4] += e; estado[5] += f; estado[6] += g; estado[7] += h;
}

void Sha256::atualiza(const std::uint8_t* dados, std::size_t tamanho) {
    total_bytes += tamanho;
    while (tamanho > 0) {
        const std::size_t cabe = 64 - tamanho_buffer;
        const std::size_t copia = tamanho < cabe ? tamanho : cabe;
        std::memcpy(buffer + tamanho_buffer, dados, copia);
        tamanho_buffer += copia;
        dados += copia;
        tamanho -= copia;
        if (tamanho_buffer == 64) {
            processa_bloco(buffer);
            tamanho_buffer = 0;
        }
    }
}

void Sha256::atualiza(const std::vector<std::uint8_t>& dados) {
    atualiza(dados.data(), dados.size());
}

void Sha256::atualiza(const std::string& dados) {
    atualiza(reinterpret_cast<const std::uint8_t*>(dados.data()), dados.size());
}

Resumo256 Sha256::finaliza() {
    const std::uint64_t total_bits = total_bytes * 8;
    const std::uint8_t um = 0x80;
    const std::uint8_t zero = 0x00;
    atualiza(&um, 1);
    while (tamanho_buffer != 56) {
        atualiza(&zero, 1);
    }
    std::uint8_t comprimento[8];
    for (int i = 0; i < 8; ++i) {
        comprimento[i] = static_cast<std::uint8_t>(total_bits >> (56 - 8 * i));
    }
    atualiza(comprimento, 8);

    Resumo256 resumo{};
    for (std::size_t i = 0; i < 8; ++i) {
        resumo[i * 4] = static_cast<std::uint8_t>(estado[i] >> 24);
        resumo[i * 4 + 1] = static_cast<std::uint8_t>(estado[i] >> 16);
        resumo[i * 4 + 2] = static_cast<std::uint8_t>(estado[i] >> 8);
        resumo[i * 4 + 3] = static_cast<std::uint8_t>(estado[i]);
    }
    reinicia();
    return resumo;
}

std::string sha256_hex(const std::string& dados) {
    Sha256 sha;
    sha.atualiza(dados);
    const Resumo256 resumo = sha.finaliza();
    return bytes_para_hex(resumo.data(), resumo.size());
}

// ############################## HMAC ##############################

HmacSha256::HmacSha256(const std::vector<std::uint8_t>& chave) {
    std::vector<std::uint8_t> bloco(chave);
    if (bloco.size() > TAMANHO_BLOCO_HMAC) {      // chaves longas são substituídas pelo seu hash
        Sha256 sha;
        sha.atualiza(bloco);
        const Resumo256 resumo = sha.finaliza();
        bloco.assign(resumo.begin(), resumo.end());
    }
    bloco.resize(TAMANHO_BLOCO_HMAC, 0x00);

    std::vector<std::uint8_t> ipad(TAMANHO_BLOCO_HMAC);
    std::vector<std::uint8_t> opad(TAMANHO_BLOCO_HMAC);
    for (std::size_t i = 0; i < TAMANHO_BLOCO_HMAC; ++i) {
        ipad[i] = static_cast<std::uint8_t>(bloco[i] ^ 0x36);
        opad[i] = static_cast<std::uint8_t>(bloco[i] ^ 0x5c);
    }
    contexto_interno.atualiza(ipad);
    contexto_externo.atualiza(opad);
}

Resumo256 HmacSha256::calcula(const std::uint8_t* dados, std::size_t tamanho) const {
    Sha256 interno = contexto_interno;   // cópia: a chave não precisa ser reprocessada
    interno.atualiza(dados, tamanho);
    const Resumo256 resumo_interno = interno.finaliza();

    Sha256 externo = contexto_externo;
    externo.atualiza(resumo_interno.data(), resumo_interno.size());
    return externo.finaliza();
}

Resumo256 HmacSha256::calcula(const std::vector<std::uint8_t>& dados) const {
    return calcula(dados.data(), dados.size());
}

// ############################## PBKDF2 ##############################

std::vector<std::uint8_t> pbkdf2_hmac_sha256(const std::string& senha, const std::vector<std::uint8_t>& sal,
                                             int iteracoes, std::size_t tamanho_saida) {
    if (iteracoes < 1) {
        throw ErroValidacao("número de iterações inválido");
    }
    if (tamanho_saida < 1) {
        throw ErroValidacao("tamanho de saída inválido");
    }
    const HmacSha256 hmac(std::vector<std::uint8_t>(senha.begin(), senha.end()));

    std::vector<std::uint8_t> derivada;
    derivada.reserve(tamanho_saida);
    for (std::uint32_t bloco = 1; derivada.size() < tamanho_saida; ++bloco) {
        std::vector<std::uint8_t> entrada(sal);
        entrada.push_back(static_cast<std::uint8_t>(bloco >> 24));
        entrada.push_back(static_cast<std::uint8_t>(bloco >> 16));
        entrada.push_back(static_cast<std::uint8_t>(bloco >> 8));
        entrada.push_back(static_cast<std::uint8_t>(bloco));

        Resumo256 u = hmac.calcula(entrada);
        Resumo256 t = u;
        for (int i = 1; i < iteracoes; ++i) {
            u = hmac.calcula(u.data(), u.size());
            for (std::size_t j = 0; j < t.size(); ++j) {
                t[j] = static_cast<std::uint8_t>(t[j] ^ u[j]);
            }
        }
        for (std::size_t j = 0; j < t.size() && derivada.size() < tamanho_saida; ++j) {
            derivada.push_back(t[j]);
        }
    }
    return derivada;
}

// ############################## HEXADECIMAL ##############################

std::string bytes_para_hex(const std::uint8_t* dados, std::size_t tamanho) {
    static const char DIGITOS[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(tamanho * 2);
    for (std::size_t i = 0; i < tamanho; ++i) {
        hex += DIGITOS[dados[i] >> 4];
        hex += DIGITOS[dados[i] & 0x0F];
    }
    return hex;
}

std::string bytes_para_hex(const std::vector<std::uint8_t>& dados) {
    return bytes_para_hex(dados.data(), dados.size());
}

std::vector<std::uint8_t> hex_para_bytes(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw ErroValidacao("hexadecimal com número ímpar de dígitos");
    }
    auto valor = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        throw ErroValidacao("dígito hexadecimal inválido");
    };
    std::vector<std::uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>(valor(hex[i]) * 16 + valor(hex[i + 1])));
    }
    return bytes;
}
