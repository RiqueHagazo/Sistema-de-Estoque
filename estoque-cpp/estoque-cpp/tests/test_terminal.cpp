#include <stdexcept>

#include "teste.hpp"
#include "terminal.hpp"

TESTE(terminal_eco_nao_e_desativado_quando_nao_solicitado) {
    EcoDesativado eco(false);
    VERIFICA(!eco.esta_ativo());
}

TESTE(terminal_eco_so_e_desativado_em_terminal_interativo) {
    // nos testes automáticos a entrada normalmente não é um terminal: nada deve ser alterado
    EcoDesativado eco(true);
    VERIFICA_IGUAL(eco.esta_ativo(), entrada_padrao_eh_terminal());
}

#if defined(__linux__)

#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {

// Troca a entrada padrão por um pseudoterminal durante o teste, e restaura no fim (RAII).
class PseudoTerminalComoEntrada {
public:
    PseudoTerminalComoEntrada() {
        mestre = posix_openpt(O_RDWR | O_NOCTTY);
        if (mestre < 0 || grantpt(mestre) != 0 || unlockpt(mestre) != 0) return;
        const char* nome_escravo = ptsname(mestre);
        if (nome_escravo == nullptr) return;
        escravo = open(nome_escravo, O_RDWR | O_NOCTTY);
        if (escravo < 0) return;
        entrada_original = dup(STDIN_FILENO);
        if (entrada_original < 0) return;
        pronto = dup2(escravo, STDIN_FILENO) >= 0;
    }
    ~PseudoTerminalComoEntrada() {
        if (entrada_original >= 0) {
            dup2(entrada_original, STDIN_FILENO);
            close(entrada_original);
        }
        if (escravo >= 0) close(escravo);
        if (mestre >= 0) close(mestre);
    }
    bool esta_pronto() const { return pronto; }

private:
    int mestre = -1;
    int escravo = -1;
    int entrada_original = -1;
    bool pronto = false;
};

bool eco_ligado() {
    termios estado;
    tcgetattr(STDIN_FILENO, &estado);
    return (estado.c_lflag & ECHO) != 0;
}

}  // namespace

TESTE(terminal_desliga_o_eco_enquanto_vive_e_religa_no_fim) {
    PseudoTerminalComoEntrada terminal;
    if (!terminal.esta_pronto()) return;   // ambiente sem pseudoterminais: nada a testar
    VERIFICA(entrada_padrao_eh_terminal());
    VERIFICA(eco_ligado());
    {
        EcoDesativado eco(true);
        VERIFICA(eco.esta_ativo());
        VERIFICA(!eco_ligado());
    }
    VERIFICA(eco_ligado());   // religado ao sair do escopo
}

TESTE(terminal_religa_o_eco_mesmo_quando_ocorre_excecao) {
    PseudoTerminalComoEntrada terminal;
    if (!terminal.esta_pronto()) return;
    try {
        EcoDesativado eco(true);
        VERIFICA(!eco_ligado());
        throw std::runtime_error("falha qualquer durante a digitação da senha");
    } catch (const std::runtime_error&) {
    }
    VERIFICA(eco_ligado());
}

TESTE(terminal_religa_o_eco_se_o_processo_for_interrompido_por_sinal) {
    // Ctrl+C na hora da senha: o processo morre sem rodar destrutores; o eco precisa voltar mesmo assim.
    for (const int sinal : {SIGINT, SIGTERM, SIGHUP}) {
        PseudoTerminalComoEntrada terminal;
        if (!terminal.esta_pronto()) return;
        const pid_t filho = fork();
        if (filho == 0) {
            EcoDesativado eco(true);
            raise(sinal);
            _exit(99);   // não deve chegar aqui: o sinal encerra o processo
        }
        VERIFICA(filho > 0);
        int status = 0;
        VERIFICA_IGUAL(waitpid(filho, &status, 0), filho);
        VERIFICA(WIFSIGNALED(status));                 // morreu pelo sinal, como esperado pelo usuário
        VERIFICA_IGUAL(WTERMSIG(status), sinal);
        VERIFICA(eco_ligado());                        // mas deixou o terminal em ordem
    }
}

TESTE(terminal_nao_mexe_no_eco_se_nao_for_solicitado) {
    PseudoTerminalComoEntrada terminal;
    if (!terminal.esta_pronto()) return;
    EcoDesativado eco(false);
    VERIFICA(!eco.esta_ativo());
    VERIFICA(eco_ligado());
}

#endif
