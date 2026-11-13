#include "vendas_dao.hpp"

#include <cmath>
#include <map>

#include "erros.hpp"

VendasDAO::VendasDAO(BancoDado& banco, const ProdutoDAO& produtos) : banco(banco), produtos(produtos) {}

void VendasDAO::registra_vendas(Vendas& venda) {
    if (venda.foi_persistida()) {
        throw ErroRegraNegocio("a venda já foi registrada");
    }
    try {
        produtos.busca_produto_id(venda.get_produto_venda().get_id());
    } catch (const ErroNaoEncontrado&) {
        throw ErroIntegridade("o produto da venda não existe");
    }
    venda.set_id(banco.get_tabela(TABELA_VENDAS).insere(venda.para_json()));
}

std::vector<std::shared_ptr<Vendas>> VendasDAO::select_registros() const {
    std::map<int, std::shared_ptr<const Produto>> produto_por_id;
    for (const std::shared_ptr<Produto>& produto : produtos.select_todos_produtos()) {
        produto_por_id[produto->get_id()] = produto;
    }

    std::vector<std::shared_ptr<Vendas>> lista_venda;
    for (const Json& linha : banco.get_tabela(TABELA_VENDAS).select_todas()) {
        const auto produto = produto_por_id.find(linha.campo("produto_vendido").como_int());
        if (produto == produto_por_id.end()) {
            throw ErroIntegridade("a venda " + std::to_string(linha.campo("id").como_int()) +
                                  " referencia um produto inexistente");
        }
        lista_venda.push_back(Vendas::de_json(linha, produto->second));
    }
    return lista_venda;
}

std::vector<std::shared_ptr<Vendas>> VendasDAO::pesquisar_periodo(const Data& data_inicial,
                                                                   const Data& data_final) const {
    if (data_final < data_inicial) {
        throw ErroValidacao("período inválido: a data inicial é posterior à data final");
    }
    std::vector<std::shared_ptr<Vendas>> lista_venda;
    for (const std::shared_ptr<Vendas>& venda : select_registros()) {
        if (venda->get_data_venda() >= data_inicial && venda->get_data_venda() <= data_final) {
            lista_venda.push_back(venda);
        }
    }
    return lista_venda;
}

double VendasDAO::valor_por_periodo(const Data& data_inicial, const Data& data_final) const {
    double total = 0.0;
    for (const std::shared_ptr<Vendas>& venda : pesquisar_periodo(data_inicial, data_final)) {
        total += venda->get_valor_venda();
    }
    return std::round(total * 100.0) / 100.0;
}
