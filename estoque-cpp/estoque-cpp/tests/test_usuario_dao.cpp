#include "auxiliar.hpp"
#include "crypto.hpp"
#include "erros.hpp"
#include "teste.hpp"

TESTE(usuario_dao_cadastra_e_atribui_o_id) {
    Ambiente ambiente;
    VERIFICA_IGUAL(ambiente.novo_usuario("maria", "segredo", Usuario::GRUPO_OPERADOR)->get_id(), 1);
    VERIFICA_IGUAL(ambiente.novo_usuario("admin", "admin123", Usuario::GRUPO_ADMINISTRADOR)->get_id(), 2);
}

TESTE(usuario_dao_rejeita_usuario_repetido) {
    Ambiente ambiente;
    ambiente.novo_usuario("maria", "segredo", 0);
    VERIFICA_EXCECAO_MSG(ambiente.novo_usuario("maria", "outra", 1), ErroIntegridade, "usuário já existe");
    VERIFICA_IGUAL(ambiente.usuarios.conta_usuarios(), 1u);
}

TESTE(usuario_dao_diferencia_maiusculas_de_minusculas) {
    Ambiente ambiente;
    ambiente.novo_usuario("Maria", "segredo", 0);
    VERIFICA_NAO_LANCA(ambiente.novo_usuario("maria", "segredo", 0));   // outro usuário, como no `LIKE BINARY`
    VERIFICA_IGUAL(ambiente.usuarios.conta_usuarios(), 2u);
}

TESTE(usuario_dao_nunca_grava_senha_em_texto_puro) {
    Ambiente ambiente;
    auto usuario = std::make_shared<Operador>(Entidade::ID_NAO_DEFINIDO, "maria", "minha-senha-em-texto-puro");
    VERIFICA_EXCECAO_MSG(ambiente.usuarios.cadastrar_usuario(*usuario), ErroValidacao, "criptografada");
    VERIFICA_IGUAL(ambiente.usuarios.conta_usuarios(), 0u);

    ambiente.novo_usuario("joao", "segredo123", 0);
    VERIFICA(le_arquivo(ambiente.pasta.arquivo("usuarios.json")).find("segredo123") == std::string::npos);
}

TESTE(usuario_dao_nao_grava_duas_vezes_o_mesmo_objeto) {
    Ambiente ambiente;
    const auto usuario = ambiente.novo_usuario("maria", "segredo", 0);
    VERIFICA_EXCECAO(ambiente.usuarios.cadastrar_usuario(*usuario), ErroRegraNegocio);
}

TESTE(usuario_dao_login_com_dados_corretos_devolve_a_classe_certa) {
    Ambiente ambiente;
    ambiente.novo_usuario("maria", "segredo", Usuario::GRUPO_OPERADOR);
    ambiente.novo_usuario("admin", "admin123", Usuario::GRUPO_ADMINISTRADOR);

    const auto maria = ambiente.usuarios.verifica_login("maria", "segredo");
    VERIFICA(maria != nullptr);
    VERIFICA(!maria->pode_administrar());
    VERIFICA(dynamic_cast<Operador*>(maria.get()) != nullptr);

    const auto admin = ambiente.usuarios.verifica_login("admin", "admin123");
    VERIFICA(admin != nullptr);
    VERIFICA(admin->pode_administrar());
    VERIFICA(dynamic_cast<Administrador*>(admin.get()) != nullptr);
}

TESTE(usuario_dao_login_com_dados_errados_devolve_nulo) {
    Ambiente ambiente;
    ambiente.novo_usuario("maria", "segredo", 0);
    VERIFICA(ambiente.usuarios.verifica_login("maria", "errada") == nullptr);
    VERIFICA(ambiente.usuarios.verifica_login("maria", "") == nullptr);
    VERIFICA(ambiente.usuarios.verifica_login("fantasma", "segredo") == nullptr);
    VERIFICA(ambiente.usuarios.verifica_login("", "") == nullptr);
    VERIFICA(ambiente.usuarios.verifica_login("MARIA", "segredo") == nullptr);   // o nome diferencia maiúsculas
}

