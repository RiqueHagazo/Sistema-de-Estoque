#include "usuario_dao.hpp"

#include <algorithm>

#include "crypto.hpp"
#include "erros.hpp"
#include "texto.hpp"

UsuarioDAO::UsuarioDAO(BancoDado& banco) : banco(banco) {}

void UsuarioDAO::cadastrar_usuario(Usuario& usuario) {
    if (usuario.foi_persistida()) {
        throw ErroRegraNegocio("o usuário já foi gravado");
    }
    if (!eh_hash_valido(usuario.get_senha())) {
        throw ErroValidacao("a senha precisa ser criptografada antes de ser gravada");
    }
    Tabela tabela = banco.get_tabela(TABELA_USUARIOS);
    for (const Json& linha : tabela.select_todas()) {
        if (linha.campo("usuario").como_texto() == usuario.get_usuario()) {
            throw ErroIntegridade("usuário já existe");
        }
    }
    usuario.set_id(tabela.insere(usuario.para_json()));
}

std::shared_ptr<Usuario> UsuarioDAO::verifica_login(const std::string& usuario, const std::string& senha) const {
    for (const Json& linha : banco.get_tabela(TABELA_USUARIOS).select_todas()) {
        if (linha.campo("usuario").como_texto() == usuario) {
            const std::shared_ptr<Usuario> encontrado = Usuario::de_json(linha);
            return encontrado->valida_senha(senha) ? encontrado : nullptr;
        }
    }
    // usuário inexistente: faz a mesma conta de uma senha errada, para o tempo não denunciar o erro
    static const std::string HASH_FALSO = cryptar("senha-que-ninguem-usa");
    valida_senha(senha, HASH_FALSO);
    return nullptr;
}

std::shared_ptr<Usuario> UsuarioDAO::busca_usuario(const std::string& nome_usuario) const {
    for (const Json& linha : banco.get_tabela(TABELA_USUARIOS).select_todas()) {
        if (linha.campo("usuario").como_texto() == nome_usuario) {
            return Usuario::de_json(linha);
        }
    }
    throw ErroNaoEncontrado("usuário não encontrado");
}

std::vector<std::shared_ptr<Usuario>> UsuarioDAO::select_usuarios() const {
    std::vector<std::shared_ptr<Usuario>> lista_usuario;
    for (const Json& linha : banco.get_tabela(TABELA_USUARIOS).select_todas()) {
        lista_usuario.push_back(Usuario::de_json(linha));
    }
    std::sort(lista_usuario.begin(), lista_usuario.end(),
              [](const std::shared_ptr<Usuario>& a, const std::shared_ptr<Usuario>& b) {
                  const std::string nome_a = normaliza(a->get_usuario());
                  const std::string nome_b = normaliza(b->get_usuario());
                  return nome_a != nome_b ? nome_a < nome_b : a->get_usuario() < b->get_usuario();
              });
    return lista_usuario;
}

void UsuarioDAO::excluir_usuario(const std::string& nome_usuario) {
    const std::shared_ptr<Usuario> usuario = busca_usuario(nome_usuario);   // lança ErroNaoEncontrado
    banco.get_tabela(TABELA_USUARIOS).exclui(usuario->get_id());
}

std::size_t UsuarioDAO::conta_usuarios() const {
    return select_usuarios().size();
}

std::size_t UsuarioDAO::conta_administradores() const {
    std::size_t total = 0;
    for (const std::shared_ptr<Usuario>& usuario : select_usuarios()) {
        if (usuario->pode_administrar()) {
            ++total;
        }
    }
    return total;
}
