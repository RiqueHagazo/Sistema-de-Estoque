#include "auxiliar.hpp"
#include "erros.hpp"
#include "teste.hpp"

TESTE(produto_dao_grava_e_atribui_o_id) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("laticínios");
    const auto leite = ambiente.novo_produto(*categoria, "leite", 10, 4.5, Data(2026, 11, 1));
    const auto arroz = ambiente.novo_produto(*categoria, "arroz", 20, 19.99);
    VERIFICA_IGUAL(leite->get_id(), 1);
    VERIFICA_IGUAL(arroz->get_id(), 2);
}

TESTE(produto_dao_lista_com_categoria_e_preserva_o_tipo_de_cada_produto) {
    Ambiente ambiente;
    const auto laticinios = ambiente.nova_categoria("laticínios");
    const auto graos = ambiente.nova_categoria("grãos");
    ambiente.novo_produto(*laticinios, "leite", 10, 4.5, Data(2026, 11, 1));
    ambiente.novo_produto(*graos, "arroz", 20, 19.99);

    const auto lista = ambiente.produtos.select_todos_produtos();
    VERIFICA_IGUAL(lista.size(), 2u);
    VERIFICA_IGUAL(lista[0]->get_nome_produto(), std::string("leite"));
    VERIFICA_IGUAL(lista[0]->get_categoria_produto().get_nome(), std::string("laticínios"));   // o antigo JOIN
    VERIFICA_IGUAL(lista[0]->get_tipo(), std::string("perecivel"));
    VERIFICA_IGUAL(lista[0]->validade_formatada(), std::string("01/11/2026"));
    VERIFICA_IGUAL(lista[1]->get_nome_produto(), std::string("arroz"));
    VERIFICA_IGUAL(lista[1]->get_categoria_produto().get_nome(), std::string("grãos"));
    VERIFICA_IGUAL(lista[1]->get_tipo(), std::string("nao perecivel"));
    VERIFICA_IGUAL(lista[1]->validade_formatada(), std::string("-"));
    VERIFICA(lista[1]->get_valor_produto() == 19.99);
}

TESTE(produto_dao_rejeita_categoria_inexistente) {
    Ambiente ambiente;
    const Categoria fantasma(42, "nao gravada");
    auto produto = Produto::cria(0, "x", 1, 1.0, Data(2026, 10, 1), std::nullopt, "", fantasma);
    VERIFICA_EXCECAO_MSG(ambiente.produtos.insere_produto(*produto), ErroIntegridade, "categoria inválida");
    VERIFICA(ambiente.produtos.select_todos_produtos().empty());
    VERIFICA(!produto->foi_persistida());
}

TESTE(produto_dao_nao_grava_duas_vezes_o_mesmo_objeto) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    const auto produto = ambiente.novo_produto(*categoria, "x", 1, 1.0);
    VERIFICA_EXCECAO(ambiente.produtos.insere_produto(*produto), ErroRegraNegocio);
    VERIFICA_IGUAL(ambiente.produtos.select_todos_produtos().size(), 1u);
}

TESTE(produto_dao_busca_por_nome_ignora_caixa_acentos_e_espacos) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    ambiente.novo_produto(*categoria, "Feijão", 5, 8.0);
    ambiente.novo_produto(*categoria, "arroz", 5, 8.0);
    ambiente.novo_produto(*categoria, "feijao", 7, 9.0);   // outro produto de mesmo nome (sem acento)

    VERIFICA_IGUAL(ambiente.produtos.busca_produto("FEIJAO").size(), 2u);
    VERIFICA_IGUAL(ambiente.produtos.busca_produto("  arroz ").size(), 1u);
    VERIFICA(ambiente.produtos.busca_produto("macarrão").empty());
    VERIFICA(ambiente.produtos.busca_produto("feij").empty());   // a busca é por nome exato, como no original
}

TESTE(produto_dao_busca_por_id) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    const auto produto = ambiente.novo_produto(*categoria, "leite", 3, 4.0);
    const auto achado = ambiente.produtos.busca_produto_id(produto->get_id());
    VERIFICA_IGUAL(achado->get_nome_produto(), std::string("leite"));
    VERIFICA_IGUAL(achado->get_categoria_produto().get_nome(), std::string("c"));
    VERIFICA_EXCECAO_MSG(ambiente.produtos.busca_produto_id(99), ErroNaoEncontrado, "produto não encontrado");
}

