#include "json.hpp"

#include <cmath>
#include <cstdio>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

#include "erros.hpp"

// ############################## CONSTRUTORES ##############################

Json::Json(bool valor) : tipo(Tipo::BOOLEANO), booleano(valor) {}
Json::Json(int valor) : tipo(Tipo::INTEIRO), inteiro(valor) {}
Json::Json(long long valor) : tipo(Tipo::INTEIRO), inteiro(valor) {}
Json::Json(double valor) : tipo(Tipo::REAL), real(valor) {}
Json::Json(const char* valor) : tipo(Tipo::TEXTO), texto(valor != nullptr ? valor : "") {}
Json::Json(const std::string& valor) : tipo(Tipo::TEXTO), texto(valor) {}

Json Json::array() {
    Json json;
    json.tipo = Tipo::ARRAY;
    return json;
}

Json Json::objeto() {
    Json json;
    json.tipo = Tipo::OBJETO;
    return json;
}

// ############################## CONSULTAS ##############################

Json::Tipo Json::get_tipo() const { return tipo; }
bool Json::eh_nulo() const { return tipo == Tipo::NULO; }
bool Json::eh_texto() const { return tipo == Tipo::TEXTO; }
bool Json::eh_numero() const { return tipo == Tipo::INTEIRO || tipo == Tipo::REAL; }
bool Json::eh_array() const { return tipo == Tipo::ARRAY; }
bool Json::eh_objeto() const { return tipo == Tipo::OBJETO; }

void Json::exige(Tipo esperado, const char* nome) const {
    if (tipo != esperado) {
        throw ErroJson(std::string("valor json não é do tipo esperado (") + nome + ")");
    }
}

bool Json::como_booleano() const {
    exige(Tipo::BOOLEANO, "booleano");
    return booleano;
}

long long Json::como_inteiro() const {
    exige(Tipo::INTEIRO, "inteiro");
    return inteiro;
}

int Json::como_int() const {
    const long long valor = como_inteiro();
    if (valor < std::numeric_limits<int>::min() || valor > std::numeric_limits<int>::max()) {
        throw ErroJson("número inteiro fora da faixa permitida");
    }
    return static_cast<int>(valor);
}

double Json::como_real() const {
    if (tipo == Tipo::INTEIRO) {
        return static_cast<double>(inteiro);
    }
    exige(Tipo::REAL, "número");
    return real;
}

const std::string& Json::como_texto() const {
    exige(Tipo::TEXTO, "texto");
    return texto;
}

// ############################## ARRAYS ##############################

Json& Json::adiciona(const Json& valor) {
    exige(Tipo::ARRAY, "array");
    itens_.push_back(valor);
    return *this;
}

std::size_t Json::tamanho() const {
    if (tipo != Tipo::ARRAY && tipo != Tipo::OBJETO) {
        throw ErroJson("tamanho só existe para array ou objeto");
    }
    return itens_.size();
}

const Json& Json::item(std::size_t indice) const {
    exige(Tipo::ARRAY, "array");
    if (indice >= itens_.size()) {
        throw ErroJson("índice fora do array");
    }
    return itens_[indice];
}

const std::vector<Json>& Json::itens() const {
    if (tipo != Tipo::ARRAY && tipo != Tipo::OBJETO) {
        throw ErroJson("itens só existem para array ou objeto");
    }
    return itens_;
}

// ############################## OBJETOS ##############################

Json& Json::define(const std::string& chave, const Json& valor) {
    exige(Tipo::OBJETO, "objeto");
    for (std::size_t i = 0; i < chaves_.size(); ++i) {
        if (chaves_[i] == chave) {
            itens_[i] = valor;
            return *this;
        }
    }
    chaves_.push_back(chave);
    itens_.push_back(valor);
    return *this;
}

bool Json::tem_campo(const std::string& chave) const {
    if (tipo != Tipo::OBJETO) {
        return false;
    }
    for (const std::string& existente : chaves_) {
        if (existente == chave) {
            return true;
        }
    }
    return false;
}

const Json& Json::campo(const std::string& chave) const {
    exige(Tipo::OBJETO, "objeto");
    for (std::size_t i = 0; i < chaves_.size(); ++i) {
        if (chaves_[i] == chave) {
            return itens_[i];
        }
    }
    throw ErroJson("campo ausente: " + chave);
}

