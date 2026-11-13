#include <memory>
#include <vector>

#include "crypto.hpp"
#include "erros.hpp"
#include "teste.hpp"
#include "usuario.hpp"

namespace {
const std::string HASH_MARIA = cryptar("segredo", 10);
const std::string HASH_ADMIN = cryptar("admin123", 10);
}  // namespace

TESTE(usuario_operador_nao_administra) {
    const Operador operador(1, "maria", HASH_MARIA);
    VERIFICA_IGUAL(operador.get_grupo(), Usuario::GRUPO_OPERADOR);
    VERIFICA_IGUAL(operador.get_nome_grupo(), std::string("operador"));
    VERIFICA(!operador.pode_administrar());
}

TESTE(usuario_administrador_administra) {
    const Administrador admin(2, "admin", HASH_ADMIN);
    VERIFICA_IGUAL(admin.get_grupo(), Usuario::GRUPO_ADMINISTRADOR);
    VERIFICA_IGUAL(admin.get_nome_grupo(), std::string("administrador"));
    VERIFICA(admin.pode_administrar());
}

TESTE(usuario_polimorfismo_pela_classe_base) {
    // o código que usa Usuario não precisa saber qual é a classe concreta (acaba o `if grupo == 1`)
    const std::vector<std::shared_ptr<Usuario>> usuarios = {std::make_shared<Operador>(1, "maria", HASH_MARIA),
                                                            std::make_shared<Administrador>(2, "admin", HASH_ADMIN)};
    int quantos_administram = 0;
    for (const auto& usuario : usuarios) {
        if (usuario->pode_administrar()) ++quantos_administram;
    }
    VERIFICA_IGUAL(quantos_administram, 1);
    VERIFICA_IGUAL(usuarios[0]->resumo(), std::string("usuário maria (operador)"));
    VERIFICA_IGUAL(usuarios[1]->resumo(), std::string("usuário admin (administrador)"));
}

TESTE(usuario_fabrica_escolhe_a_classe_pelo_grupo) {
    VERIFICA(dynamic_cast<Operador*>(Usuario::cria(0, 1, "a", HASH_MARIA).get()) != nullptr);
    VERIFICA(dynamic_cast<Administrador*>(Usuario::cria(1, 1, "a", HASH_MARIA).get()) != nullptr);
    VERIFICA_EXCECAO_MSG(Usuario::cria(2, 1, "a", HASH_MARIA), ErroValidacao, "grupo inválido");
    VERIFICA_EXCECAO(Usuario::cria(-1, 1, "a", HASH_MARIA), ErroValidacao);
}

TESTE(usuario_valida_senha_contra_o_hash) {
    const Operador maria(1, "maria", HASH_MARIA);
    VERIFICA(maria.valida_senha("segredo"));
    VERIFICA(!maria.valida_senha("Segredo"));
    VERIFICA(!maria.valida_senha(""));
}

TESTE(usuario_guarda_o_hash_e_nao_a_senha) {
    const Operador maria(1, "maria", HASH_MARIA);
    VERIFICA_IGUAL(maria.get_senha(), HASH_MARIA);
    VERIFICA(maria.get_senha().find("segredo") == std::string::npos);
    VERIFICA(maria.para_json().campo("senha").como_texto().find("segredo") == std::string::npos);
}

TESTE(usuario_valida_nome_e_senha) {
    VERIFICA_EXCECAO(Operador(1, "", HASH_MARIA), ErroValidacao);
    VERIFICA_EXCECAO(Operador(1, "   ", HASH_MARIA), ErroValidacao);
    VERIFICA_EXCECAO(Operador(1, std::string(256, 'u'), HASH_MARIA), ErroValidacao);
    VERIFICA_EXCECAO(Operador(1, "maria", ""), ErroValidacao);
    VERIFICA_IGUAL(Operador(1, "  maria ", HASH_MARIA).get_usuario(), std::string("maria"));
}

TESTE(usuario_nome_diferencia_maiusculas) {
    // o original usava `LIKE BINARY`: "Maria" e "maria" são usuários diferentes
    VERIFICA(Operador(1, "Maria", HASH_MARIA).get_usuario() != Operador(2, "maria", HASH_MARIA).get_usuario());
}

TESTE(usuario_ida_e_volta_em_json_preserva_o_grupo) {
    const std::shared_ptr<Usuario> originais[] = {std::make_shared<Operador>(1, "maria", HASH_MARIA),
                                                  std::make_shared<Administrador>(2, "admin", HASH_ADMIN)};
    for (const auto& original : originais) {
        const auto lido = Usuario::de_json(Json::de_texto(original->para_json().para_texto()));
        VERIFICA_IGUAL(lido->get_usuario(), original->get_usuario());
        VERIFICA_IGUAL(lido->get_grupo(), original->get_grupo());
        VERIFICA_IGUAL(lido->get_senha(), original->get_senha());
        VERIFICA_IGUAL(lido->pode_administrar(), original->pode_administrar());
    }
}

TESTE(usuario_de_json_invalido_vira_erro_de_persistencia) {
    VERIFICA_EXCECAO(Usuario::de_json(Json::de_texto(R"({"id": 1, "usuario": "a", "senha": "x", "grupo": 7})")), ErroPersistencia);
    VERIFICA_EXCECAO(Usuario::de_json(Json::de_texto(R"({"id": 1, "usuario": "a", "senha": "x"})")), ErroPersistencia);
    VERIFICA_EXCECAO(Usuario::de_json(Json::de_texto(R"({"id": 1, "usuario": "", "senha": "x", "grupo": 0})")), ErroPersistencia);
    VERIFICA_EXCECAO(Usuario::de_json(Json::de_texto(R"({"id": 1, "usuario": "a", "senha": "", "grupo": 0})")), ErroPersistencia);
}
