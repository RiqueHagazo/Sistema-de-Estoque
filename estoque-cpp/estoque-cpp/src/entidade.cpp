#include "entidade.hpp"

#include "erros.hpp"
#include "texto.hpp"

Entidade::Entidade(int id) : id(id) {
    if (id < 0) {
        throw ErroValidacao("id inválido");
    }
}

int Entidade::get_id() const { return id; }

void Entidade::set_id(int novo_id) {
    if (novo_id < 0) {
        throw ErroValidacao("id inválido");
    }
    id = novo_id;
}

bool Entidade::foi_persistida() const { return id != ID_NAO_DEFINIDO; }

std::string Entidade::valida_texto(const std::string& valor, const std::string& campo, bool obrigatorio) {
    const std::string limpo = remove_espacos(valor);
    if (obrigatorio && limpo.empty()) {
        throw ErroValidacao(campo + " não pode ficar em branco");
    }
    if (tamanho_utf8(limpo) > TAMANHO_MAXIMO_TEXTO) {
        throw ErroValidacao(campo + " muito longo (máximo " + std::to_string(TAMANHO_MAXIMO_TEXTO) + " caracteres)");
    }
    for (const char c : limpo) {
        if (static_cast<unsigned char>(c) < 0x20) {
            throw ErroValidacao(campo + " contém caracteres inválidos");
        }
    }
    return limpo;
}