const std::vector<std::string>& Json::chaves() const {
    exige(Tipo::OBJETO, "objeto");
    return chaves_;
}

bool Json::operator==(const Json& outro) const {
    if (tipo != outro.tipo) {
        return false;
    }
    switch (tipo) {
        case Tipo::NULO: return true;
        case Tipo::BOOLEANO: return booleano == outro.booleano;
        case Tipo::INTEIRO: return inteiro == outro.inteiro;
        case Tipo::REAL: return real == outro.real;
        case Tipo::TEXTO: return texto == outro.texto;
        case Tipo::ARRAY: return itens_ == outro.itens_;
        case Tipo::OBJETO: return chaves_ == outro.chaves_ && itens_ == outro.itens_;
    }
    return false;
}

// ############################## LEITURA (PARSER) ##############################

namespace {

constexpr int PROFUNDIDADE_MAXIMA = 64;   // protege contra documentos aninhados demais

bool eh_digito(char c) { return c >= '0' && c <= '9'; }

bool texto_para_real(const std::string& token, double& resultado) {
    std::istringstream entrada(token);
    entrada.imbue(std::locale::classic());
    entrada >> resultado;
    return !entrada.fail() && entrada.eof();
}

class Interpretador {
public:
    explicit Interpretador(const std::string& texto_json) : texto(texto_json) {}

    Json interpreta() {
        pula_espacos();
        Json resultado = le_valor();
        pula_espacos();
        if (!fim()) {
            erro("conteúdo inesperado após o fim do documento");
        }
        return resultado;
    }

private:
    const std::string& texto;
    std::size_t posicao = 0;
    int profundidade = 0;

    [[noreturn]] void erro(const std::string& mensagem) const {
        int linha = 1;
        int coluna = 1;
        for (std::size_t i = 0; i < posicao && i < texto.size(); ++i) {
            if (texto[i] == '\n') {
                ++linha;
                coluna = 1;
            } else {
                ++coluna;
            }
        }
        throw ErroJson("json inválido (linha " + std::to_string(linha) + ", coluna " +
                       std::to_string(coluna) + "): " + mensagem);
    }

    bool fim() const { return posicao >= texto.size(); }
    char atual() const { return fim() ? '\0' : texto[posicao]; }

    void pula_espacos() {
        while (!fim() && (atual() == ' ' || atual() == '\t' || atual() == '\n' || atual() == '\r')) {
            ++posicao;
        }
    }

    void espera(char esperado) {
        if (atual() != esperado) {
            erro(std::string("esperado '") + esperado + "'");
        }
        ++posicao;
    }

    void entra() {
        if (++profundidade > PROFUNDIDADE_MAXIMA) {
            erro("documento aninhado demais");
        }
    }

    void sai() { --profundidade; }

    Json le_valor() {
        if (fim()) {
            erro("fim inesperado do documento");
        }
        const char c = atual();
        if (c == '{') return le_objeto();
        if (c == '[') return le_array();
        if (c == '"') return Json(le_texto());
        if (c == 't') { le_literal("true"); return Json(true); }
        if (c == 'f') { le_literal("false"); return Json(false); }
        if (c == 'n') { le_literal("null"); return Json(); }
        if (c == '-' || eh_digito(c)) return le_numero();
        erro(std::string("caractere inesperado '") + c + "'");
    }

    void le_literal(const std::string& literal) {
        if (texto.compare(posicao, literal.size(), literal) != 0) {
            erro("valor desconhecido");
        }
        posicao += literal.size();
    }

    Json le_objeto() {
        entra();
        Json objeto = Json::objeto();
        ++posicao;   // '{'
        pula_espacos();
        if (atual() == '}') {
            ++posicao;
            sai();
            return objeto;
        }
        while (true) {
            pula_espacos();
            if (atual() != '"') {
                erro("esperado nome de campo entre aspas");
            }
            const std::string chave = le_texto();
            pula_espacos();
            espera(':');
            pula_espacos();
            objeto.define(chave, le_valor());
            pula_espacos();
            if (atual() == ',') {
                ++posicao;
                continue;
            }
            if (atual() == '}') {
                ++posicao;
                break;
            }
            erro("esperado ',' ou '}'");
        }
        sai();
        return objeto;
    }

