#include <cmath>
#include <limits>

#include "erros.hpp"
#include "json.hpp"
#include "teste.hpp"

// ---------------------------------------------------------------- leitura

TESTE(json_le_literais) {
    VERIFICA(Json::de_texto("null").eh_nulo());
    VERIFICA_IGUAL(Json::de_texto("true").como_booleano(), true);
    VERIFICA_IGUAL(Json::de_texto("false").como_booleano(), false);
    VERIFICA_IGUAL(Json::de_texto("  \n\t 7 \r\n").como_inteiro(), 7LL);
}

TESTE(json_distingue_inteiro_de_real) {
    VERIFICA(Json::de_texto("42").get_tipo() == Json::Tipo::INTEIRO);
    VERIFICA(Json::de_texto("42.0").get_tipo() == Json::Tipo::REAL);
    VERIFICA(Json::de_texto("4e2").get_tipo() == Json::Tipo::REAL);
    VERIFICA_PROXIMO(Json::de_texto("-12.5e-1").como_real(), -1.25, 1e-12);
    VERIFICA_PROXIMO(Json::de_texto("3").como_real(), 3.0, 1e-12);   // inteiro também serve como real
}

TESTE(json_inteiro_gigante_vira_real) {
    const Json valor = Json::de_texto("99999999999999999999");
    VERIFICA(valor.get_tipo() == Json::Tipo::REAL);
}

TESTE(json_le_textos_com_escapes) {
    VERIFICA_IGUAL(Json::de_texto("\"a\\\"b\\\\c\\/d\"").como_texto(), std::string("a\"b\\c/d"));
    VERIFICA_IGUAL(Json::de_texto("\"l1\\nl2\\tx\\r\\b\\f\"").como_texto(), std::string("l1\nl2\tx\r\b\f"));
}

TESTE(json_le_unicode_e_pares_substitutos) {
    VERIFICA_IGUAL(Json::de_texto("\"caf\\u00e9\"").como_texto(), std::string("café"));
    // U+1F600 = par substituto D83D DE00 -> 4 bytes em UTF-8
    VERIFICA_IGUAL(Json::de_texto("\"\\ud83d\\ude00\"").como_texto(), std::string("\xF0\x9F\x98\x80"));
    // texto UTF-8 "cru" passa direto
    VERIFICA_IGUAL(Json::de_texto("\"ação\"").como_texto(), std::string("ação"));
}

TESTE(json_le_objetos_e_arrays_aninhados) {
    const Json doc = Json::de_texto(R"({"a": [1, 2, {"b": null}], "c": {"d": "x"}, "e": []})");
    VERIFICA(doc.eh_objeto());
    VERIFICA_IGUAL(doc.tamanho(), 3u);
    VERIFICA_IGUAL(doc.campo("a").tamanho(), 3u);
    VERIFICA_IGUAL(doc.campo("a").item(1).como_int(), 2);
    VERIFICA(doc.campo("a").item(2).campo("b").eh_nulo());
    VERIFICA_IGUAL(doc.campo("c").campo("d").como_texto(), std::string("x"));
    VERIFICA_IGUAL(doc.campo("e").tamanho(), 0u);
}

TESTE(json_preserva_ordem_dos_campos) {
    const Json doc = Json::de_texto(R"({"z": 1, "a": 2, "m": 3})");
    VERIFICA_IGUAL(doc.chaves()[0], std::string("z"));
    VERIFICA_IGUAL(doc.chaves()[1], std::string("a"));
    VERIFICA_IGUAL(doc.chaves()[2], std::string("m"));
}

// ---------------------------------------------------------------- erros de leitura

