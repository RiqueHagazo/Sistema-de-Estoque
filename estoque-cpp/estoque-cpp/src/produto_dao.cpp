#include "produto_dao.hpp"

#include "erros.hpp"
#include "texto.hpp"

ProdutoDAO::ProdutoDAO(BancoDado& banco) : banco(banco) {}

std::map<int, Categoria> ProdutoDAO::carrega_categorias() const {
    std::map<int, Categoria> categorias;
    for (const Json& linha : banco.get_tabela(TABELA_CATEGORIA).select_todas()) {
        const Categoria categoria = Categoria::de_json(linha);
        categorias.emplace(categoria.get_id(), categoria);
    }
    return categorias;
}

std::shared_ptr<Produto> ProdutoDAO::converte(const Json& linha, const std::map<int, Categoria>& categorias) const {
    const int id_categoria = linha.campo("id_categoria").como_int();
    const auto encontrada = categorias.find(id_categoria);
    if (encontrada == categorias.end()) {
        throw ErroIntegridade("o produto " + std::to_string(linha.campo("id").como_int()) +
                              " referencia uma categoria inexistente (" + std::to_string(id_categoria) + ")");
    }
    return Produto::de_json(linha, encontrada->second);
}

bool ProdutoDAO::possui_vendas(int id_produto) const {
    for (const Json& linha : banco.get_tabela(TABELA_VENDAS).select_todas()) {
        if (linha.campo("produto_vendido").como_int() == id_produto) {
            return true;
        }
    }
    return false;
}

void ProdutoDAO::insere_produto(Produto& produto) {
    if (produto.foi_persistida()) {
        throw ErroRegraNegocio("o produto já foi gravado");
    }
    if (carrega_categorias().count(produto.get_categoria_produto().get_id()) == 0) {
        throw ErroIntegridade("categoria inválida");
    }
    produto.set_id(banco.get_tabela(TABELA_PRODUTO).insere(produto.para_json()));
}

std::vector<std::shared_ptr<Produto>> ProdutoDAO::select_todos_produtos() const {
    const std::map<int, Categoria> categorias = carrega_categorias();
    std::vector<std::shared_ptr<Produto>> lista_produto;
    for (const Json& linha : banco.get_tabela(TABELA_PRODUTO).select_todas()) {
        lista_produto.push_back(converte(linha, categorias));
    }
    return lista_produto;
}

std::vector<std::shared_ptr<Produto>> ProdutoDAO::busca_produto(const std::string& nome_p) const {
    std::vector<std::shared_ptr<Produto>> lista_produto;
    for (const std::shared_ptr<Produto>& produto : select_todos_produtos()) {
        if (iguais_normalizados(produto->get_nome_produto(), remove_espacos(nome_p))) {
            lista_produto.push_back(produto);
        }
    }
    return lista_produto;
}

std::shared_ptr<Produto> ProdutoDAO::busca_produto_id(int id) const {
    for (const Json& linha : banco.get_tabela(TABELA_PRODUTO).select_todas()) {
        if (linha.campo("id").como_int() == id) {
            return converte(linha, carrega_categorias());
        }
    }
    throw ErroNaoEncontrado("produto não encontrado");
}

void ProdutoDAO::excluir_produto(int id_produto) {
    busca_produto_id(id_produto);   // lança ErroNaoEncontrado se não existir
    if (possui_vendas(id_produto)) {
        throw ErroIntegridade("o produto possui vendas registradas e não pode ser excluído");
    }
    banco.get_tabela(TABELA_PRODUTO).exclui(id_produto);
}

void ProdutoDAO::atualiza_quantidade(int id, int quantidade) {
    if (quantidade < 0 || quantidade > Produto::QUANTIDADE_MAXIMA) {
        throw ErroValidacao("quantidade inválida: use um valor entre 0 e " + std::to_string(Produto::QUANTIDADE_MAXIMA));
    }
    Tabela tabela = banco.get_tabela(TABELA_PRODUTO);
    for (const Json& linha : tabela.select_todas()) {
        if (linha.campo("id").como_int() == id) {
            Json atualizada = linha;
            atualizada.define("quantidade", quantidade);
            tabela.atualiza(atualizada);
            return;
        }
    }
    throw ErroNaoEncontrado("produto não encontrado");
}
