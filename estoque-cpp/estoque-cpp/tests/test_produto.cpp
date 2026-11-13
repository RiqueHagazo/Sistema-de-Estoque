#include <memory>
#include <vector>

#include "erros.hpp"
#include "produto.hpp"
#include "teste.hpp"

namespace {

const Categoria CATEGORIA(1, "laticínios");
const Data CHEGADA(2026, 10, 1);

std::shared_ptr<Produto> leite(int quantidade = 10) {
    return Produto::cria(1, "leite", quantidade, 4.5, CHEGADA, Data(2026, 11, 1), "integral 1L", CATEGORIA);
}

std::shared_ptr<Produto> arroz(int quantidade = 20) {
    return Produto::cria(2, "arroz", quantidade, 19.99, CHEGADA, std::nullopt, "", CATEGORIA);
}

}  // namespace

// ---------------------------------------------------------------- criação e invariantes

TESTE(produto_guarda_todos_os_dados) {
    const auto produto = leite();
    VERIFICA_IGUAL(produto->get_id(), 1);
    VERIFICA_IGUAL(produto->get_nome_produto(), std::string("leite"));
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 10);
    VERIFICA_PROXIMO(produto->get_valor_produto(), 4.5, 1e-12);
    VERIFICA(produto->get_chegada_produto() == CHEGADA);
    VERIFICA_IGUAL(produto->get_descricao_produto(), std::string("integral 1L"));
    VERIFICA_IGUAL(produto->get_categoria_produto().get_nome(), std::string("laticínios"));
}

TESTE(produto_rejeita_dados_invalidos) {
    auto cria = [](const std::string& nome, int quantidade, double valor) {
        return Produto::cria(1, nome, quantidade, valor, CHEGADA, std::nullopt, "", CATEGORIA);
    };
    VERIFICA_EXCECAO(cria("", 1, 1.0), ErroValidacao);
    VERIFICA_EXCECAO(cria("  ", 1, 1.0), ErroValidacao);
    VERIFICA_EXCECAO(cria("x", -1, 1.0), ErroValidacao);
    VERIFICA_EXCECAO(cria("x", Produto::QUANTIDADE_MAXIMA + 1, 1.0), ErroValidacao);
    VERIFICA_EXCECAO(cria("x", 1, -0.01), ErroValidacao);
    VERIFICA_EXCECAO(cria("x", 1, Produto::VALOR_MAXIMO * 2), ErroValidacao);
    VERIFICA_EXCECAO(cria("x", 1, std::nan("")), ErroValidacao);
    VERIFICA_EXCECAO(cria("x", 1, INFINITY), ErroValidacao);
    VERIFICA_NAO_LANCA(cria("x", 0, 0.0));   // estoque zerado e produto gratuito são válidos
}

TESTE(produto_descricao_e_opcional_mas_tem_limite) {
    VERIFICA_NAO_LANCA(Produto::cria(1, "x", 1, 1.0, CHEGADA, std::nullopt, "", CATEGORIA));
    VERIFICA_EXCECAO(Produto::cria(1, "x", 1, 1.0, CHEGADA, std::nullopt, std::string(256, 'd'), CATEGORIA), ErroValidacao);
}

TESTE(produto_setters_validam_e_nao_alteram_em_caso_de_erro) {
    const auto produto = leite();
    produto->set_nome_produto("  leite integral ");
    produto->set_quantidade_produto(3);
    produto->set_valor_produto(5.0);
    produto->set_descricao_produto("caixa");
    VERIFICA_IGUAL(produto->get_nome_produto(), std::string("leite integral"));
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 3);
    VERIFICA_PROXIMO(produto->get_valor_produto(), 5.0, 1e-12);
    VERIFICA_IGUAL(produto->get_descricao_produto(), std::string("caixa"));

    VERIFICA_EXCECAO(produto->set_nome_produto(""), ErroValidacao);
    VERIFICA_EXCECAO(produto->set_quantidade_produto(-1), ErroValidacao);
    VERIFICA_EXCECAO(produto->set_valor_produto(-1.0), ErroValidacao);
    VERIFICA_EXCECAO(produto->set_descricao_produto(std::string(300, 'x')), ErroValidacao);
    VERIFICA_IGUAL(produto->get_nome_produto(), std::string("leite integral"));
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 3);
}

// ---------------------------------------------------------------- regras de estoque

