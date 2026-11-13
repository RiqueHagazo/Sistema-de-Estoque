#include "datahora.hpp"

#include <cstdio>
#include <ctime>
#include <tuple>

#include "erros.hpp"
#include "texto.hpp"

namespace {

std::tm hora_local() {
    const std::time_t agora = std::time(nullptr);
    const std::tm* local = std::localtime(&agora);
    if (local == nullptr) {
        throw ErroEstoque("não foi possível obter a data/hora do sistema");
    }
    return *local;
}

// converte `quantidade` dígitos de `texto`, a partir de `inicio`
int le_numero(const std::string& texto, std::size_t inicio, std::size_t quantidade, const char* mensagem) {
    int valor = 0;
    for (std::size_t i = inicio; i < inicio + quantidade; ++i) {
        if (texto[i] < '0' || texto[i] > '9') {
            throw ErroValidacao(mensagem);
        }
        valor = valor * 10 + (texto[i] - '0');
    }
    return valor;
}

}  // namespace

// ############################## DATA ##############################

bool Data::eh_valida(int ano, int mes, int dia) {
    if (ano < ANO_MINIMO || ano > ANO_MAXIMO || mes < 1 || mes > 12 || dia < 1) {
        return false;
    }
    static const int DIAS_NO_MES[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool bissexto = (ano % 4 == 0 && ano % 100 != 0) || ano % 400 == 0;
    const int limite = (mes == 2 && bissexto) ? 29 : DIAS_NO_MES[mes - 1];
    return dia <= limite;
}

Data::Data(int ano, int mes, int dia) : ano(ano), mes(mes), dia(dia) {
    if (!eh_valida(ano, mes, dia)) {
        throw ErroValidacao("data inexistente: " + std::to_string(dia) + "/" + std::to_string(mes) + "/" +
                            std::to_string(ano));
    }
}

Data Data::de_texto(const std::string& texto) {
    const std::string limpo = remove_espacos(texto);
    const char* MENSAGEM = "data inválida: use dd/mm/aaaa";
    if (limpo.size() == 10 && limpo[4] == '-' && limpo[7] == '-') {
        return Data(le_numero(limpo, 0, 4, MENSAGEM), le_numero(limpo, 5, 2, MENSAGEM),
                    le_numero(limpo, 8, 2, MENSAGEM));
    }
    if (limpo.size() == 10 && limpo[2] == '/' && limpo[5] == '/') {
        return Data(le_numero(limpo, 6, 4, MENSAGEM), le_numero(limpo, 3, 2, MENSAGEM),
                    le_numero(limpo, 0, 2, MENSAGEM));
    }
    throw ErroValidacao(MENSAGEM);
}

Data Data::hoje() {
    const std::tm local = hora_local();
    return Data(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
}

int Data::get_ano() const { return ano; }
int Data::get_mes() const { return mes; }
int Data::get_dia() const { return dia; }

std::string Data::para_texto() const {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", ano, mes, dia);
    return buffer;
}

std::string Data::formatada() const {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", dia, mes, ano);
    return buffer;
}

// Algoritmo "days from civil" (Howard Hinnant): dias desde 01/01/1970.
int Data::dias_desde_epoca() const {
    const int a = ano - (mes <= 2 ? 1 : 0);
    const int era = a / 400;   // a >= 1900, nunca negativo
    const unsigned int ano_da_era = static_cast<unsigned int>(a - era * 400);
    const unsigned int mes_ajustado = static_cast<unsigned int>(mes > 2 ? mes - 3 : mes + 9);
    const unsigned int dia_do_ano = (153 * mes_ajustado + 2) / 5 + static_cast<unsigned int>(dia) - 1;
    const unsigned int dia_da_era = ano_da_era * 365 + ano_da_era / 4 - ano_da_era / 100 + dia_do_ano;
    return era * 146097 + static_cast<int>(dia_da_era) - 719468;
}

int Data::dias_ate(const Data& outra) const {
    return outra.dias_desde_epoca() - dias_desde_epoca();
}

bool Data::operator==(const Data& outra) const {
    return std::tie(ano, mes, dia) == std::tie(outra.ano, outra.mes, outra.dia);
}
bool Data::operator!=(const Data& outra) const { return !(*this == outra); }
bool Data::operator<(const Data& outra) const {
    return std::tie(ano, mes, dia) < std::tie(outra.ano, outra.mes, outra.dia);
}
bool Data::operator<=(const Data& outra) const { return !(outra < *this); }
bool Data::operator>(const Data& outra) const { return outra < *this; }
bool Data::operator>=(const Data& outra) const { return !(*this < outra); }

std::string formata_data(const Data& data) {
    return data.formatada();
}

// ############################## HORARIO ##############################

Horario::Horario(int hora, int minuto, int segundo) : hora(hora), minuto(minuto), segundo(segundo) {
    if (hora < 0 || hora > 23 || minuto < 0 || minuto > 59 || segundo < 0 || segundo > 59) {
        throw ErroValidacao("horário inválido: use hh:mm:ss");
    }
}

Horario Horario::de_texto(const std::string& texto) {
    const std::string limpo = remove_espacos(texto);
    const char* MENSAGEM = "horário inválido: use hh:mm:ss";
    if (limpo.size() == 5 && limpo[2] == ':') {
        return Horario(le_numero(limpo, 0, 2, MENSAGEM), le_numero(limpo, 3, 2, MENSAGEM), 0);
    }
    if (limpo.size() == 8 && limpo[2] == ':' && limpo[5] == ':') {
        return Horario(le_numero(limpo, 0, 2, MENSAGEM), le_numero(limpo, 3, 2, MENSAGEM),
                       le_numero(limpo, 6, 2, MENSAGEM));
    }
    throw ErroValidacao(MENSAGEM);
}

Horario Horario::agora() {
    const std::tm local = hora_local();
    return Horario(local.tm_hour, local.tm_min, local.tm_sec > 59 ? 59 : local.tm_sec);   // evita o "segundo bissexto" (60)
}

int Horario::get_hora() const { return hora; }
int Horario::get_minuto() const { return minuto; }
int Horario::get_segundo() const { return segundo; }

std::string Horario::para_texto() const {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hora, minuto, segundo);
    return buffer;
}

bool Horario::operator==(const Horario& outro) const {
    return std::tie(hora, minuto, segundo) == std::tie(outro.hora, outro.minuto, outro.segundo);
}
bool Horario::operator!=(const Horario& outro) const { return !(*this == outro); }
bool Horario::operator<(const Horario& outro) const {
    return std::tie(hora, minuto, segundo) < std::tie(outro.hora, outro.minuto, outro.segundo);
}

// ############################## RELOGIO ##############################

Data RelogioSistema::hoje() const { return Data::hoje(); }
Horario RelogioSistema::agora() const { return Horario::agora(); }
