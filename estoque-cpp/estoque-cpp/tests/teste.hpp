#ifndef TESTE_HPP
#define TESTE_HPP

/**
 * @file teste.hpp
 * @brief Mini-framework de testes de unidade (sem dependências externas).
 *
 * Uso:
 * @code
 * TESTE(soma_funciona) {
 *     VERIFICA_IGUAL(1 + 1, 2);
 *     VERIFICA_EXCECAO(converte_inteiro("x", "campo"), ErroValidacao);
 * }
 * @endcode
 * Os testes se registram sozinhos; `test_main.cpp` executa todos.
 */

#include <cmath>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct CasoDeTeste {
    const char* nome;
    const char* arquivo;
    void (*funcao)();
};

inline std::vector<CasoDeTeste>& casos_de_teste() {
    static std::vector<CasoDeTeste> casos;
    return casos;
}

struct RegistradorDeTeste {
    RegistradorDeTeste(const char* nome, const char* arquivo, void (*funcao)()) {
        casos_de_teste().push_back({nome, arquivo, funcao});
    }
};

/** @brief Lançada pelas macros VERIFICA_* quando uma verificação falha. */
struct FalhaDeTeste {
    std::string mensagem;
};

namespace teste_detalhe {

template <typename T, typename = void>
struct EhImprimivel : std::false_type {};

template <typename T>
struct EhImprimivel<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
    : std::true_type {};

template <typename T>
std::string mostra(const T& valor) {
    if constexpr (EhImprimivel<T>::value) {
        std::ostringstream saida;
        saida << std::boolalpha << valor;
        return saida.str();
    } else {
        return "<nao imprimivel>";
    }
}

inline std::string mostra(const std::string& valor) { return "\"" + valor + "\""; }

/// Compara dentro de uma função: os temporários passados como argumento vivem até o fim da chamada.
template <typename A, typename B>
void verifica_igual(const A& obtido, const B& esperado, const char* expressao, const char* arquivo, int linha) {
    if (!(obtido == esperado)) {
        std::ostringstream saida;
        saida << arquivo << ':' << linha << ": esperado " << mostra(esperado) << " mas foi obtido "
              << mostra(obtido) << "  [" << expressao << "]";
        throw FalhaDeTeste{saida.str()};
    }
}

}  // namespace teste_detalhe

#define TESTE(nome)                                                              \
    static void teste_##nome();                                                  \
    static RegistradorDeTeste registrador_##nome(#nome, __FILE__, &teste_##nome); \
    static void teste_##nome()

#define FALHA(mensagem)                                                  \
    do {                                                                 \
        std::ostringstream _oss;                                         \
        _oss << __FILE__ << ':' << __LINE__ << ": " << mensagem;         \
        throw FalhaDeTeste{_oss.str()};                                  \
    } while (0)

#define VERIFICA(condicao)                                    \
    do {                                                      \
        if (!(condicao)) FALHA("VERIFICA(" #condicao ") falhou"); \
    } while (0)

#define VERIFICA_IGUAL(obtido, esperado) \
    teste_detalhe::verifica_igual((obtido), (esperado), #obtido " == " #esperado, __FILE__, __LINE__)

#define VERIFICA_PROXIMO(obtido, esperado, tolerancia)                                          \
    do {                                                                                        \
        const double _o = (obtido);                                                             \
        const double _e = (esperado);                                                           \
        if (!(std::fabs(_o - _e) <= (tolerancia))) {                                            \
            FALHA("esperado ~" << _e << " mas foi obtido " << _o << "  [" #obtido "]");         \
        }                                                                                       \
    } while (0)

#define VERIFICA_EXCECAO(expressao, TipoExcecao)                                                \
    do {                                                                                        \
        bool _lancou = false;                                                                   \
        try {                                                                                   \
            (void)(expressao);                                                                  \
        } catch (const TipoExcecao&) {                                                          \
            _lancou = true;                                                                     \
        } catch (const std::exception& _e) {                                                    \
            FALHA("lançou exceção de outro tipo (" << _e.what() << ") em: " #expressao);        \
        }                                                                                       \
        if (!_lancou) FALHA("esperava exceção " #TipoExcecao " em: " #expressao);               \
    } while (0)

/// Como VERIFICA_EXCECAO, e também confere se a mensagem contém `trecho`.
#define VERIFICA_EXCECAO_MSG(expressao, TipoExcecao, trecho)                                    \
    do {                                                                                        \
        bool _lancou = false;                                                                   \
        try {                                                                                   \
            (void)(expressao);                                                                  \
        } catch (const TipoExcecao& _e) {                                                       \
            _lancou = true;                                                                     \
            if (std::string(_e.what()).find(trecho) == std::string::npos) {                     \
                FALHA("mensagem \"" << _e.what() << "\" não contém \"" << trecho << "\"");      \
            }                                                                                   \
        } catch (const std::exception& _e) {                                                    \
            FALHA("lançou exceção de outro tipo (" << _e.what() << ") em: " #expressao);        \
        }                                                                                       \
        if (!_lancou) FALHA("esperava exceção " #TipoExcecao " em: " #expressao);               \
    } while (0)

#define VERIFICA_NAO_LANCA(expressao)                                                           \
    do {                                                                                        \
        try {                                                                                   \
            (void)(expressao);                                                                  \
        } catch (const std::exception& _e) {                                                    \
            FALHA("não esperava exceção, mas lançou: " << _e.what());                           \
        }                                                                                       \
    } while (0)

#endif
