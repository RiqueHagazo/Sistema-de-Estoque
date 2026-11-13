#include "terminal.hpp"

#include <utility>

#if defined(__unix__) || defined(__APPLE__)

#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include <csignal>

namespace {

// Estado compartilhado com o tratador de sinais (só há um eco desligado por vez).
termios terminal_original;
volatile sig_atomic_t eco_desligado = 0;

const int SINAIS_TRATADOS[] = {SIGINT, SIGTERM, SIGHUP};
constexpr int QUANTIDADE_SINAIS = 3;

// Ctrl+C (ou kill) durante a senha encerra o processo sem rodar destrutores: religa o eco antes de morrer,
// senão o terminal do usuário ficaria sem mostrar o que ele digita. Só usa funções seguras em sinais.
void religa_eco_e_repete_sinal(int sinal) {
    if (eco_desligado) {
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal_original);
    }
    signal(sinal, SIG_DFL);
    raise(sinal);
}

}  // namespace

struct EcoDesativado::Estado {
    struct sigaction acoes_anteriores[QUANTIDADE_SINAIS];
};

bool entrada_padrao_eh_terminal() {
    return isatty(STDIN_FILENO) != 0;
}

EcoDesativado::EcoDesativado(bool ativar) {
    if (!ativar || eco_desligado || !entrada_padrao_eh_terminal()) {
        return;
    }
    termios original;
    if (tcgetattr(STDIN_FILENO, &original) != 0) {
        return;
    }
    termios sem_eco = original;
    sem_eco.c_lflag &= ~static_cast<tcflag_t>(ECHO);

    auto novo_estado = std::make_unique<Estado>();
    terminal_original = original;
    eco_desligado = 1;

    struct sigaction acao;
    acao.sa_handler = religa_eco_e_repete_sinal;
    sigemptyset(&acao.sa_mask);
    acao.sa_flags = 0;
    for (int i = 0; i < QUANTIDADE_SINAIS; ++i) {
        sigaction(SINAIS_TRATADOS[i], &acao, &novo_estado->acoes_anteriores[i]);
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &sem_eco) != 0) {
        for (int i = 0; i < QUANTIDADE_SINAIS; ++i) {
            sigaction(SINAIS_TRATADOS[i], &novo_estado->acoes_anteriores[i], nullptr);
        }
        eco_desligado = 0;
        return;
    }
    estado = std::move(novo_estado);
}

EcoDesativado::~EcoDesativado() {
    if (estado) {
        tcsetattr(STDIN_FILENO, TCSANOW, &terminal_original);
        for (int i = 0; i < QUANTIDADE_SINAIS; ++i) {
            sigaction(SINAIS_TRATADOS[i], &estado->acoes_anteriores[i], nullptr);
        }
        eco_desligado = 0;
    }
}

#elif defined(_WIN32)

#include <io.h>
#include <cstdio>

struct EcoDesativado::Estado {};

bool entrada_padrao_eh_terminal() {
    return _isatty(_fileno(stdin)) != 0;
}

EcoDesativado::EcoDesativado(bool) {}
EcoDesativado::~EcoDesativado() = default;

#else

struct EcoDesativado::Estado {};

bool entrada_padrao_eh_terminal() {
    return false;
}

EcoDesativado::EcoDesativado(bool) {}
EcoDesativado::~EcoDesativado() = default;

#endif

bool EcoDesativado::esta_ativo() const {
    return static_cast<bool>(estado);
}
