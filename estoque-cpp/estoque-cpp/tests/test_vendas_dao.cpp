#include "auxiliar.hpp"
#include "erros.hpp"
#include "teste.hpp"

namespace {
struct Cenario {
    Ambiente ambiente;
    std::shared_ptr<Categoria> categoria;
    std::shared_ptr<Produto> leite;
    std::shared_ptr<Produto> arroz;

    Cenario() {
        categoria = ambiente.nova_categoria("mercearia");
        leite = ambiente.novo_produto(*categoria, "leite", 100, 4.5, Data(2026, 12, 1));
        arroz = ambiente.novo_produto(*categoria, "arroz", 100, 20.0);
    }
};
}  // namespace

TESTE(vendas_dao_grava_e_atribui_o_id) {
    Cenario c;
    const auto primeira = c.ambiente.nova_venda(c.leite, "maria", 2, Data(2026, 10, 30));
    const auto segunda = c.ambiente.nova_venda(c.arroz, "joao", 1, Data(2026, 10, 30));
    VERIFICA_IGUAL(primeira->get_id(), 1);
    VERIFICA_IGUAL(segunda->get_id(), 2);
}

TESTE(vendas_dao_lista_com_produto_e_categoria_o_antigo_join) {
    Cenario c;
    c.ambiente.nova_venda(c.leite, "maria", 2, Data(2026, 10, 30), Horario(14, 3, 22));
    const auto registros = c.ambiente.vendas.select_registros();
    VERIFICA_IGUAL(registros.size(), 1u);
    const Vendas& venda = *registros[0];
    VERIFICA_IGUAL(venda.get_produto_venda().get_nome_produto(), std::string("leite"));
    VERIFICA_IGUAL(venda.get_produto_venda().get_categoria_produto().get_nome(), std::string("mercearia"));
    VERIFICA_IGUAL(venda.get_vendedor(), std::string("maria"));
    VERIFICA_IGUAL(venda.get_quantidade_venda(), 2);
    VERIFICA(venda.get_valor_venda() == 9.0);
    VERIFICA(venda.get_data_venda() == Data(2026, 10, 30));
    VERIFICA(venda.get_horario_venda() == Horario(14, 3, 22));
}

TESTE(vendas_dao_sem_vendas_devolve_lista_vazia) {
    Cenario c;
    VERIFICA(c.ambiente.vendas.select_registros().empty());
}

TESTE(vendas_dao_rejeita_produto_inexistente) {
    Cenario c;
    const Categoria categoria(1, "x");
    const auto fantasma = Produto::cria(77, "fantasma", 1, 1.0, Data(2026, 10, 1), std::nullopt, "", categoria);
    Vendas venda(Entidade::ID_NAO_DEFINIDO, "maria", fantasma, 1, 1.0, Data(2026, 10, 30), Horario(10, 0, 0));
    VERIFICA_EXCECAO_MSG(c.ambiente.vendas.registra_vendas(venda), ErroIntegridade, "produto da venda não existe");
    VERIFICA(c.ambiente.vendas.select_registros().empty());
}

TESTE(vendas_dao_nao_registra_duas_vezes_o_mesmo_objeto) {
    Cenario c;
    const auto venda = c.ambiente.nova_venda(c.leite, "maria", 1, Data(2026, 10, 30));
    VERIFICA_EXCECAO(c.ambiente.vendas.registra_vendas(*venda), ErroRegraNegocio);
    VERIFICA_IGUAL(c.ambiente.vendas.select_registros().size(), 1u);
}

TESTE(vendas_dao_pesquisa_por_periodo_com_limites_inclusivos) {
    Cenario c;
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 9));    // fora (antes)
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 10));   // dentro (limite inicial)
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 15));   // dentro
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 20));   // dentro (limite final)
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 21));   // fora (depois)
    const auto periodo = c.ambiente.vendas.pesquisar_periodo(Data(2026, 10, 10), Data(2026, 10, 20));
    VERIFICA_IGUAL(periodo.size(), 3u);
    VERIFICA(periodo[0]->get_data_venda() == Data(2026, 10, 10));
    VERIFICA(periodo[2]->get_data_venda() == Data(2026, 10, 20));
}

TESTE(vendas_dao_periodo_de_um_unico_dia) {
    Cenario c;
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 10));
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 11));
    VERIFICA_IGUAL(c.ambiente.vendas.pesquisar_periodo(Data(2026, 10, 10), Data(2026, 10, 10)).size(), 1u);
}

TESTE(vendas_dao_rejeita_periodo_invertido) {
    Cenario c;
    VERIFICA_EXCECAO_MSG(c.ambiente.vendas.pesquisar_periodo(Data(2026, 10, 20), Data(2026, 10, 10)), ErroValidacao,
                         "período inválido");
    VERIFICA_EXCECAO(c.ambiente.vendas.valor_por_periodo(Data(2026, 10, 20), Data(2026, 10, 10)), ErroValidacao);
}

TESTE(vendas_dao_soma_o_valor_do_periodo) {
    Cenario c;
    c.ambiente.nova_venda(c.leite, "u", 2, Data(2026, 10, 10));    //  9.00
    c.ambiente.nova_venda(c.arroz, "u", 1, Data(2026, 10, 11));    // 20.00
    c.ambiente.nova_venda(c.arroz, "u", 5, Data(2026, 11, 30));    // fora do período
    VERIFICA_PROXIMO(c.ambiente.vendas.valor_por_periodo(Data(2026, 10, 1), Data(2026, 10, 31)), 29.0, 1e-9);
    VERIFICA_PROXIMO(c.ambiente.vendas.valor_por_periodo(Data(2026, 1, 1), Data(2026, 12, 31)), 129.0, 1e-9);
}

TESTE(vendas_dao_valor_de_periodo_sem_vendas_e_zero) {
    Cenario c;
    c.ambiente.nova_venda(c.leite, "u", 1, Data(2026, 10, 10));
    VERIFICA(c.ambiente.vendas.valor_por_periodo(Data(2025, 1, 1), Data(2025, 1, 31)) == 0.0);
}

TESTE(vendas_dao_soma_sem_erro_de_ponto_flutuante) {
    Cenario c;
    const auto barato = c.ambiente.novo_produto(*c.categoria, "bala", 100, 0.1);
    const auto outro = c.ambiente.novo_produto(*c.categoria, "chiclete", 100, 0.2);
    c.ambiente.nova_venda(barato, "u", 1, Data(2026, 10, 10));
    c.ambiente.nova_venda(outro, "u", 1, Data(2026, 10, 10));
    VERIFICA(c.ambiente.vendas.valor_por_periodo(Data(2026, 10, 1), Data(2026, 10, 31)) == 0.3);   // 0.1 + 0.2 != 0.3 sem arredondar
}

TESTE(vendas_dao_detecta_venda_de_produto_inexistente_no_arquivo) {
    Cenario c;
    escreve_arquivo(c.ambiente.pasta.arquivo("vendas.json"),
                    R"({"proximo_id": 2, "linhas": [{"id": 1, "produto_vendido": 99, "vendedor": "m",
                        "data_venda": "2026-10-10", "horario_venda": "10:00:00", "quantidade_venda": 1, "valor_venda": 1.0}]})");
    VERIFICA_EXCECAO_MSG(c.ambiente.vendas.select_registros(), ErroIntegridade, "produto inexistente");
}
