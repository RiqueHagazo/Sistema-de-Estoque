#include <string>

#include "crypto.hpp"
#include "erros.hpp"
#include "teste.hpp"
#include "texto.hpp"

namespace {
constexpr int ITERACOES_RAPIDAS = 10;   // deixa os testes rápidos; o padrão (10000) é testado à parte
}

TESTE(crypto_hash_tem_o_formato_documentado) {
    const std::string hash = cryptar("senha123", ITERACOES_RAPIDAS);
    const auto partes = divide(hash, '$');
    VERIFICA_IGUAL(partes.size(), 4u);
    VERIFICA_IGUAL(partes[0], std::string("pbkdf2-sha256"));
    VERIFICA_IGUAL(partes[1], std::string("10"));
    VERIFICA_IGUAL(partes[2].size(), 32u);   // sal de 16 bytes em hexadecimal
    VERIFICA_IGUAL(partes[3].size(), 64u);   // hash de 32 bytes em hexadecimal
    VERIFICA(eh_hash_valido(hash));
}

TESTE(crypto_nao_guarda_a_senha_em_texto_puro) {
    const std::string hash = cryptar("minhaSenhaSecreta", ITERACOES_RAPIDAS);
    VERIFICA(hash.find("minhaSenhaSecreta") == std::string::npos);
}

TESTE(crypto_valida_senha_correta_e_rejeita_incorreta) {
    const std::string hash = cryptar("senha123", ITERACOES_RAPIDAS);
    VERIFICA(valida_senha("senha123", hash));
    VERIFICA(!valida_senha("senha124", hash));
    VERIFICA(!valida_senha("SENHA123", hash));
    VERIFICA(!valida_senha("", hash));
    VERIFICA(!valida_senha("senha123 ", hash));
}

TESTE(crypto_mesma_senha_gera_hashes_diferentes_por_causa_do_sal) {
    const std::string primeiro = cryptar("igual", ITERACOES_RAPIDAS);
    const std::string segundo = cryptar("igual", ITERACOES_RAPIDAS);
    VERIFICA(primeiro != segundo);
    VERIFICA(valida_senha("igual", primeiro));
    VERIFICA(valida_senha("igual", segundo));
}

TESTE(crypto_aceita_senha_vazia_e_com_acentos) {
    VERIFICA(valida_senha("", cryptar("", ITERACOES_RAPIDAS)));
    VERIFICA(valida_senha("ação-çãõ-日本", cryptar("ação-çãõ-日本", ITERACOES_RAPIDAS)));
    VERIFICA(!valida_senha("acao-cao-日本", cryptar("ação-çãõ-日本", ITERACOES_RAPIDAS)));
}

TESTE(crypto_usa_custo_padrao_e_ele_funciona) {
    const std::string hash = cryptar("senha");
    VERIFICA_IGUAL(divide(hash, '$')[1], std::to_string(ITERACOES_PADRAO));
    VERIFICA(valida_senha("senha", hash));
}

TESTE(crypto_hash_adulterado_nao_valida) {
    std::string hash = cryptar("senha123", ITERACOES_RAPIDAS);
    std::string adulterado = hash;
    adulterado.back() = adulterado.back() == '0' ? '1' : '0';   // troca 1 dígito do hash
    VERIFICA(eh_hash_valido(adulterado));                        // formato continua válido...
    VERIFICA(!valida_senha("senha123", adulterado));             // ...mas a senha não confere mais
}

TESTE(crypto_hash_malformado_devolve_falso_sem_lancar_excecao) {
    const char* malformados[] = {
        "",
        "senha123",
        "$$$",
        "bcrypt$10$00$00",
        "pbkdf2-sha256$10$zz$yy",
        "pbkdf2-sha256$x$00112233445566778899aabbccddeeff$00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff",
        "pbkdf2-sha256$10$0011$0011",                                   // sal e hash curtos demais
        "pbkdf2-sha256$0$00112233445566778899aabbccddeeff$00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff",
        "pbkdf2-sha256$10$00112233445566778899aabbccddeeff"};
    for (const char* texto : malformados) {
        if (eh_hash_valido(texto)) FALHA("não deveria ser hash válido: [" << texto << "]");
        VERIFICA_NAO_LANCA(valida_senha("qualquer", texto));
        VERIFICA(!valida_senha("qualquer", texto));
    }
}

TESTE(crypto_ignora_hash_com_custo_absurdo_sem_travar) {
    // um arquivo adulterado com 999 milhões de iterações não pode congelar o programa
    const std::string sal = "00112233445566778899aabbccddeeff";
    const std::string hash = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";
    VERIFICA(!valida_senha("x", "pbkdf2-sha256$999999999$" + sal + "$" + hash));
}

TESTE(crypto_rejeita_custo_invalido_ao_gerar) {
    VERIFICA_EXCECAO(cryptar("x", 0), ErroValidacao);
    VERIFICA_EXCECAO(cryptar("x", -1), ErroValidacao);
    VERIFICA_EXCECAO(cryptar("x", ITERACOES_MAXIMAS + 1), ErroValidacao);
}