TESTE(produto_baixa_estoque_desconta_a_quantidade) {
    const auto produto = leite(10);
    produto->baixa_estoque(3);
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 7);
    produto->baixa_estoque(7);   // pode zerar o estoque
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 0);
}

TESTE(produto_baixa_estoque_rejeita_quantidade_maior_que_o_estoque) {
    const auto produto = leite(5);
    VERIFICA_EXCECAO_MSG(produto->baixa_estoque(6), ErroEstoqueInsuficiente, "produtos insuficientes ou igual a zero");
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 5);   // estoque intacto
}

TESTE(produto_baixa_estoque_rejeita_zero_e_negativos) {
    const auto produto = leite(5);
    VERIFICA_EXCECAO(produto->baixa_estoque(0), ErroEstoqueInsuficiente);
    VERIFICA_EXCECAO(produto->baixa_estoque(-2), ErroEstoqueInsuficiente);
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 5);
}

TESTE(produto_erro_de_estoque_eh_regra_de_negocio) {
    const auto produto = leite(1);
    VERIFICA_EXCECAO(produto->baixa_estoque(2), ErroRegraNegocio);
    VERIFICA_EXCECAO(produto->baixa_estoque(2), ErroEstoque);
}

TESTE(produto_repoe_estoque) {
    const auto produto = leite(10);
    produto->repoe_estoque(5);
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 15);
    VERIFICA_EXCECAO(produto->repoe_estoque(0), ErroValidacao);
    VERIFICA_EXCECAO(produto->repoe_estoque(-3), ErroValidacao);
    VERIFICA_IGUAL(produto->get_quantidade_produto(), 15);
}

TESTE(produto_reposicao_nao_estoura_o_limite) {
    const auto produto = leite(Produto::QUANTIDADE_MAXIMA - 1);
    VERIFICA_EXCECAO(produto->repoe_estoque(2), ErroRegraNegocio);
    VERIFICA_EXCECAO(produto->repoe_estoque(Produto::QUANTIDADE_MAXIMA), ErroRegraNegocio);
    VERIFICA_NAO_LANCA(produto->repoe_estoque(1));
    VERIFICA_IGUAL(produto->get_quantidade_produto(), Produto::QUANTIDADE_MAXIMA);
}

TESTE(produto_calcula_valor_arredondando_centavos) {
    const auto produto = arroz();   // 19.99
    VERIFICA(produto->calcula_valor(3) == 59.97);   // 19.99 * 3 em ponto flutuante daria 59.96999...
    VERIFICA(produto->calcula_valor(1) == 19.99);
    VERIFICA(produto->calcula_valor(0) == 0.0);
}

// ---------------------------------------------------------------- polimorfismo

TESTE(produto_fabrica_escolhe_a_classe_concreta) {
    VERIFICA(dynamic_cast<ProdutoPerecivel*>(leite().get()) != nullptr);
    VERIFICA(dynamic_cast<ProdutoNaoPerecivel*>(arroz().get()) != nullptr);
    VERIFICA(dynamic_cast<ProdutoPerecivel*>(arroz().get()) == nullptr);
}

TESTE(produto_comportamento_depende_do_tipo) {
    const std::vector<std::shared_ptr<Produto>> produtos = {leite(), arroz()};   // lista única, tipos diferentes
    const Data hoje(2026, 10, 30);

    VERIFICA_IGUAL(produtos[0]->get_tipo(), std::string("perecivel"));
    VERIFICA_IGUAL(produtos[1]->get_tipo(), std::string("nao perecivel"));

    VERIFICA_IGUAL(produtos[0]->validade_formatada(), std::string("01/11/2026"));
    VERIFICA_IGUAL(produtos[1]->validade_formatada(), std::string("-"));

    VERIFICA(produtos[0]->get_validade_produto().has_value());
    VERIFICA(!produtos[1]->get_validade_produto().has_value());

    VERIFICA_IGUAL(*produtos[0]->dias_para_vencer(hoje), 2);
    VERIFICA(!produtos[1]->dias_para_vencer(hoje).has_value());
}

TESTE(produto_perecivel_vence_depois_da_validade) {
    const auto produto = Produto::cria(1, "iogurte", 5, 3.0, CHEGADA, Data(2026, 10, 30), "", CATEGORIA);
    VERIFICA(!produto->esta_vencido(Data(2026, 10, 29)));
    VERIFICA(!produto->esta_vencido(Data(2026, 10, 30)));   // no dia da validade ainda pode ser usado
    VERIFICA(produto->esta_vencido(Data(2026, 10, 31)));
    VERIFICA_IGUAL(*produto->dias_para_vencer(Data(2026, 11, 2)), -3);
}

