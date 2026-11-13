#include "categoria_dao.hpp"

#include <algorithm>

#include "erros.hpp"
#include "texto.hpp"

CategoriaDAO::CategoriaDAO(BancoDado& banco) : banco(banco) {}

void CategoriaDAO::insert_categoria(Categoria& categoria) {
    if (categoria.foi_persistida()) {
        throw ErroRegraNegocio("a categoria já foi gravada");
    }
    Tabela tabela = banco.get_tabela(TABELA_CATEGORIA);
    for (const Json& linha : tabela.select_todas()) {
        if (iguais_normalizados(linha.campo("nome").como_texto(), categoria.get_nome())) {
            throw ErroIntegridade("categoria já existe");
        }
    }
    categoria.set_id(tabela.insere(categoria.para_json()));
}

std::vector<std::shared_ptr<Categoria>> CategoriaDAO::select_todas_categorias() const {
    std::vector<std::shared_ptr<Categoria>> lista_categoria;
    for (const Json& linha : banco.get_tabela(TABELA_CATEGORIA).select_todas()) {
        lista_categoria.push_back(std::make_shared<Categoria>(Categoria::de_json(linha)));
    }
    std::sort(lista_categoria.begin(), lista_categoria.end(),
              [](const std::shared_ptr<Categoria>& a, const std::shared_ptr<Categoria>& b) {
                  const std::string nome_a = normaliza(a->get_nome());
                  const std::string nome_b = normaliza(b->get_nome());
                  return nome_a != nome_b ? nome_a < nome_b : a->get_id() < b->get_id();
              });
    return lista_categoria;
}

std::shared_ptr<Categoria> CategoriaDAO::busca_categoria_id(int id) const {
    for (const Json& linha : banco.get_tabela(TABELA_CATEGORIA).select_todas()) {
        if (linha.campo("id").como_int() == id) {
            return std::make_shared<Categoria>(Categoria::de_json(linha));
        }
    }
    throw ErroNaoEncontrado("categoria não encontrada");
}
