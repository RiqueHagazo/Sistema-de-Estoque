#ifndef ERROS_HPP
#define ERROS_HPP

#include <stdexcept>
#include <string>

/**
 * @file erros.hpp
 * @brief Hierarquia de exceções do sistema de estoque.
 *
 * Toda falha prevista pelo sistema deriva de ErroEstoque. Assim, o código pode
 * tratar um erro específico (ex.: ErroEstoqueInsuficiente) ou qualquer erro do
 * sistema de uma só vez (catch por ErroEstoque).
 *
 * @code
 * ErroEstoque
 *  |-- ErroValidacao            dado informado é inválido
 *  |-- ErroPersistencia         falha ao ler/gravar os arquivos
 *  |    `-- ErroJson            texto JSON malformado
 *  |-- ErroNaoEncontrado        registro inexistente
 *  |-- ErroIntegridade          violação de chave estrangeira / unicidade
 *  |-- ErroRegraNegocio         regra do domínio violada
 *  |    `-- ErroEstoqueInsuficiente
 *  |-- ErroAutenticacao         login ou senha inválidos
 *  `-- ErroPermissao            usuário sem permissão para a operação
 * @endcode
 */

/** @brief Raiz da hierarquia de erros do sistema. */
class ErroEstoque : public std::runtime_error {
public:
    /** @param mensagem texto exibido ao usuário (em português, minúsculo). */
    explicit ErroEstoque(const std::string& mensagem) : std::runtime_error(mensagem) {}
};

/** @brief Dado informado é inválido (campo vazio, número fora da faixa, data impossível...). */
class ErroValidacao : public ErroEstoque {
public:
    using ErroEstoque::ErroEstoque;
};

/** @brief Falha ao ler ou gravar os arquivos de dados. */
class ErroPersistencia : public ErroEstoque {
public:
    using ErroEstoque::ErroEstoque;
};

/** @brief Texto JSON malformado ou valor JSON de tipo inesperado. */
class ErroJson : public ErroPersistencia {
public:
    using ErroPersistencia::ErroPersistencia;
};

/** @brief Registro procurado não existe. */
class ErroNaoEncontrado : public ErroEstoque {
public:
    using ErroEstoque::ErroEstoque;
};

/** @brief Operação violaria a integridade dos dados (equivalente às FKs/UNIQUE do banco). */
class ErroIntegridade : public ErroEstoque {
public:
    using ErroEstoque::ErroEstoque;
};

/** @brief Regra de negócio violada. */
class ErroRegraNegocio : public ErroEstoque {
public:
    using ErroEstoque::ErroEstoque;
};

/** @brief Quantidade solicitada é zero, negativa ou maior que o estoque disponível. */
class ErroEstoqueInsuficiente : public ErroRegraNegocio {
public:
    using ErroRegraNegocio::ErroRegraNegocio;
};

/** @brief Usuário ou senha incorretos. */
class ErroAutenticacao : public ErroEstoque {
public:
    using ErroEstoque::ErroEstoque;
};

/** @brief Usuário autenticado, mas sem permissão para a operação. */
class ErroPermissao : public ErroEstoque {
public:
    using ErroEstoque::ErroEstoque;
};

#endif
