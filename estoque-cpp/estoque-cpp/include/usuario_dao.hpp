#ifndef USUARIO_DAO_HPP
#define USUARIO_DAO_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "bancodado.hpp"
#include "usuario.hpp"

/**
 * @file usuario_dao.hpp
 * @brief Acesso aos dados de usuários (parte do antigo `dao.py`).
 */

/** @brief Grava e consulta usuários na tabela `usuarios`. */
class UsuarioDAO {
public:
    explicit UsuarioDAO(BancoDado& banco);

    /**
     * @brief Grava um novo usuário e atribui o id gerado ao objeto.
     * @throws ErroRegraNegocio se o usuário já foi gravado.
     * @throws ErroValidacao se a senha do objeto não estiver criptografada (impede gravar senha em texto puro).
     * @throws ErroIntegridade ("usuário já existe") se o nome já estiver em uso.
     */
    void cadastrar_usuario(Usuario& usuario);

    /**
     * @brief Confere usuário e senha (o `verifica_login` do original).
     * @return o usuário (já como Operador ou Administrador) se os dados conferem; nullptr caso contrário.
     * @note Leva tempo parecido para usuário inexistente e para senha errada, para não revelar quais usuários existem.
     */
    std::shared_ptr<Usuario> verifica_login(const std::string& usuario, const std::string& senha) const;

    /** @throws ErroNaoEncontrado se o usuário não existir (diferencia maiúsculas de minúsculas). */
    std::shared_ptr<Usuario> busca_usuario(const std::string& nome_usuario) const;

    /** @brief Todos os usuários em ordem alfabética (o `ORDER BY usuario` do original). */
    std::vector<std::shared_ptr<Usuario>> select_usuarios() const;

    /** @throws ErroNaoEncontrado se o usuário não existir. */
    void excluir_usuario(const std::string& nome_usuario);

    std::size_t conta_usuarios() const;
    std::size_t conta_administradores() const;

private:
    BancoDado& banco;
};

#endif
