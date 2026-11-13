#include "produto.hpp"

#include <cmath>

#include "erros.hpp"

namespace {

int valida_quantidade(int quantidade) {
    if (quantidade < 0 || quantidade > Produto::QUANTIDADE_MAXIMA) {
        throw ErroValidacao("quantidade inválida: use um valor entre 0 e " + std::to_string(Produto::QUANTIDADE_MAXIMA));
    }
    return quantidade;
}

double valida_valor(double valor) {
    if (!std::isfinite(valor) || valor < 0 || valor > Produto::VALOR_MAXIMO) {
        throw ErroValidacao("valor inválido: use um número entre 0 e 1000000000");
    }
    return valor;
}

double arredonda_centavos(double valor) {
    return std::round(valor * 100.0) / 100.0;
}

}  // namespace

// ############################## PRODUTO ##############################

Produto::Produto(int id, const std::string& nome, int quantidade, double valor, const Data& chegada,
                 const std::string& descricao, const Categoria& categoria)
    : Entidade(id),
      nome_produto(valida_texto(nome, "nome do produto", true)),
      quantidade_produto(valida_quantidade(quantidade)),
      valor_produto(valida_valor(valor)),
      chegada_produto(chegada),
      descricao_produto(valida_texto(descricao, "descrição", false)),
      categoria_produto(categoria) {}

const std::string& Produto::get_nome_produto() const { return nome_produto; }
int Produto::get_quantidade_produto() const { return quantidade_produto; }
double Produto::get_valor_produto() const { return valor_produto; }
const Data& Produto::get_chegada_produto() const { return chegada_produto; }
const std::string& Produto::get_descricao_produto() const { return descricao_produto; }
const Categoria& Produto::get_categoria_produto() const { return categoria_produto; }

void Produto::set_nome_produto(const std::string& nome) {
    nome_produto = valida_texto(nome, "nome do produto", true);
}

void Produto::set_quantidade_produto(int quantidade) {
    quantidade_produto = valida_quantidade(quantidade);
}

void Produto::set_valor_produto(double valor) {
    valor_produto = valida_valor(valor);
}

void Produto::set_descricao_produto(const std::string& descricao) {
    descricao_produto = valida_texto(descricao, "descrição", false);
}

void Produto::baixa_estoque(int quantidade_vendida) {
    // mesma regra e mesma mensagem da rota `realiza_venda` do projeto original
    if (quantidade_vendida <= 0 || quantidade_vendida > quantidade_produto) {
        throw ErroEstoqueInsuficiente("produtos insuficientes ou igual a zero");
    }
    quantidade_produto -= quantidade_vendida;
}

void Produto::repoe_estoque(int quantidade) {
    if (quantidade <= 0) {
        throw ErroValidacao("a quantidade de reposição deve ser maior que zero");
    }
    if (quantidade > QUANTIDADE_MAXIMA - quantidade_produto) {   // evita overflow antes de somar
        throw ErroRegraNegocio("a reposição ultrapassa o estoque máximo permitido");
    }
    quantidade_produto += quantidade;
}

double Produto::calcula_valor(int quantidade) const {
    return arredonda_centavos(valor_produto * quantidade);
}

std::string Produto::validade_formatada() const {
    const std::optional<Data> validade = get_validade_produto();
    return validade ? validade->formatada() : "-";
}

Json Produto::para_json() const {
    const std::optional<Data> validade = get_validade_produto();
    Json linha = Json::objeto();
    linha.define("id", get_id())
        .define("nome_produto", nome_produto)
        .define("quantidade", quantidade_produto)
        .define("valor", valor_produto)
        .define("chegada", chegada_produto.para_texto())
        .define("validade", validade ? Json(validade->para_texto()) : Json())
        .define("descricao", descricao_produto)
        .define("id_categoria", categoria_produto.get_id());
    return linha;
}

std::string Produto::resumo() const {
    return "produto " + std::to_string(get_id()) + " (" + nome_produto + ")";
}

std::shared_ptr<Produto> Produto::cria(int id, const std::string& nome, int quantidade, double valor,
                                       const Data& chegada, const std::optional<Data>& validade,
                                       const std::string& descricao, const Categoria& categoria) {
    if (validade) {
        return std::make_shared<ProdutoPerecivel>(id, nome, quantidade, valor, chegada, *validade, descricao, categoria);
    }
    return std::make_shared<ProdutoNaoPerecivel>(id, nome, quantidade, valor, chegada, descricao, categoria);
}

std::shared_ptr<Produto> Produto::de_json(const Json& linha, const Categoria& categoria) {
    try {
        std::optional<Data> validade;
        if (linha.tem_campo("validade") && !linha.campo("validade").eh_nulo()) {
            validade = Data::de_texto(linha.campo("validade").como_texto());
        }
        const std::string descricao =
            (linha.tem_campo("descricao") && !linha.campo("descricao").eh_nulo()) ? linha.campo("descricao").como_texto() : "";
        return cria(linha.campo("id").como_int(), linha.campo("nome_produto").como_texto(),
                    linha.campo("quantidade").como_int(), linha.campo("valor").como_real(),
                    Data::de_texto(linha.campo("chegada").como_texto()), validade, descricao, categoria);
    } catch (const ErroValidacao& e) {
        throw ErroPersistencia(std::string("registro de produto inválido: ") + e.what());
    }
}

// ############################## NAO PERECIVEL ##############################

ProdutoNaoPerecivel::ProdutoNaoPerecivel(int id, const std::string& nome, int quantidade, double valor,
                                         const Data& chegada, const std::string& descricao, const Categoria& categoria)
    : Produto(id, nome, quantidade, valor, chegada, descricao, categoria) {}

std::optional<Data> ProdutoNaoPerecivel::get_validade_produto() const { return std::nullopt; }
bool ProdutoNaoPerecivel::esta_vencido(const Data&) const { return false; }
std::optional<int> ProdutoNaoPerecivel::dias_para_vencer(const Data&) const { return std::nullopt; }
std::string ProdutoNaoPerecivel::get_tipo() const { return "nao perecivel"; }

// ############################## PERECIVEL ##############################

ProdutoPerecivel::ProdutoPerecivel(int id, const std::string& nome, int quantidade, double valor, const Data& chegada,
                                   const Data& validade, const std::string& descricao, const Categoria& categoria)
    : Produto(id, nome, quantidade, valor, chegada, descricao, categoria), validade_produto(validade) {
    if (validade < chegada) {
        throw ErroValidacao("a validade não pode ser anterior à data de chegada");
    }
}

std::optional<Data> ProdutoPerecivel::get_validade_produto() const { return validade_produto; }
bool ProdutoPerecivel::esta_vencido(const Data& hoje) const { return validade_produto < hoje; }
std::optional<int> ProdutoPerecivel::dias_para_vencer(const Data& hoje) const { return hoje.dias_ate(validade_produto); }
std::string ProdutoPerecivel::get_tipo() const { return "perecivel"; }
