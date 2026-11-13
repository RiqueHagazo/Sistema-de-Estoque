#include "auxiliar.hpp"
#include "datahora.hpp"
#include "erros.hpp"
#include "teste.hpp"

// ---------------------------------------------------------------- Data

TESTE(data_aceita_datas_validas) {
    const Data data(2026, 10, 30);
    VERIFICA_IGUAL(data.get_ano(), 2026);
    VERIFICA_IGUAL(data.get_mes(), 10);
    VERIFICA_IGUAL(data.get_dia(), 30);
}

TESTE(data_trata_anos_bissextos) {
    VERIFICA(Data::eh_valida(2024, 2, 29));
    VERIFICA(!Data::eh_valida(2025, 2, 29));
    VERIFICA(Data::eh_valida(2000, 2, 29));    // divisível por 400
    VERIFICA(!Data::eh_valida(1900, 2, 29));   // divisível por 100, mas não por 400
    VERIFICA(!Data::eh_valida(2100, 2, 29));
}

TESTE(data_rejeita_datas_inexistentes) {
    VERIFICA_EXCECAO(Data(2026, 2, 30), ErroValidacao);
    VERIFICA_EXCECAO(Data(2026, 4, 31), ErroValidacao);
    VERIFICA_EXCECAO(Data(2026, 13, 1), ErroValidacao);
    VERIFICA_EXCECAO(Data(2026, 0, 10), ErroValidacao);
    VERIFICA_EXCECAO(Data(2026, 5, 0), ErroValidacao);
    VERIFICA_EXCECAO(Data(1899, 12, 31), ErroValidacao);
    VERIFICA_EXCECAO(Data(10000, 1, 1), ErroValidacao);
    VERIFICA_EXCECAO_MSG(Data(2026, 2, 30), ErroValidacao, "30/2/2026");
}

TESTE(data_interpreta_os_dois_formatos) {
    VERIFICA(Data::de_texto("2026-10-30") == Data(2026, 10, 30));
    VERIFICA(Data::de_texto("30/10/2026") == Data(2026, 10, 30));
    VERIFICA(Data::de_texto("  05/01/2027 ") == Data(2027, 1, 5));
}

