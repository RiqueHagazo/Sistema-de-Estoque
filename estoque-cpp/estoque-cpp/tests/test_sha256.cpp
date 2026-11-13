#include <string>
#include <vector>

#include "erros.hpp"
#include "sha256.hpp"
#include "teste.hpp"

namespace {
std::vector<std::uint8_t> bytes(const std::string& texto) {
    return std::vector<std::uint8_t>(texto.begin(), texto.end());
}
std::string hmac_hex(const std::vector<std::uint8_t>& chave, const std::string& mensagem) {
    const Resumo256 resumo = HmacSha256(chave).calcula(bytes(mensagem));
    return bytes_para_hex(resumo.data(), resumo.size());
}
}  // namespace

// vetores oficiais (NIST / RFC 6234)
TESTE(sha256_vetores_oficiais) {
    VERIFICA_IGUAL(sha256_hex(""), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    VERIFICA_IGUAL(sha256_hex("abc"), std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    VERIFICA_IGUAL(sha256_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
                   std::string("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
}

TESTE(sha256_um_milhao_de_letras_a) {
    VERIFICA_IGUAL(sha256_hex(std::string(1000000, 'a')),
                   std::string("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
}

TESTE(sha256_incremental_igual_ao_de_uma_vez) {
    const std::string mensagem = "a rapida raposa marrom pula sobre o cao preguicoso, repetidas vezes, ate passar de um bloco";
    Sha256 incremental;
    for (const char c : mensagem) {
        const std::uint8_t byte = static_cast<std::uint8_t>(c);
        incremental.atualiza(&byte, 1);
    }
    const Resumo256 resumo = incremental.finaliza();
    VERIFICA_IGUAL(bytes_para_hex(resumo.data(), resumo.size()), sha256_hex(mensagem));
}

TESTE(sha256_pode_ser_reutilizado_apos_finalizar) {
    Sha256 sha;
    sha.atualiza(std::string("lixo"));
    sha.finaliza();
    sha.atualiza(std::string("abc"));
    const Resumo256 resumo = sha.finaliza();
    VERIFICA_IGUAL(bytes_para_hex(resumo.data(), resumo.size()),
                   std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
}

// vetores da RFC 4231
TESTE(hmac_sha256_vetores_da_rfc_4231) {
    VERIFICA_IGUAL(hmac_hex(std::vector<std::uint8_t>(20, 0x0b), "Hi There"),
                   std::string("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"));
    VERIFICA_IGUAL(hmac_hex(bytes("Jefe"), "what do ya want for nothing?"),
                   std::string("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
}

TESTE(hmac_sha256_chave_maior_que_o_bloco) {
    VERIFICA_IGUAL(hmac_hex(std::vector<std::uint8_t>(131, 0xaa), "Test Using Larger Than Block-Size Key - Hash Key First"),
                   std::string("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"));
}

TESTE(hmac_sha256_pode_calcular_varias_mensagens_com_a_mesma_chave) {
    const HmacSha256 hmac(bytes("Jefe"));
    const Resumo256 primeira = hmac.calcula(bytes("what do ya want for nothing?"));
    const Resumo256 outra = hmac.calcula(bytes("outra mensagem"));
    const Resumo256 repetida = hmac.calcula(bytes("what do ya want for nothing?"));
    VERIFICA(primeira == repetida);
    VERIFICA(primeira != outra);
}

TESTE(pbkdf2_vetores_conhecidos) {
    const std::string senha = "password";
    const std::vector<std::uint8_t> sal = bytes("salt");
    VERIFICA_IGUAL(bytes_para_hex(pbkdf2_hmac_sha256(senha, sal, 1, 32)),
                   std::string("120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b"));
    VERIFICA_IGUAL(bytes_para_hex(pbkdf2_hmac_sha256(senha, sal, 2, 32)),
                   std::string("ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43"));
    VERIFICA_IGUAL(bytes_para_hex(pbkdf2_hmac_sha256(senha, sal, 4096, 32)),
                   std::string("c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a"));
}

TESTE(pbkdf2_saida_maior_que_um_bloco) {
    VERIFICA_IGUAL(bytes_para_hex(pbkdf2_hmac_sha256("passwordPASSWORDpassword",
                                                      bytes("saltSALTsaltSALTsaltSALTsaltSALTsalt"), 4096, 40)),
                   std::string("348c89dbcbd32b2f32d814b8116e84cf2b17347ebc1800181c4e2a1fb8dd53e1c635518c7dac47e9"));
}

TESTE(pbkdf2_rejeita_parametros_invalidos) {
    VERIFICA_EXCECAO(pbkdf2_hmac_sha256("x", bytes("sal"), 0, 32), ErroValidacao);
    VERIFICA_EXCECAO(pbkdf2_hmac_sha256("x", bytes("sal"), -5, 32), ErroValidacao);
    VERIFICA_EXCECAO(pbkdf2_hmac_sha256("x", bytes("sal"), 1, 0), ErroValidacao);
}

TESTE(hexadecimal_ida_e_volta) {
    const std::vector<std::uint8_t> original = {0x00, 0x01, 0xab, 0xff, 0x10};
    VERIFICA_IGUAL(bytes_para_hex(original), std::string("0001abff10"));
    VERIFICA(hex_para_bytes("0001abff10") == original);
    VERIFICA(hex_para_bytes("0001ABFF10") == original);   // maiúsculas também valem
    VERIFICA(hex_para_bytes("").empty());
}

TESTE(hexadecimal_invalido_lanca_erro) {
    VERIFICA_EXCECAO(hex_para_bytes("abc"), ErroValidacao);     // número ímpar de dígitos
    VERIFICA_EXCECAO(hex_para_bytes("zz"), ErroValidacao);
    VERIFICA_EXCECAO(hex_para_bytes("0g"), ErroValidacao);
}
