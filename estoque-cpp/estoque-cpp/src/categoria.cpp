#include "categoria.hpp"

#include "erros.hpp"

Categoria::Categoria(int id, const std::string& nome)
    : Entidade(id), nome_categoria(valida_texto(nome, "nome da categoria", true)) {}

const std::string& Categoria::get_nome() const { return nome_categoria; }

void Categoria::set_nome(const std::string& nome) {
    nome_categoria = valida_texto(nome, "nome da categoria", true);
}

Json Categoria::para_json() const {
    Json linha = Json::objeto();
    linha.define("id", get_id()).define("nome", nome_categoria);
    return linha;
}

std::string Categoria::resumo() const {
    return "categoria " + std::to_string(get_id()) + " (" + nome_categoria + ")";
}

Categoria Categoria::de_json(const Json& linha) {
    try {
        return Categoria(linha.campo("id").como_int(), linha.campo("nome").como_texto());
    } catch (const ErroValidacao& e) {
        throw ErroPersistencia(std::string("registro de categoria inválido: ") + e.what());
    }
}