    Json le_array() {
        entra();
        Json array = Json::array();
        ++posicao;   // '['
        pula_espacos();
        if (atual() == ']') {
            ++posicao;
            sai();
            return array;
        }
        while (true) {
            pula_espacos();
            array.adiciona(le_valor());
            pula_espacos();
            if (atual() == ',') {
                ++posicao;
                continue;
            }
            if (atual() == ']') {
                ++posicao;
                break;
            }
            erro("esperado ',' ou ']'");
        }
        sai();
        return array;
    }

    unsigned int le_hex4() {
        if (posicao + 4 > texto.size()) {
            erro("sequência \\u incompleta");
        }
        unsigned int valor = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = texto[posicao++];
            valor <<= 4;
            if (c >= '0' && c <= '9') valor |= static_cast<unsigned int>(c - '0');
            else if (c >= 'a' && c <= 'f') valor |= static_cast<unsigned int>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') valor |= static_cast<unsigned int>(c - 'A' + 10);
            else erro("dígito hexadecimal inválido em \\u");
        }
        return valor;
    }

    static void adiciona_utf8(std::string& saida, unsigned int ponto) {
        if (ponto < 0x80) {
            saida += static_cast<char>(ponto);
        } else if (ponto < 0x800) {
            saida += static_cast<char>(0xC0 | (ponto >> 6));
            saida += static_cast<char>(0x80 | (ponto & 0x3F));
        } else if (ponto < 0x10000) {
            saida += static_cast<char>(0xE0 | (ponto >> 12));
            saida += static_cast<char>(0x80 | ((ponto >> 6) & 0x3F));
            saida += static_cast<char>(0x80 | (ponto & 0x3F));
        } else {
            saida += static_cast<char>(0xF0 | (ponto >> 18));
            saida += static_cast<char>(0x80 | ((ponto >> 12) & 0x3F));
            saida += static_cast<char>(0x80 | ((ponto >> 6) & 0x3F));
            saida += static_cast<char>(0x80 | (ponto & 0x3F));
        }
    }

    std::string le_texto() {
        ++posicao;   // aspas de abertura
        std::string resultado;
        while (true) {
            if (fim()) {
                erro("texto sem aspas de fechamento");
            }
            const unsigned char c = static_cast<unsigned char>(texto[posicao]);
            if (c == '"') {
                ++posicao;
                return resultado;
            }
            if (c < 0x20) {
                erro("caractere de controle dentro do texto");
            }
            if (c != '\\') {
                resultado += static_cast<char>(c);
                ++posicao;
                continue;
            }
            ++posicao;   // '\'
            if (fim()) {
                erro("escape incompleto");
            }
            const char escape = texto[posicao++];
            switch (escape) {
                case '"': resultado += '"'; break;
                case '\\': resultado += '\\'; break;
                case '/': resultado += '/'; break;
                case 'b': resultado += '\b'; break;
                case 'f': resultado += '\f'; break;
                case 'n': resultado += '\n'; break;
                case 'r': resultado += '\r'; break;
                case 't': resultado += '\t'; break;
                case 'u': {
                    unsigned int ponto = le_hex4();
                    if (ponto >= 0xD800 && ponto <= 0xDBFF) {
                        if (posicao + 2 > texto.size() || texto.compare(posicao, 2, "\\u") != 0) {
                            erro("par substituto incompleto");
                        }
                        posicao += 2;
                        const unsigned int baixo = le_hex4();
                        if (baixo < 0xDC00 || baixo > 0xDFFF) {
                            erro("par substituto inválido");
                        }
                        ponto = 0x10000 + ((ponto - 0xD800) << 10) + (baixo - 0xDC00);
                    } else if (ponto >= 0xDC00 && ponto <= 0xDFFF) {
                        erro("par substituto inválido");
                    }
                    adiciona_utf8(resultado, ponto);
                    break;
                }
                default:
                    --posicao;
                    erro("sequência de escape inválida");
            }
        }
    }

    Json le_numero() {
        const std::size_t inicio = posicao;
        bool real = false;
        if (atual() == '-') {
            ++posicao;
        }
        if (atual() == '0') {
            ++posicao;
        } else if (eh_digito(atual())) {
            while (eh_digito(atual())) ++posicao;
        } else {
            erro("número inválido");
        }
        if (atual() == '.') {
            real = true;
            ++posicao;
            if (!eh_digito(atual())) {
                erro("número inválido: faltam dígitos após o ponto");
            }
            while (eh_digito(atual())) ++posicao;
        }
        if (atual() == 'e' || atual() == 'E') {
            real = true;
            ++posicao;
            if (atual() == '+' || atual() == '-') ++posicao;
            if (!eh_digito(atual())) {
                erro("número inválido: faltam dígitos no expoente");
            }
            while (eh_digito(atual())) ++posicao;
        }
        const std::string token = texto.substr(inicio, posicao - inicio);
        if (!real) {
            try {
                return Json(static_cast<long long>(std::stoll(token)));
            } catch (const std::out_of_range&) {
                // inteiro grande demais: tenta como real
            }
        }
        double valor = 0.0;
        if (!texto_para_real(token, valor) || !std::isfinite(valor)) {
            erro("número fora da faixa");
        }
        return Json(valor);
    }
};