TESTE(json_rejeita_documentos_malformados) {
    const char* invalidos[] = {
        "", "   ", "{", "}", "[1,]", "[1 2]", "{\"a\"}", "{\"a\":}", "{\"a\":1,}", "{a:1}", "tru", "nul",
        "\"abc", "01", "1.", "-", "+1", "1e", ".5", "{\"a\":1} x", "[1]]", "'texto'",
        "\"\\x\"", "\"\\u12\"", "\"\\uzzzz\"", "\"\\ud83d\"", "\"\\ude00\"", "\"quebra\nde linha\"",
        "{\"a\":1 \"b\":2}", "\"abc\\", "\"\\ud83d\\u0041\"", "1e999", "-1e999"};
    for (const char* texto : invalidos) {
        bool lancou = false;
        try {
            Json::de_texto(texto);
        } catch (const ErroJson&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar: [" << texto << "]");
    }
}

TESTE(json_erro_informa_linha_e_coluna) {
    VERIFICA_EXCECAO_MSG(Json::de_texto("{\n  \"a\": 1,\n  \"b\": ?\n}"), ErroJson, "linha 3");
}

TESTE(json_limita_profundidade_de_aninhamento) {
    const std::string fundo(200, '[');
    VERIFICA_EXCECAO_MSG(Json::de_texto(fundo), ErroJson, "aninhado demais");
}

TESTE(json_erro_json_eh_erro_de_persistencia_e_de_estoque) {
    VERIFICA_EXCECAO(Json::de_texto("{"), ErroPersistencia);
    VERIFICA_EXCECAO(Json::de_texto("{"), ErroEstoque);
}

// ---------------------------------------------------------------- acesso tipado

TESTE(json_acesso_com_tipo_errado_lanca_erro) {
    const Json numero(5);
    VERIFICA_EXCECAO(numero.como_texto(), ErroJson);
    VERIFICA_EXCECAO(numero.como_booleano(), ErroJson);
    VERIFICA_EXCECAO(numero.item(0), ErroJson);
    VERIFICA_EXCECAO(numero.campo("x"), ErroJson);
    VERIFICA_EXCECAO(numero.tamanho(), ErroJson);
    VERIFICA_EXCECAO(Json("txt").como_inteiro(), ErroJson);
    VERIFICA_EXCECAO(Json("txt").como_real(), ErroJson);
}

TESTE(json_campo_ausente_e_indice_fora_lancam_erro) {
    const Json doc = Json::de_texto(R"({"a": [1]})");
    VERIFICA_EXCECAO_MSG(doc.campo("nao_existe"), ErroJson, "campo ausente: nao_existe");
    VERIFICA_EXCECAO(doc.campo("a").item(1), ErroJson);
    VERIFICA(doc.tem_campo("a"));
    VERIFICA(!doc.tem_campo("b"));
    VERIFICA(!Json(3).tem_campo("a"));
}

TESTE(json_como_int_valida_faixa) {
    VERIFICA_IGUAL(Json(2147483647LL).como_int(), 2147483647);
    VERIFICA_EXCECAO(Json(2147483648LL).como_int(), ErroJson);
    VERIFICA_EXCECAO(Json(-2147483649LL).como_int(), ErroJson);
}

TESTE(json_define_substitui_campo_existente_sem_mudar_ordem) {
    Json doc = Json::objeto();
    doc.define("a", 1).define("b", 2).define("a", 99);
    VERIFICA_IGUAL(doc.tamanho(), 2u);
    VERIFICA_IGUAL(doc.campo("a").como_int(), 99);
    VERIFICA_IGUAL(doc.chaves()[0], std::string("a"));
}

TESTE(json_adiciona_so_funciona_em_array) {
    Json objeto = Json::objeto();
    VERIFICA_EXCECAO(objeto.adiciona(1), ErroJson);
    Json array = Json::array();
    VERIFICA_EXCECAO(array.define("x", 1), ErroJson);
    array.adiciona(1);
    array.adiciona("dois");
    VERIFICA_IGUAL(array.tamanho(), 2u);
    VERIFICA_IGUAL(array.itens().size(), 2u);
}

// ---------------------------------------------------------------- escrita

TESTE(json_escreve_compacto) {
    Json doc = Json::objeto();
    doc.define("a", 1).define("b", Json::array()).define("c", "x\"y").define("d", Json());
    doc.define("e", true);
    VERIFICA_IGUAL(doc.para_texto(-1), std::string(R"({"a":1,"b":[],"c":"x\"y","d":null,"e":true})"));
}

TESTE(json_escreve_arrays_simples_em_uma_linha_e_objetos_em_varias) {
    Json linha1 = Json::objeto();
    linha1.define("id", 1).define("nome", "leite");
    Json linhas = Json::array();
    linhas.adiciona(linha1);
    Json doc = Json::objeto();
    doc.define("proximo_id", 2).define("linhas", linhas);
    const std::string esperado =
        "{\n"
        "  \"proximo_id\": 2,\n"
        "  \"linhas\": [\n"
        "    {\"id\": 1, \"nome\": \"leite\"}\n"
        "  ]\n"
        "}";
    VERIFICA_IGUAL(doc.para_texto(2), esperado);
}

TESTE(json_escapa_caracteres_de_controle) {
    const Json texto(std::string("a\nb\tc\x01"));
    VERIFICA_IGUAL(texto.para_texto(-1), std::string("\"a\\nb\\tc\\u0001\""));
}

TESTE(json_ida_e_volta_preserva_o_documento) {
    Json doc = Json::objeto();
    doc.define("texto", "ação \"citada\" \\ barra\nlinha").define("inteiro", -15).define("real", 19.99);
    doc.define("nulo", Json()).define("verdade", false);
    Json lista = Json::array();
    lista.adiciona(1).adiciona(2.5).adiciona("três");
    doc.define("lista", lista);
    for (int indentacao : {-1, 0, 2, 4}) {
        VERIFICA(Json::de_texto(doc.para_texto(indentacao)) == doc);
    }
}

TESTE(json_numeros_reais_fazem_ida_e_volta_exata) {
    const double valores[] = {0.1, 19.99, 59.97, 1e-7, 123456789.123456789, 1.0 / 3.0, -0.5, 1e21, 5.0, 0.0};
    for (double valor : valores) {
        const Json lido = Json::de_texto(Json(valor).para_texto(-1));
        VERIFICA(lido.get_tipo() == Json::Tipo::REAL);
        VERIFICA(lido.como_real() == valor);
    }
    VERIFICA_IGUAL(Json(19.99).para_texto(-1), std::string("19.99"));
    VERIFICA_IGUAL(Json(5.0).para_texto(-1), std::string("5.0"));
}

TESTE(json_nao_grava_numeros_nao_finitos) {
    VERIFICA_EXCECAO(Json(std::nan("")).para_texto(), ErroJson);
    VERIFICA_EXCECAO(Json(std::numeric_limits<double>::infinity()).para_texto(), ErroJson);
}

TESTE(json_igualdade_profunda) {
    VERIFICA(Json::de_texto("[1,{\"a\":2}]") == Json::de_texto("[ 1 , { \"a\" : 2 } ]"));
    VERIFICA(Json::de_texto("[1,2]") != Json::de_texto("[1,3]"));
    VERIFICA(Json(1) != Json(1.0));
    VERIFICA(Json("a") != Json());
}

TESTE(json_le_caracteres_escapados_de_1_2_e_3_bytes) {
    VERIFICA_IGUAL(Json::de_texto("\"\\u0041\"").como_texto(), std::string("A"));           // 1 byte
    VERIFICA_IGUAL(Json::de_texto("\"\\u00e9\"").como_texto(), std::string("é"));           // 2 bytes
    VERIFICA_IGUAL(Json::de_texto("\"\\u20ac\"").como_texto(), std::string("\xE2\x82\xAC"));   // 3 bytes (euro)
}

TESTE(json_escreve_todos_os_escapes_curtos) {
    const Json texto(std::string("\b\f\n\r\t\"\\"));
    VERIFICA_IGUAL(texto.para_texto(-1), std::string("\"\\b\\f\\n\\r\\t\\\"\\\\\""));
    VERIFICA(Json::de_texto(texto.para_texto(-1)).como_texto() == texto.como_texto());
}

TESTE(json_consultas_de_tipo) {
    VERIFICA(Json("x").eh_texto());
    VERIFICA(!Json(1).eh_texto());
    VERIFICA(Json(1).eh_numero());
    VERIFICA(Json(1.5).eh_numero());
    VERIFICA(!Json("1").eh_numero());
    VERIFICA(Json().eh_nulo());
    VERIFICA(Json::array().eh_array());
    VERIFICA(Json::objeto().eh_objeto());
    VERIFICA(Json(nullptr).eh_nulo());
    VERIFICA_EXCECAO(Json(5).itens(), ErroJson);
}

TESTE(json_documento_grande_faz_ida_e_volta) {
    Json lista = Json::array();
    for (int i = 0; i < 2000; ++i) {
        Json item = Json::objeto();
        item.define("id", i).define("nome", "item " + std::to_string(i)).define("preco", i * 0.25);
        lista.adiciona(item);
    }
    VERIFICA(Json::de_texto(lista.para_texto(2)) == lista);
}
