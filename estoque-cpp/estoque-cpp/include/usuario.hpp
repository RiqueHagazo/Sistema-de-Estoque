#ifndef USUARIO_HPP
#define USUARIO_HPP

#include <memory>
#include <string>

#include "entidade.hpp"

/**
 * @file usuario.hpp
 * @brief Usuários do sistema: classe base abstrata, Operador e Administrador.
 *
 * O campo `grupo` do projeto original (0 = operador, 1 = administrador), que era
 * testado com `if grupo == 1` em vários pontos do `main.py` e dos templates, virou
 * uma hierarquia: cada tipo de usuário responde por si se pode administrar o sistema.
 */

/** @brief Usuário do sistema (tabela `usuarios`). A senha é sempre guardada criptografada. */
class Usuario : public Entidade {
public:
    static constexpr int GRUPO_OPERADOR = 0;
    static constexpr int GRUPO_ADMINISTRADOR = 1;

    /**
     * @param id             identificador (Entidade::ID_NAO_DEFINIDO se ainda não gravado)
     * @param user           nome de usuário (diferencia maiúsculas de minúsculas, como o `LIKE BINARY` original)
     * @param senha_cryptada hash gerado por cryptar() (nunca a senha em texto puro)
     * @throws ErroValidacao se o nome estiver em branco/longo demais ou o hash estiver vazio.
     */
    Usuario(int id, const std::string& user, const std::string& senha_cryptada);
    ~Usuario() override = default;

    const std::string& get_usuario() const;
    /** @brief Hash da senha (não a senha). */
    const std::string& get_senha() const;

    /** @brief Confere uma senha em texto puro contra o hash guardado. */
    bool valida_senha(const std::string& senha) const;

    /** @name Comportamento polimórfico (depende do tipo de usuário) */
    ///@{
    /** @brief GRUPO_OPERADOR ou GRUPO_ADMINISTRADOR (valor gravado no arquivo). */
    virtual int get_grupo() const = 0;
    /** @brief "operador" ou "administrador". */
    virtual std::string get_nome_grupo() const = 0;
    /** @brief true se pode ver o registro de vendas e usar a área administrativa. */
    virtual bool pode_administrar() const = 0;
    ///@}

    Json para_json() const override;
    std::string resumo() const override;

    /**
     * @brief Fábrica: cria Operador ou Administrador conforme o grupo.
     * @throws ErroValidacao se o grupo não existir.
     */
    static std::shared_ptr<Usuario> cria(int grupo, int id, const std::string& user, const std::string& senha_cryptada);

    /** @brief Reconstrói o usuário a partir de uma linha do arquivo. @throws ErroPersistencia se inválida. */
    static std::shared_ptr<Usuario> de_json(const Json& linha);

private:
    std::string usuario;
    std::string senha;
};

/** @brief Usuário comum: registra vendas e consulta produtos. */
class Operador : public Usuario {
public:
    Operador(int id, const std::string& user, const std::string& senha_cryptada);
    int get_grupo() const override;
    std::string get_nome_grupo() const override;
    bool pode_administrar() const override;
};

/** @brief Usuário com acesso total: também vê o registro de vendas e gerencia usuários. */
class Administrador : public Usuario {
public:
    Administrador(int id, const std::string& user, const std::string& senha_cryptada);
    int get_grupo() const override;
    std::string get_nome_grupo() const override;
    bool pode_administrar() const override;
};

#endif
