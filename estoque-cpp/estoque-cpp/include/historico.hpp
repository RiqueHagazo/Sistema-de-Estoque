#ifndef HISTORICO_HPP
#define HISTORICO_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "datahora.hpp"

/**
 * @file historico.hpp
 * @brief Histórico de operações em arquivo de texto (funcionalidade adicional).
 */

/**
 * @brief Registra o que cada usuário fez (logins, vendas, cadastros, exclusões...).
 *
 * Uma linha por operação, no formato:
 * `aaaa-mm-dd hh:mm:ss | usuario | OPERACAO | detalhe`
 *
 * O histórico é secundário: se não for possível gravá-lo, a operação principal
 * continua e registra() apenas devolve false.
 */
class Historico {
public:
    /**
     * @param caminho caminho do arquivo de texto
     * @param relogio fonte da data/hora (deve viver mais que o Historico)
     */
    Historico(const std::string& caminho, const Relogio& relogio);

    /**
     * @brief Acrescenta uma linha ao histórico.
     * @return true se gravou; false se falhou (nunca lança exceção).
     */
    bool registra(const std::string& usuario, const std::string& operacao, const std::string& detalhe) noexcept;

    /**
     * @brief As últimas `quantidade` linhas (da mais antiga para a mais recente).
     * @throws ErroPersistencia se o arquivo existir mas não puder ser lido.
     */
    std::vector<std::string> ultimas_linhas(std::size_t quantidade) const;

private:
    std::string caminho;
    const Relogio& relogio;
};

#endif