TESTE(produto_dao_exclui_produto_sem_vendas) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    const auto a = ambiente.novo_produto(*categoria, "a", 1, 1.0);
    ambiente.novo_produto(*categoria, "b", 1, 1.0);
    ambiente.produtos.excluir_produto(a->get_id());
    const auto restantes = ambiente.produtos.select_todos_produtos();
    VERIFICA_IGUAL(restantes.size(), 1u);
    VERIFICA_IGUAL(restantes[0]->get_nome_produto(), std::string("b"));
}

TESTE(produto_dao_exclusao_de_produto_inexistente_lanca_erro) {
    Ambiente ambiente;
    VERIFICA_EXCECAO(ambiente.produtos.excluir_produto(7), ErroNaoEncontrado);
}

TESTE(produto_dao_nao_exclui_produto_que_tem_vendas) {
    // no original, a chave estrangeira de `vendas` impedia isso (e derrubava a página); agora é um erro tratável
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    const auto produto = ambiente.novo_produto(*categoria, "leite", 10, 4.0);
    ambiente.nova_venda(produto, "maria", 2, Data(2026, 10, 30));
    VERIFICA_EXCECAO_MSG(ambiente.produtos.excluir_produto(produto->get_id()), ErroIntegridade, "vendas registradas");
    VERIFICA_IGUAL(ambiente.produtos.select_todos_produtos().size(), 1u);
}

TESTE(produto_dao_atualiza_a_quantidade) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    const auto produto = ambiente.novo_produto(*categoria, "leite", 10, 4.0);
    ambiente.produtos.atualiza_quantidade(produto->get_id(), 3);
    VERIFICA_IGUAL(ambiente.produtos.busca_produto_id(produto->get_id())->get_quantidade_produto(), 3);
    ambiente.produtos.atualiza_quantidade(produto->get_id(), 0);
    VERIFICA_IGUAL(ambiente.produtos.busca_produto_id(produto->get_id())->get_quantidade_produto(), 0);
}

TESTE(produto_dao_atualiza_quantidade_valida_os_dados) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    const auto produto = ambiente.novo_produto(*categoria, "leite", 10, 4.0);
    VERIFICA_EXCECAO(ambiente.produtos.atualiza_quantidade(produto->get_id(), -1), ErroValidacao);
    VERIFICA_EXCECAO(ambiente.produtos.atualiza_quantidade(produto->get_id(), Produto::QUANTIDADE_MAXIMA + 1), ErroValidacao);
    VERIFICA_EXCECAO(ambiente.produtos.atualiza_quantidade(99, 1), ErroNaoEncontrado);
    VERIFICA_IGUAL(ambiente.produtos.busca_produto_id(produto->get_id())->get_quantidade_produto(), 10);
}

TESTE(produto_dao_atualizar_quantidade_nao_mexe_nos_outros_campos) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("c");
    const auto produto = ambiente.novo_produto(*categoria, "leite", 10, 4.5, Data(2026, 11, 1));
    const Json antes = produto->para_json();
    ambiente.produtos.atualiza_quantidade(produto->get_id(), 6);
    Json esperado = antes;
    esperado.define("quantidade", 6);
    VERIFICA(ambiente.produtos.busca_produto_id(produto->get_id())->para_json() == esperado);
}

TESTE(produto_dao_detecta_produto_com_categoria_inexistente_no_arquivo) {
    Ambiente ambiente;
    escreve_arquivo(ambiente.pasta.arquivo("produto.json"),
                    R"({"proximo_id": 2, "linhas": [{"id": 1, "nome_produto": "x", "quantidade": 1, "valor": 1.0,
                        "chegada": "2026-10-01", "validade": null, "descricao": "", "id_categoria": 9}]})");
    VERIFICA_EXCECAO_MSG(ambiente.produtos.select_todos_produtos(), ErroIntegridade, "categoria inexistente");
}

TESTE(produto_dao_linha_invalida_vira_erro_de_persistencia) {
    Ambiente ambiente;
    ambiente.nova_categoria("c");
    escreve_arquivo(ambiente.pasta.arquivo("produto.json"),
                    R"({"proximo_id": 2, "linhas": [{"id": 1, "nome_produto": "x", "quantidade": -4, "valor": 1.0,
                        "chegada": "2026-10-01", "validade": null, "descricao": "", "id_categoria": 1}]})");
    VERIFICA_EXCECAO(ambiente.produtos.select_todos_produtos(), ErroPersistencia);
}
