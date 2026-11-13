#include <cmath>

#include "erros.hpp"
#include "teste.hpp"
#include "texto.hpp"

TESTE(texto_remove_espacos_das_pontas) {
    VERIFICA_IGUAL(remove_espacos("  oi mundo \t\n"), std::string("oi mundo"));
    VERIFICA_IGUAL(remove_espacos("   "), std::string(""));
    VERIFICA_IGUAL(remove_espacos(""), std::string(""));
    VERIFICA_IGUAL(remove_espacos("a"), std::string("a"));
}

TESTE(texto_normaliza_ignora_caixa_e_acentos) {
    VERIFICA_IGUAL(normaliza("Água"), std::string("agua"));
    VERIFICA_IGUAL(normaliza("AÇÚCAR"), std::string("acucar"));
    VERIFICA_IGUAL(normaliza("São João"), std::string("sao joao"));
    VERIFICA_IGUAL(normaliza("Pão de Açúcar"), std::string("pao de acucar"));
    VERIFICA_IGUAL(normaliza("Ñandú"), std::string("nandu"));
    VERIFICA_IGUAL(normaliza("ABC xyz 123"), std::string("abc xyz 123"));
}

TESTE(texto_normaliza_preserva_simbolos_fora_do_alfabeto_latino) {
    VERIFICA_IGUAL(normaliza("2×3÷1"), std::string("2×3÷1"));
    VERIFICA_IGUAL(normaliza("日本"), std::string("日本"));
    VERIFICA_IGUAL(normaliza("\xC3"), std::string("\xC3"));   // byte solto não derruba a função
}

TESTE(texto_iguais_normalizados) {
    VERIFICA(iguais_normalizados("Arroz", "arroz"));
    VERIFICA(iguais_normalizados("feijão", "FEIJAO"));
    VERIFICA(!iguais_normalizados("arroz", "arroz "));
}

TESTE(texto_tamanho_utf8_conta_caracteres) {
    VERIFICA_IGUAL(tamanho_utf8("abc"), 3u);
    VERIFICA_IGUAL(tamanho_utf8("açúcar"), 6u);
    VERIFICA_IGUAL(tamanho_utf8(""), 0u);
}

TESTE(texto_limita_texto) {
    VERIFICA_IGUAL(limita_texto("curto", 10), std::string("curto"));
    VERIFICA_IGUAL(limita_texto("abcdefghij", 8), std::string("abcde..."));
    VERIFICA_IGUAL(limita_texto("açúcar refinado", 8), std::string("açúca..."));
    VERIFICA_IGUAL(limita_texto("abcdef", 3), std::string("abc"));
    VERIFICA_IGUAL(limita_texto("abcdef", 6), std::string("abcdef"));
}

TESTE(texto_preenche_considera_caracteres_e_nao_bytes) {
    VERIFICA_IGUAL(preenche("ab", 5), std::string("ab   "));
    VERIFICA_IGUAL(preenche("é", 3), std::string("é  "));
    VERIFICA_IGUAL(preenche("abcdef", 3), std::string("abcdef"));
}

TESTE(texto_formata_valor_igual_ao_filtro_original) {
    VERIFICA_IGUAL(formata_valor(0), std::string("0.00"));
    VERIFICA_IGUAL(formata_valor(5), std::string("5.00"));
    VERIFICA_IGUAL(formata_valor(1234.5), std::string("1,234.50"));
    VERIFICA_IGUAL(formata_valor(1234567.891), std::string("1,234,567.89"));
    VERIFICA_IGUAL(formata_valor(999.999), std::string("1,000.00"));
    VERIFICA_IGUAL(formata_valor(-1234.5), std::string("-1,234.50"));
    VERIFICA_IGUAL(formata_valor(-0.001), std::string("0.00"));
    VERIFICA_IGUAL(formata_valor(100), std::string("100.00"));
}

TESTE(texto_formata_valor_nao_finito_devolve_vazio) {
    VERIFICA_IGUAL(formata_valor(std::nan("")), std::string(""));
    VERIFICA_IGUAL(formata_valor(INFINITY), std::string(""));
}

TESTE(texto_converte_inteiro_valido) {
    VERIFICA_IGUAL(converte_inteiro("42", "quantidade"), 42);
    VERIFICA_IGUAL(converte_inteiro("  -7 ", "quantidade"), -7);
    VERIFICA_IGUAL(converte_inteiro("+3", "quantidade"), 3);
    VERIFICA_IGUAL(converte_inteiro("2147483647", "quantidade"), 2147483647);
}

TESTE(texto_converte_inteiro_invalido_lanca_erro_de_validacao) {
    const char* invalidos[] = {"", "  ", "abc", "4.5", "1e3", "-", "+", "12a", "1 2", "2147483648", "99999999999999999999"};
    for (const char* texto : invalidos) {
        bool lancou = false;
        try {
            converte_inteiro(texto, "quantidade");
        } catch (const ErroValidacao&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar: [" << texto << "]");
    }
    VERIFICA_EXCECAO_MSG(converte_inteiro("x", "quantidade"), ErroValidacao, "'quantidade'");
}

TESTE(texto_converte_real_aceita_virgula_e_ponto) {
    VERIFICA_PROXIMO(converte_real("12,50", "valor"), 12.5, 1e-12);
    VERIFICA_PROXIMO(converte_real("12.5", "valor"), 12.5, 1e-12);
    VERIFICA_PROXIMO(converte_real(" -3 ", "valor"), -3.0, 1e-12);
    VERIFICA_PROXIMO(converte_real(".5", "valor"), 0.5, 1e-12);
    VERIFICA_PROXIMO(converte_real("7.", "valor"), 7.0, 1e-12);
    VERIFICA_PROXIMO(converte_real("0", "valor"), 0.0, 1e-12);
}

TESTE(texto_converte_real_invalido_lanca_erro_de_validacao) {
    const char* invalidos[] = {"", " ", "abc", "1,2,3", "1.2.3", "-", ".", "1e5", "12R$", "nan", "inf", "--1"};
    for (const char* texto : invalidos) {
        bool lancou = false;
        try {
            converte_real(texto, "valor");
        } catch (const ErroValidacao&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar: [" << texto << "]");
    }
}

TESTE(texto_divide_mantem_partes_vazias) {
    const auto partes = divide("a$b$$c", '$');
    VERIFICA_IGUAL(partes.size(), 4u);
    VERIFICA_IGUAL(partes[2], std::string(""));
    VERIFICA_IGUAL(divide("abc", '$').size(), 1u);
    VERIFICA_IGUAL(divide("a$", '$').size(), 2u);
    VERIFICA_IGUAL(divide("", '$').size(), 0u);
}

TESTE(texto_converte_real_rejeita_numero_grande_demais) {
    VERIFICA_EXCECAO(converte_real(std::string(400, '9'), "valor"), ErroValidacao);
}