// ############################## ESCRITA ##############################

std::string formata_real(double valor) {
    if (!std::isfinite(valor)) {
        throw ErroJson("número não finito não pode ser gravado em json");
    }
    std::string resultado;
    for (int precisao = 15; precisao <= 17; ++precisao) {
        std::ostringstream saida;
        saida.imbue(std::locale::classic());
        saida << std::setprecision(precisao) << valor;
        resultado = saida.str();
        double volta = 0.0;
        if (texto_para_real(resultado, volta) && volta == valor) {
            break;   // menor precisão que reproduz exatamente o mesmo double
        }
    }
    if (resultado.find_first_of(".eE") == std::string::npos) {
        resultado += ".0";   // mantém o tipo "real" ao ler de volta
    }
    return resultado;
}

void escreve_texto(const std::string& texto, std::string& saida) {
    saida += '"';
    for (const char caractere : texto) {
        const unsigned char c = static_cast<unsigned char>(caractere);
        switch (c) {
            case '"': saida += "\\\""; break;
            case '\\': saida += "\\\\"; break;
            case '\b': saida += "\\b"; break;
            case '\f': saida += "\\f"; break;
            case '\n': saida += "\\n"; break;
            case '\r': saida += "\\r"; break;
            case '\t': saida += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned int>(c));
                    saida += buffer;
                } else {
                    saida += static_cast<char>(c);
                }
        }
    }
    saida += '"';
}

// Arrays/objetos que só contêm valores simples são escritos em uma única linha.
bool eh_simples(const Json& valor) {   // só é chamada para arrays e objetos
    for (const Json& item : valor.itens()) {
        if (item.eh_array() || item.eh_objeto()) {
            return false;
        }
    }
    return true;
}

void escreve(const Json& valor, std::string& saida, int indentacao, int nivel) {
    const bool compacto = indentacao < 0;
    switch (valor.get_tipo()) {
        case Json::Tipo::NULO: saida += "null"; return;
        case Json::Tipo::BOOLEANO: saida += valor.como_booleano() ? "true" : "false"; return;
        case Json::Tipo::INTEIRO: saida += std::to_string(valor.como_inteiro()); return;
        case Json::Tipo::REAL: saida += formata_real(valor.como_real()); return;
        case Json::Tipo::TEXTO: escreve_texto(valor.como_texto(), saida); return;
        case Json::Tipo::ARRAY:
        case Json::Tipo::OBJETO: break;
    }

    const bool objeto = valor.eh_objeto();
    const std::size_t total = valor.tamanho();
    saida += objeto ? '{' : '[';
    if (total == 0) {
        saida += objeto ? '}' : ']';
        return;
    }
    const bool uma_linha = compacto || eh_simples(valor);
    for (std::size_t i = 0; i < total; ++i) {
        if (i > 0) {
            saida += (uma_linha && !compacto) ? ", " : ",";
        }
        if (!uma_linha) {
            saida += '\n';
            saida.append(static_cast<std::size_t>((nivel + 1) * indentacao), ' ');
        }
        if (objeto) {
            escreve_texto(valor.chaves()[i], saida);
            saida += compacto ? ":" : ": ";
        }
        escreve(valor.itens()[i], saida, indentacao, nivel + 1);
    }
    if (!uma_linha) {
        saida += '\n';
        saida.append(static_cast<std::size_t>(nivel * indentacao), ' ');
    }
    saida += objeto ? '}' : ']';
}

}  // namespace

Json Json::de_texto(const std::string& texto_json) {
    Interpretador interpretador(texto_json);
    return interpretador.interpreta();
}

std::string Json::para_texto(int indentacao) const {
    std::string resultado;
    escreve(*this, resultado, indentacao, 0);
    return resultado;
}
