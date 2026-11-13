#include <cstring>
#include <iostream>
#include <string>

#include "teste.hpp"

/**
 * Executa todos os testes registrados.
 *   ./testes            roda tudo (mostra só as falhas e o resumo)
 *   ./testes -v         lista cada teste
 *   ./testes <trecho>   roda só os testes cujo nome contém <trecho>
 */
int main(int argc, char** argv) {
    bool detalhado = false;
    std::string filtro;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-v") == 0) {
            detalhado = true;
        } else {
            filtro = argv[i];
        }
    }

    int executados = 0;
    int falhas = 0;
    for (const CasoDeTeste& caso : casos_de_teste()) {
        if (!filtro.empty() && std::string(caso.nome).find(filtro) == std::string::npos) {
            continue;
        }
        ++executados;
        try {
            caso.funcao();
            if (detalhado) std::cout << "[ OK  ] " << caso.nome << '\n';
        } catch (const FalhaDeTeste& falha) {
            ++falhas;
            std::cout << "[FALHA] " << caso.nome << "\n        " << falha.mensagem << '\n';
        } catch (const std::exception& e) {
            ++falhas;
            std::cout << "[FALHA] " << caso.nome << "\n        exceção inesperada: " << e.what() << '\n';
        } catch (...) {
            ++falhas;
            std::cout << "[FALHA] " << caso.nome << "\n        exceção desconhecida\n";
        }
    }
    std::cout << (executados - falhas) << "/" << executados << " testes passaram";
    if (falhas > 0) std::cout << " (" << falhas << " falharam)";
    std::cout << std::endl;
    return falhas == 0 ? 0 : 1;
}