TESTE(usuario_dao_login_com_hash_desconhecido_no_arquivo_apenas_falha) {
    // por exemplo, um hash bcrypt trazido do sistema antigo: não derruba o programa, só não autentica
    Ambiente ambiente;
    escreve_arquivo(ambiente.pasta.arquivo("usuarios.json"),
                    R"({"proximo_id": 2, "linhas": [{"id": 1, "usuario": "antigo", "senha": "$2b$12$abcdefghijklmnopqrstuv", "grupo": 1}]})");
    VERIFICA_NAO_LANCA(ambiente.usuarios.verifica_login("antigo", "qualquer"));
    VERIFICA(ambiente.usuarios.verifica_login("antigo", "qualquer") == nullptr);
}

TESTE(usuario_dao_lista_em_ordem_alfabetica) {
    Ambiente ambiente;
    for (const char* nome : {"zeca", "Ana", "bia", "álvaro"}) {
        ambiente.novo_usuario(nome, "senha1", 0);
    }
    const auto lista = ambiente.usuarios.select_usuarios();
    VERIFICA_IGUAL(lista.size(), 4u);
    VERIFICA_IGUAL(lista[0]->get_usuario(), std::string("álvaro"));
    VERIFICA_IGUAL(lista[1]->get_usuario(), std::string("Ana"));
    VERIFICA_IGUAL(lista[2]->get_usuario(), std::string("bia"));
    VERIFICA_IGUAL(lista[3]->get_usuario(), std::string("zeca"));
}

TESTE(usuario_dao_exclui_usuario) {
    Ambiente ambiente;
    ambiente.novo_usuario("maria", "segredo", 0);
    ambiente.novo_usuario("joao", "segredo", 0);
    ambiente.usuarios.excluir_usuario("maria");
    VERIFICA_IGUAL(ambiente.usuarios.conta_usuarios(), 1u);
    VERIFICA(ambiente.usuarios.verifica_login("maria", "segredo") == nullptr);
    VERIFICA_EXCECAO_MSG(ambiente.usuarios.excluir_usuario("maria"), ErroNaoEncontrado, "usuário não encontrado");
    VERIFICA_EXCECAO(ambiente.usuarios.excluir_usuario("MARIA"), ErroNaoEncontrado);
}

TESTE(usuario_dao_busca_usuario) {
    Ambiente ambiente;
    ambiente.novo_usuario("maria", "segredo", 0);
    VERIFICA_IGUAL(ambiente.usuarios.busca_usuario("maria")->get_usuario(), std::string("maria"));
    VERIFICA_EXCECAO(ambiente.usuarios.busca_usuario("ninguem"), ErroNaoEncontrado);
}

TESTE(usuario_dao_conta_administradores) {
    Ambiente ambiente;
    VERIFICA_IGUAL(ambiente.usuarios.conta_administradores(), 0u);
    ambiente.novo_usuario("maria", "segredo", 0);
    ambiente.novo_usuario("admin1", "segredo", 1);
    ambiente.novo_usuario("admin2", "segredo", 1);
    VERIFICA_IGUAL(ambiente.usuarios.conta_usuarios(), 3u);
    VERIFICA_IGUAL(ambiente.usuarios.conta_administradores(), 2u);
}

TESTE(usuario_dao_linha_corrompida_vira_erro_de_persistencia) {
    Ambiente ambiente;
    escreve_arquivo(ambiente.pasta.arquivo("usuarios.json"),
                    R"({"proximo_id": 2, "linhas": [{"id": 1, "usuario": "x", "senha": "y", "grupo": 9}]})");
    VERIFICA_EXCECAO(ambiente.usuarios.verifica_login("x", "y"), ErroPersistencia);
    VERIFICA_EXCECAO(ambiente.usuarios.select_usuarios(), ErroPersistencia);
}
