#include "vendas.hpp"

#include <cmath>

#include "erros.hpp"

namespace {

const Produto& exige_produto(const std::shared_ptr<const Produto>& produto) {
    if (!produto) {
        throw ErroValidacao("a venda precisa de um produto");
    }
    return *produto;
}

int valida_quantidade_venda(int quantidade) {
    if (quantidade <= 0 || quantidade > Produto::QUANTIDADE_MAXIMA) {
        throw ErroValidacao("quantidade vendida inválida");
    }
    return quantidade;
}

double valida_valor_venda(double valor) {
    if (!std::isfinite(valor) || valor < 0) {
        throw ErroValidacao("valor da venda inválido");
    }
    return valor;
}

}  // namespace

Vendas::Vendas(int id, const std::string& usuario, const std::shared_ptr<const Produto>& produto, int quantidade,
               double valor, const Data& data, const Horario& horario)
    : Entidade(id),
      vendedor(valida_texto(usuario, "vendedor", true)),
      produto_venda((exige_produto(produto), produto)),
      quantidade_venda(valida_quantidade_venda(quantidade)),
      valor_venda(valida_valor_venda(valor)),
      data_venda(data),
      horario_venda(horario) {}

const std::string& Vendas::get_vendedor() const { return vendedor; }
const Produto& Vendas::get_produto_venda() const { return *produto_venda; }
int Vendas::get_quantidade_venda() const { return quantidade_venda; }
double Vendas::get_valor_venda() const { return valor_venda; }
const Data& Vendas::get_data_venda() const { return data_venda; }
const Horario& Vendas::get_horario_venda() const { return horario_venda; }

Json Vendas::para_json() const {
    Json linha = Json::objeto();
    linha.define("id", get_id())
        .define("produto_vendido", produto_venda->get_id())
        .define("vendedor", vendedor)
        .define("data_venda", data_venda.para_texto())
        .define("horario_venda", horario_venda.para_texto())
        .define("quantidade_venda", quantidade_venda)
        .define("valor_venda", valor_venda);
    return linha;
}

std::string Vendas::resumo() const {
    return "venda " + std::to_string(get_id()) + " de " + std::to_string(quantidade_venda) + " x " +
           produto_venda->get_nome_produto();
}

std::shared_ptr<Vendas> Vendas::de_json(const Json& linha, const std::shared_ptr<const Produto>& produto) {
    try {
        return std::make_shared<Vendas>(linha.campo("id").como_int(), linha.campo("vendedor").como_texto(), produto,
                                        linha.campo("quantidade_venda").como_int(), linha.campo("valor_venda").como_real(),
                                        Data::de_texto(linha.campo("data_venda").como_texto()),
                                        Horario::de_texto(linha.campo("horario_venda").como_texto()));
    } catch (const ErroValidacao& e) {
        throw ErroPersistencia(std::string("registro de venda inválido: ") + e.what());
    }
}
