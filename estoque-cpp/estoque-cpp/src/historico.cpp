#include "historico.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "erros.hpp"

namespace {

// o histórico é uma linha por operação: quebras de linha dentro dos textos são trocadas por espaço
std::string em_uma_linha(std::string texto) {
    std::replace(texto.begin(), texto.end(), '\n', ' ');
    std::replace(texto.begin(), texto.end(), '\r', ' ');
    return texto;
}

}  // namespace

Historico::Historico(const std::string& caminho, const Relogio& relogio) : caminho(caminho), relogio(relogio) {}

bool Historico::registra(const std::string& usuario, const std::string& operacao, const std::string& detalhe) noexcept {
    try {
        std::ofstream arquivo(caminho, std::ios::app);
        if (!arquivo) {
            return false;
        }
        arquivo << relogio.hoje().para_texto() << ' ' << relogio.agora().para_texto() << " | " << em_uma_linha(usuario)
                << " | " << em_uma_linha(operacao) << " | " << em_uma_linha(detalhe) << '\n';
        arquivo.flush();
        return static_cast<bool>(arquivo);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> Historico::ultimas_linhas(std::size_t quantidade) const {
    std::vector<std::string> linhas;
    std::error_code erro;
    if (!std::filesystem::exists(caminho, erro)) {
        return linhas;   // ainda não há histórico
    }
    std::ifstream arquivo(caminho);
    if (!arquivo) {
        throw ErroPersistencia("não foi possível abrir o histórico " + caminho);
    }
    std::string linha;
    while (std::getline(arquivo, linha)) {
        if (!linha.empty()) {
            linhas.push_back(linha);
        }
    }
    if (arquivo.bad()) {
        throw ErroPersistencia("falha ao ler o histórico " + caminho);
    }
    if (linhas.size() > quantidade) {
        linhas.erase(linhas.begin(), linhas.end() - static_cast<std::ptrdiff_t>(quantidade));
    }
    return linhas;
}
