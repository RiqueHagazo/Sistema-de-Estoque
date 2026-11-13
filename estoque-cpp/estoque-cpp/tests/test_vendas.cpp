#include <memory>

#include "erros.hpp"
#include "teste.hpp"
#include "vendas.hpp"

namespace {

std::shared_ptr<const Produto> produto_de_teste() {
    return Produto::cria(5, "leite", 10, 4.5, Data(2026, 10, 1), Data(2026, 11, 1), "", Categoria(1, "laticínios"));
}

Vendas venda_valida() {
    return Vendas(9, "maria", produto_de_teste(), 2, 9.0, Data(2026, 10, 30), Horario(14, 3, 22));
}

}  // namespace

TESTE(vendas_guarda_todos_os_dados) {
    const Vendas venda = venda_valida();
    VERIFICA_IGUAL(venda.get_id(), 9);
    VERIFICA_IGUAL(venda.get_vendedor(), std::string("maria"));
    VERIFICA_IGUAL(venda.get_produto_venda().get_nome_produto(), std::string("leite"));
    VERIFICA_IGUAL(venda.get_produto_venda().get_categoria_produto().get_nome(), std::string("laticínios"));
    VERIFICA_IGUAL(venda.get_quantidade_venda(), 2);
    VERIFICA_PROXIMO(venda.get_valor_venda(), 9.0, 1e-12);
    VERIFICA(venda.get_data_venda() == Data(2026, 10, 30));
    VERIFICA(venda.get_horario_venda() == Horario(14, 3, 22));
}

TESTE(vendas_rejeita_dados_invalidos) {
    const auto produto = produto_de_teste();
    const Data data(2026, 10, 30);
    const Horario horario(8, 0, 0);
    VERIFICA_EXCECAO(Vendas(1, "maria", nullptr, 1, 1.0, data, horario), ErroValidacao);   // sem produto
    VERIFICA_EXCECAO(Vendas(1, "", produto, 1, 1.0, data, horario), ErroValidacao);         // sem vendedor
    VERIFICA_EXCECAO(Vendas(1, "maria", produto, 0, 1.0, data, horario), ErroValidacao);    // quantidade zero
    VERIFICA_EXCECAO(Vendas(1, "maria", produto, -1, 1.0, data, horario), ErroValidacao);
    VERIFICA_EXCECAO(Vendas(1, "maria", produto, 1, -1.0, data, horario), ErroValidacao);   // valor negativo
    VERIFICA_EXCECAO(Vendas(1, "maria", produto, 1, std::nan(""), data, horario), ErroValidacao);
}

TESTE(vendas_json_usa_os_nomes_de_coluna_do_original) {
    const Json linha = venda_valida().para_json();
    VERIFICA_IGUAL(linha.campo("id").como_int(), 9);
    VERIFICA_IGUAL(linha.campo("produto_vendido").como_int(), 5);   // grava só o id do produto (a "chave estrangeira")
    VERIFICA_IGUAL(linha.campo("vendedor").como_texto(), std::string("maria"));
    VERIFICA_IGUAL(linha.campo("data_venda").como_texto(), std::string("2026-10-30"));
    VERIFICA_IGUAL(linha.campo("horario_venda").como_texto(), std::string("14:03:22"));
    VERIFICA_IGUAL(linha.campo("quantidade_venda").como_int(), 2);
    VERIFICA_PROXIMO(linha.campo("valor_venda").como_real(), 9.0, 1e-12);
}

TESTE(vendas_ida_e_volta_em_json) {
    const Vendas original = venda_valida();
    const auto lida = Vendas::de_json(Json::de_texto(original.para_json().para_texto()), produto_de_teste());
    VERIFICA(lida->para_json() == original.para_json());
}

TESTE(vendas_de_json_invalido_vira_erro_de_persistencia) {
    const char* invalidas[] = {
        R"({"id": 1})",
        R"({"id": 1, "vendedor": "m", "data_venda": "2026-02-30", "horario_venda": "10:00:00", "quantidade_venda": 1, "valor_venda": 1})",
        R"({"id": 1, "vendedor": "m", "data_venda": "2026-02-10", "horario_venda": "99:00:00", "quantidade_venda": 1, "valor_venda": 1})",
        R"({"id": 1, "vendedor": "m", "data_venda": "2026-02-10", "horario_venda": "10:00:00", "quantidade_venda": 0, "valor_venda": 1})"};
    for (const char* texto : invalidas) {
        bool lancou = false;
        try {
            Vendas::de_json(Json::de_texto(texto), produto_de_teste());
        } catch (const ErroPersistencia&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar: " << texto);
    }
}

TESTE(vendas_resumo_descreve_a_venda) {
    VERIFICA_IGUAL(venda_valida().resumo(), std::string("venda 9 de 2 x leite"));
}
