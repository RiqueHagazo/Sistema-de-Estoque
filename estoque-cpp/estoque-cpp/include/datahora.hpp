#ifndef DATAHORA_HPP
#define DATAHORA_HPP

#include <string>

/**
 * @file datahora.hpp
 * @brief Data, horário e relógio (injetável) usados nas vendas e na validade dos produtos.
 */

/**
 * @brief Data do calendário gregoriano, sempre válida (ano 1900..9999).
 *
 * Substitui os campos `date` do banco. Todo construtor valida a data, então
 * não existe objeto Data inválido (31/02, mês 13, ...).
 */
class Data {
public:
    static constexpr int ANO_MINIMO = 1900;
    static constexpr int ANO_MAXIMO = 9999;

    /** @throws ErroValidacao se a data não existir. */
    Data(int ano, int mes, int dia);

    /**
     * @brief Interpreta "aaaa-mm-dd" (formato do `<input type="date">`) ou "dd/mm/aaaa".
     * @throws ErroValidacao se o texto não estiver em um dos formatos ou a data não existir.
     */
    static Data de_texto(const std::string& texto);

    /** @brief Data atual do sistema. */
    static Data hoje();

    /** @brief Informa se a combinação ano/mês/dia existe no calendário. */
    static bool eh_valida(int ano, int mes, int dia);

    int get_ano() const;
    int get_mes() const;
    int get_dia() const;

    /** @brief Formato de armazenamento: "aaaa-mm-dd". */
    std::string para_texto() const;
    /** @brief Formato de exibição (filtro `formata_data` do original): "dd/mm/aaaa". */
    std::string formatada() const;

    /** @brief Dias entre esta data e `outra` (positivo se `outra` é posterior). */
    int dias_ate(const Data& outra) const;

    bool operator==(const Data& outra) const;
    bool operator!=(const Data& outra) const;
    bool operator<(const Data& outra) const;
    bool operator<=(const Data& outra) const;
    bool operator>(const Data& outra) const;
    bool operator>=(const Data& outra) const;

private:
    int dias_desde_epoca() const;

    int ano;
    int mes;
    int dia;
};

/** @brief Equivalente ao filtro `formata_data` do template original ("dd/mm/aaaa"). */
std::string formata_data(const Data& data);

/** @brief Horário do dia (00:00:00 a 23:59:59). */
class Horario {
public:
    /** @throws ErroValidacao se algum componente estiver fora da faixa. */
    Horario(int hora, int minuto, int segundo = 0);

    /** @brief Interpreta "hh:mm" ou "hh:mm:ss". @throws ErroValidacao se inválido. */
    static Horario de_texto(const std::string& texto);

    /** @brief Horário atual do sistema. */
    static Horario agora();

    int get_hora() const;
    int get_minuto() const;
    int get_segundo() const;

    /** @brief "hh:mm:ss". */
    std::string para_texto() const;

    bool operator==(const Horario& outro) const;
    bool operator!=(const Horario& outro) const;
    bool operator<(const Horario& outro) const;

private:
    int hora;
    int minuto;
    int segundo;
};

/**
 * @brief Fonte de data e hora (interface).
 *
 * As classes que precisam do "agora" recebem um Relogio em vez de consultar o
 * sistema diretamente; os testes usam um relógio fixo e o resultado é previsível.
 */
class Relogio {
public:
    virtual ~Relogio() = default;
    virtual Data hoje() const = 0;
    virtual Horario agora() const = 0;
};

/** @brief Relógio real, baseado na hora local do computador. */
class RelogioSistema : public Relogio {
public:
    Data hoje() const override;
    Horario agora() const override;
};

#endif
