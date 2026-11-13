#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <memory>

/**
 * @file terminal.hpp
 * @brief Detalhes do terminal: saber se há uma pessoa digitando e esconder a senha digitada.
 */

/** @brief true se a entrada padrão é um terminal interativo (e não um arquivo ou pipe redirecionado). */
bool entrada_padrao_eh_terminal();

/**
 * @brief Desliga o eco do terminal enquanto o objeto existir (RAII), para digitar senhas.
 *
 * O eco é religado no destrutor, mesmo se ocorrer uma exceção. Não faz nada se `ativar`
 * for false ou se a entrada não for um terminal (por exemplo, quando a entrada vem de arquivo).
 * Em sistemas sem suporte (ex.: Windows nativo) também não faz nada.
 */
class EcoDesativado {
public:
    explicit EcoDesativado(bool ativar);
    ~EcoDesativado();
    EcoDesativado(const EcoDesativado&) = delete;
    EcoDesativado& operator=(const EcoDesativado&) = delete;

    /** @brief true se o eco foi realmente desligado. */
    bool esta_ativo() const;

private:
    struct Estado;
    std::unique_ptr<Estado> estado;
};

#endif
