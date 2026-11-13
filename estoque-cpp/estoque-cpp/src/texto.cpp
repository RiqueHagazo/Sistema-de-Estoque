#include "texto.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

#include "erros.hpp"

std::string remove_espacos(const std::string& texto) {
    const char* ESPACOS = " \t\n\r\f\v";
    const std::size_t inicio = texto.find_first_not_of(ESPACOS);
    if (inicio == std::string::npos) {
        return "";
    }
    const std::size_t fim = texto.find_last_not_of(ESPACOS);
    return texto.substr(inicio, fim - inicio + 1);
}

std::string normaliza(const std::string& texto) {
    // letras de U+00E0 a U+00FF sem acento (a posição 23, U+00F7, é o sinal de divisão)
    static const char SEM_ACENTO[] = "aaaaaaaceeeeiiiidnooooo/ouuuuyty";
    std::string resultado;
    resultado.reserve(texto.size());
    for (std::size_t i = 0; i < texto.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(texto[i]);
        if (c < 0x80) {
            resultado += static_cast<char>(std::tolower(c));
            continue;
        }
        if (c == 0xC3 && i + 1 < texto.size()) {
            const unsigned char c2 = static_cast<unsigned char>(texto[i + 1]);
            if (c2 >= 0x80 && c2 <= 0xBF) {
                unsigned int ponto = 0xC0u + (c2 - 0x80u);
                if (ponto <= 0xDE && ponto != 0xD7) {
                    ponto += 0x20;   // maiúscula acentuada -> minúscula
                }
                if (ponto >= 0xE0 && ponto != 0xF7) {
                    resultado += SEM_ACENTO[ponto - 0xE0];
                    ++i;
                    continue;
                }
            }
        }
        resultado += static_cast<char>(c);
    }
    return resultado;
}

bool iguais_normalizados(const std::string& a, const std::string& b) {
    return normaliza(a) == normaliza(b);
}

std::size_t tamanho_utf8(const std::string& texto) {
    std::size_t total = 0;
    for (const char c : texto) {
        if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
            ++total;   // bytes de continuação (10xxxxxx) não iniciam caractere
        }
    }
    return total;
}

std::string limita_texto(const std::string& texto, std::size_t largura) {
    if (tamanho_utf8(texto) <= largura) {
        return texto;
    }
    const std::size_t manter = largura > 3 ? largura - 3 : largura;
    std::size_t caracteres = 0;
    std::size_t bytes = 0;
    while (bytes < texto.size()) {
        if ((static_cast<unsigned char>(texto[bytes]) & 0xC0) != 0x80) {
            if (caracteres == manter) {
                break;
            }
            ++caracteres;
        }
        ++bytes;
    }
    return texto.substr(0, bytes) + (largura > 3 ? "..." : "");
}

std::string preenche(const std::string& texto, std::size_t largura) {
    const std::size_t tamanho = tamanho_utf8(texto);
    if (tamanho >= largura) {
        return texto;
    }
    return texto + std::string(largura - tamanho, ' ');
}

std::string formata_valor(double valor) {
    if (!std::isfinite(valor)) {
        return "";
    }
    std::ostringstream saida;
    saida.imbue(std::locale::classic());
    saida << std::fixed << std::setprecision(2) << std::fabs(valor);
    const std::string numero = saida.str();               // ex.: "1234.50"
    const std::size_t ponto = numero.find('.');
    const std::string inteira = numero.substr(0, ponto);
    const std::string fracao = numero.substr(ponto);

    std::string agrupada;
    int contador = 0;
    for (auto it = inteira.rbegin(); it != inteira.rend(); ++it) {
        if (contador > 0 && contador % 3 == 0) {
            agrupada += ',';
        }
        agrupada += *it;
        ++contador;
    }
    std::reverse(agrupada.begin(), agrupada.end());

    const bool negativo = valor < 0 && numero != "0.00";
    return (negativo ? "-" : "") + agrupada + fracao;
}

int converte_inteiro(const std::string& texto, const std::string& campo) {
    const std::string limpo = remove_espacos(texto);
    const std::string mensagem = "entrada inválida em '" + campo + "': esperado número inteiro";

    std::size_t inicio = (!limpo.empty() && (limpo[0] == '-' || limpo[0] == '+')) ? 1 : 0;
    if (inicio >= limpo.size()) {
        throw ErroValidacao(mensagem);
    }
    for (std::size_t i = inicio; i < limpo.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(limpo[i]))) {
            throw ErroValidacao(mensagem);
        }
    }
    try {
        const long long valor = std::stoll(limpo);
        if (valor < INT_MIN || valor > INT_MAX) {
            throw ErroValidacao(mensagem + " dentro da faixa permitida");
        }
        return static_cast<int>(valor);
    } catch (const std::out_of_range&) {
        throw ErroValidacao(mensagem + " dentro da faixa permitida");
    }
}

double converte_real(const std::string& texto, const std::string& campo) {
    std::string limpo = remove_espacos(texto);
    const std::string mensagem = "entrada inválida em '" + campo + "': esperado número";

    std::replace(limpo.begin(), limpo.end(), ',', '.');
    std::size_t inicio = (!limpo.empty() && (limpo[0] == '-' || limpo[0] == '+')) ? 1 : 0;
    bool tem_digito = false;
    int pontos = 0;
    for (std::size_t i = inicio; i < limpo.size(); ++i) {
        if (limpo[i] == '.') {
            ++pontos;
        } else if (std::isdigit(static_cast<unsigned char>(limpo[i]))) {
            tem_digito = true;
        } else {
            throw ErroValidacao(mensagem);
        }
    }
    if (!tem_digito || pontos > 1) {
        throw ErroValidacao(mensagem);
    }
    std::istringstream entrada(limpo);
    entrada.imbue(std::locale::classic());
    double valor = 0.0;
    entrada >> valor;
    if (entrada.fail() || !std::isfinite(valor)) {
        throw ErroValidacao(mensagem);
    }
    return valor;
}

std::vector<std::string> divide(const std::string& texto, char separador) {
    std::vector<std::string> partes;
    std::string parte;
    std::istringstream entrada(texto);
    while (std::getline(entrada, parte, separador)) {
        partes.push_back(parte);
    }
    if (!texto.empty() && texto.back() == separador) {
        partes.push_back("");
    }
    return partes;
}
