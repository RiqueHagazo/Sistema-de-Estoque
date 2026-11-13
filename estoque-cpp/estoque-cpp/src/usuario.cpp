#include "usuario.hpp"

#include "crypto.hpp"
#include "erros.hpp"

// ############################## USUARIO ##############################

Usuario::Usuario(int id, const std::string& user, const std::string& senha_cryptada)
    : Entidade(id), usuario(valida_texto(user, "usuário", true)), senha(senha_cryptada) {
    if (senha.empty()) {
        throw ErroValidacao("senha não pode ficar em branco");
    }
}

const std::string& Usuario::get_usuario() const { return usuario; }
const std::string& Usuario::get_senha() const { return senha; }

bool Usuario::valida_senha(const std::string& senha_digitada) const {
    return ::valida_senha(senha_digitada, senha);
}

Json Usuario::para_json() const {
    Json linha = Json::objeto();
    linha.define("id", get_id()).define("usuario", usuario).define("senha", senha).define("grupo", get_grupo());
    return linha;
}

std::string Usuario::resumo() const {
    return "usuário " + usuario + " (" + get_nome_grupo() + ")";
}

std::shared_ptr<Usuario> Usuario::cria(int grupo, int id, const std::string& user, const std::string& senha_cryptada) {
    switch (grupo) {
        case GRUPO_OPERADOR: return std::make_shared<Operador>(id, user, senha_cryptada);
        case GRUPO_ADMINISTRADOR: return std::make_shared<Administrador>(id, user, senha_cryptada);
        default: throw ErroValidacao("grupo inválido: use 0 (operador) ou 1 (administrador)");
    }
}

std::shared_ptr<Usuario> Usuario::de_json(const Json& linha) {
    try {
        return cria(linha.campo("grupo").como_int(), linha.campo("id").como_int(), linha.campo("usuario").como_texto(),
                    linha.campo("senha").como_texto());
    } catch (const ErroValidacao& e) {
        throw ErroPersistencia(std::string("registro de usuário inválido: ") + e.what());
    }
}

// ############################## OPERADOR ##############################

Operador::Operador(int id, const std::string& user, const std::string& senha_cryptada)
    : Usuario(id, user, senha_cryptada) {}
int Operador::get_grupo() const { return GRUPO_OPERADOR; }
std::string Operador::get_nome_grupo() const { return "operador"; }
bool Operador::pode_administrar() const { return false; }

// ############################## ADMINISTRADOR ##############################

Administrador::Administrador(int id, const std::string& user, const std::string& senha_cryptada)
    : Usuario(id, user, senha_cryptada) {}
int Administrador::get_grupo() const { return GRUPO_ADMINISTRADOR; }
std::string Administrador::get_nome_grupo() const { return "administrador"; }
bool Administrador::pode_administrar() const { return true; }