TESTE(produto_nao_perecivel_nunca_vence) {
    VERIFICA(!arroz()->esta_vencido(Data(2099, 12, 31)));
}

TESTE(produto_perecivel_exige_validade_a_partir_da_chegada) {
    VERIFICA_EXCECAO_MSG(Produto::cria(1, "x", 1, 1.0, CHEGADA, Data(2026, 9, 30), "", CATEGORIA), ErroValidacao,
                         "anterior à data de chegada");
    VERIFICA_NAO_LANCA(Produto::cria(1, "x", 1, 1.0, CHEGADA, CHEGADA, "", CATEGORIA));   // validade == chegada vale
}

// ---------------------------------------------------------------- JSON

TESTE(produto_json_usa_os_nomes_de_coluna_do_original) {
    const Json linha = leite()->para_json();
    VERIFICA_IGUAL(linha.campo("id").como_int(), 1);
    VERIFICA_IGUAL(linha.campo("nome_produto").como_texto(), std::string("leite"));
    VERIFICA_IGUAL(linha.campo("quantidade").como_int(), 10);
    VERIFICA_PROXIMO(linha.campo("valor").como_real(), 4.5, 1e-12);
    VERIFICA_IGUAL(linha.campo("chegada").como_texto(), std::string("2026-10-01"));
    VERIFICA_IGUAL(linha.campo("validade").como_texto(), std::string("2026-11-01"));
    VERIFICA_IGUAL(linha.campo("descricao").como_texto(), std::string("integral 1L"));
    VERIFICA_IGUAL(linha.campo("id_categoria").como_int(), 1);
}

TESTE(produto_sem_validade_grava_nulo_como_o_null_do_banco) {
    VERIFICA(arroz()->para_json().campo("validade").eh_nulo());
}

TESTE(produto_ida_e_volta_em_json_preserva_o_tipo) {
    for (const auto& original : {leite(), arroz()}) {
        const auto lido = Produto::de_json(Json::de_texto(original->para_json().para_texto()), CATEGORIA);
        VERIFICA(lido->para_json() == original->para_json());
        VERIFICA_IGUAL(lido->get_tipo(), original->get_tipo());
    }
}

TESTE(produto_de_json_aceita_linha_sem_descricao) {
    const Json linha = Json::de_texto(
        R"({"id": 4, "nome_produto": "sal", "quantidade": 2, "valor": 3, "chegada": "2026-01-02", "validade": null, "id_categoria": 1})");
    const auto produto = Produto::de_json(linha, CATEGORIA);
    VERIFICA_IGUAL(produto->get_descricao_produto(), std::string(""));
    VERIFICA_PROXIMO(produto->get_valor_produto(), 3.0, 1e-12);   // inteiro no arquivo também serve como valor
}

TESTE(produto_de_json_invalido_vira_erro_de_persistencia) {
    const char* invalidas[] = {
        R"({"id": 1})",
        R"({"id": 1, "nome_produto": "x", "quantidade": -5, "valor": 1, "chegada": "2026-01-01", "id_categoria": 1})",
        R"({"id": 1, "nome_produto": "x", "quantidade": 1, "valor": 1, "chegada": "2026-13-01", "id_categoria": 1})",
        R"({"id": 1, "nome_produto": "x", "quantidade": 1, "valor": 1, "chegada": "2026-05-01", "validade": "2026-04-01", "id_categoria": 1})",
        R"({"id": 1, "nome_produto": "", "quantidade": 1, "valor": 1, "chegada": "2026-05-01", "id_categoria": 1})",
        R"({"id": 1, "nome_produto": "x", "quantidade": "muitos", "valor": 1, "chegada": "2026-05-01", "id_categoria": 1})"};
    for (const char* texto : invalidas) {
        bool lancou = false;
        try {
            Produto::de_json(Json::de_texto(texto), CATEGORIA);
        } catch (const ErroPersistencia&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar: " << texto);
    }
}

TESTE(produto_resumo_identifica_o_produto) {
    VERIFICA_IGUAL(leite()->resumo(), std::string("produto 1 (leite)"));
}
