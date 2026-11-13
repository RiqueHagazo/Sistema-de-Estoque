#include "categoria.hpp"
#include "erros.hpp"
#include "teste.hpp"

TESTE(categoria_guarda_id_e_nome) {
    const Categoria categoria(3, "bebidas");
    VERIFICA_IGUAL(categoria.get_id(), 3);
    VERIFICA_IGUAL(categoria.get_nome(), std::string("bebidas"));
    VERIFICA(categoria.foi_persistida());
}

TESTE(categoria_sem_id_nao_esta_persistida) {
    const Categoria categoria(Entidade::ID_NAO_DEFINIDO, "limpeza");
    VERIFICA(!categoria.foi_persistida());
}

TESTE(categoria_remove_espacos_do_nome) {
    VERIFICA_IGUAL(Categoria(1, "  frios \t").get_nome(), std::string("frios"));
}

TESTE(categoria_rejeita_nome_em_branco) {
    VERIFICA_EXCECAO_MSG(Categoria(1, ""), ErroValidacao, "não pode ficar em branco");
    VERIFICA_EXCECAO(Categoria(1, "   \t "), ErroValidacao);
}

TESTE(categoria_limita_o_tamanho_do_nome) {
    VERIFICA_NAO_LANCA(Categoria(1, std::string(255, 'a')));
    VERIFICA_EXCECAO_MSG(Categoria(1, std::string(256, 'a')), ErroValidacao, "muito longo");
    // o limite conta caracteres, não bytes: 255 letras acentuadas ainda cabem
    std::string acentuado;
    for (int i = 0; i < 255; ++i) acentuado += "é";
    VERIFICA_NAO_LANCA(Categoria(1, acentuado));
}

TESTE(categoria_rejeita_caracteres_de_controle) {
    VERIFICA_EXCECAO(Categoria(1, "linha1\nlinha2"), ErroValidacao);
    VERIFICA_EXCECAO(Categoria(1, std::string("a\0b", 3)), ErroValidacao);
}

TESTE(categoria_rejeita_id_negativo) {
    VERIFICA_EXCECAO(Categoria(-1, "x"), ErroValidacao);
    Categoria categoria(1, "x");
    VERIFICA_EXCECAO(categoria.set_id(-5), ErroValidacao);
    categoria.set_id(9);
    VERIFICA_IGUAL(categoria.get_id(), 9);
}

TESTE(categoria_set_nome_valida) {
    Categoria categoria(1, "antigo");
    categoria.set_nome("  novo ");
    VERIFICA_IGUAL(categoria.get_nome(), std::string("novo"));
    VERIFICA_EXCECAO(categoria.set_nome(""), ErroValidacao);
    VERIFICA_IGUAL(categoria.get_nome(), std::string("novo"));   // falhou sem alterar
}

TESTE(categoria_ida_e_volta_em_json) {
    const Categoria original(7, "pães e doces");
    const Categoria lida = Categoria::de_json(Json::de_texto(original.para_json().para_texto()));
    VERIFICA_IGUAL(lida.get_id(), 7);
    VERIFICA_IGUAL(lida.get_nome(), std::string("pães e doces"));
}

TESTE(categoria_json_usa_os_nomes_de_coluna_do_original) {
    const Json linha = Categoria(2, "frios").para_json();
    VERIFICA_IGUAL(linha.campo("id").como_int(), 2);
    VERIFICA_IGUAL(linha.campo("nome").como_texto(), std::string("frios"));
}

TESTE(categoria_de_json_invalido_vira_erro_de_persistencia) {
    VERIFICA_EXCECAO(Categoria::de_json(Json::de_texto(R"({"id": 1})")), ErroPersistencia);              // sem nome
    VERIFICA_EXCECAO(Categoria::de_json(Json::de_texto(R"({"id": 1, "nome": ""})")), ErroPersistencia);   // nome em branco
    VERIFICA_EXCECAO(Categoria::de_json(Json::de_texto(R"({"id": "x", "nome": "a"})")), ErroPersistencia);
    VERIFICA_EXCECAO(Categoria::de_json(Json::de_texto(R"([1, 2])")), ErroPersistencia);
}

TESTE(categoria_pode_ser_usada_como_entidade) {
    const Categoria categoria(4, "limpeza");
    const Entidade& entidade = categoria;   // polimorfismo
    VERIFICA_IGUAL(entidade.resumo(), std::string("categoria 4 (limpeza)"));
    VERIFICA_IGUAL(entidade.para_json().campo("nome").como_texto(), std::string("limpeza"));
}