TESTE(data_rejeita_textos_invalidos) {
    const char* invalidos[] = {"", "abc", "2026-13-01", "31/04/2026", "2026/10/10", "10-10-2026", "2026-1-1",
                               "30/10/26", "aaaa-mm-dd", "2026-10-3x", "1899-01-01"};
    for (const char* texto : invalidos) {
        bool lancou = false;
        try {
            Data::de_texto(texto);
        } catch (const ErroValidacao&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar: [" << texto << "]");
    }
}

TESTE(data_formata_para_armazenar_e_para_exibir) {
    const Data data(2026, 3, 7);
    VERIFICA_IGUAL(data.para_texto(), std::string("2026-03-07"));
    VERIFICA_IGUAL(data.formatada(), std::string("07/03/2026"));
    VERIFICA_IGUAL(formata_data(data), std::string("07/03/2026"));
}

TESTE(data_operadores_de_comparacao) {
    const Data a(2026, 10, 30);
    const Data b(2026, 11, 1);
    const Data c(2027, 1, 1);
    VERIFICA(a < b);
    VERIFICA(b < c);
    VERIFICA(a <= a);
    VERIFICA(c > a);
    VERIFICA(c >= c);
    VERIFICA(a == Data(2026, 10, 30));
    VERIFICA(a != b);
    VERIFICA(!(b < a));
}

TESTE(data_calcula_dias_entre_datas) {
    VERIFICA_IGUAL(Data(2026, 10, 30).dias_ate(Data(2026, 11, 6)), 7);
    VERIFICA_IGUAL(Data(2024, 2, 28).dias_ate(Data(2024, 3, 1)), 2);     // ano bissexto
    VERIFICA_IGUAL(Data(2025, 2, 28).dias_ate(Data(2025, 3, 1)), 1);
    VERIFICA_IGUAL(Data(2025, 12, 31).dias_ate(Data(2026, 1, 1)), 1);
    VERIFICA_IGUAL(Data(2026, 10, 30).dias_ate(Data(2026, 10, 30)), 0);
    VERIFICA_IGUAL(Data(2026, 11, 6).dias_ate(Data(2026, 10, 30)), -7);
    VERIFICA_IGUAL(Data(2026, 1, 1).dias_ate(Data(2027, 1, 1)), 365);
    VERIFICA_IGUAL(Data(2024, 1, 1).dias_ate(Data(2025, 1, 1)), 366);
}

TESTE(data_hoje_devolve_data_valida) {
    const Data hoje = Data::hoje();
    VERIFICA(hoje.get_ano() >= 2024);
    VERIFICA(Data::eh_valida(hoje.get_ano(), hoje.get_mes(), hoje.get_dia()));
}

// ---------------------------------------------------------------- Horario

TESTE(horario_aceita_e_formata) {
    const Horario horario(9, 5, 3);
    VERIFICA_IGUAL(horario.para_texto(), std::string("09:05:03"));
    VERIFICA_IGUAL(horario.get_hora(), 9);
    VERIFICA_IGUAL(horario.get_minuto(), 5);
    VERIFICA_IGUAL(horario.get_segundo(), 3);
    VERIFICA_IGUAL(Horario(23, 59).para_texto(), std::string("23:59:00"));
}

TESTE(horario_rejeita_valores_fora_da_faixa) {
    VERIFICA_EXCECAO(Horario(24, 0, 0), ErroValidacao);
    VERIFICA_EXCECAO(Horario(-1, 0, 0), ErroValidacao);
    VERIFICA_EXCECAO(Horario(10, 60, 0), ErroValidacao);
    VERIFICA_EXCECAO(Horario(10, 0, 60), ErroValidacao);
}

TESTE(horario_interpreta_textos) {
    VERIFICA(Horario::de_texto("14:03:22") == Horario(14, 3, 22));
    VERIFICA(Horario::de_texto("14:03") == Horario(14, 3, 0));
    VERIFICA_EXCECAO(Horario::de_texto("14h03"), ErroValidacao);
    VERIFICA_EXCECAO(Horario::de_texto("25:00:00"), ErroValidacao);
    VERIFICA_EXCECAO(Horario::de_texto(""), ErroValidacao);
    VERIFICA_EXCECAO(Horario::de_texto("aa:bb:cc"), ErroValidacao);
}

TESTE(horario_comparacao) {
    VERIFICA(Horario(8, 0, 0) < Horario(8, 0, 1));
    VERIFICA(Horario(8, 59, 59) < Horario(9, 0, 0));
    VERIFICA(!(Horario(9, 0, 0) < Horario(9, 0, 0)));
    VERIFICA(Horario(9, 0, 0) != Horario(9, 0, 1));
}

TESTE(horario_agora_devolve_horario_valido) {
    const Horario agora = Horario::agora();
    VERIFICA(agora.get_hora() >= 0 && agora.get_hora() <= 23);
}

// ---------------------------------------------------------------- Relogio (polimorfismo)

TESTE(relogio_fixo_e_substituivel_pelo_relogio_do_sistema) {
    RelogioFixo fixo(Data(2026, 10, 30), Horario(14, 3, 22));
    const Relogio& relogio = fixo;   // uso polimórfico, via interface
    VERIFICA(relogio.hoje() == Data(2026, 10, 30));
    VERIFICA(relogio.agora() == Horario(14, 3, 22));
    fixo.define(Data(2027, 1, 1), Horario(0, 0, 0));
    VERIFICA(relogio.hoje() == Data(2027, 1, 1));

    RelogioSistema real;
    const Relogio& relogio_real = real;
    VERIFICA(relogio_real.hoje().get_ano() >= 2024);
    VERIFICA(relogio_real.agora().get_hora() >= 0);
}
