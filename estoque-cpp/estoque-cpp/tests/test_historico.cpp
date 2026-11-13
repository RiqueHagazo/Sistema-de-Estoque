#include <filesystem>

#include "auxiliar.hpp"
#include "erros.hpp"
#include "historico.hpp"
#include "teste.hpp"

namespace {
RelogioFixo relogio_de_teste() { return RelogioFixo(Data(2026, 10, 30), Horario(14, 3, 22)); }
}  // namespace

TESTE(historico_registra_linha_com_data_hora_usuario_e_operacao) {
    DiretorioTemporario base;
    const RelogioFixo relogio = relogio_de_teste();
    Historico historico(base.arquivo("historico.log"), relogio);
    VERIFICA(historico.registra("maria", "VENDA", "produto 3, quantidade 2"));
    VERIFICA_IGUAL(le_arquivo(base.arquivo("historico.log")),
                   std::string("2026-10-30 14:03:22 | maria | VENDA | produto 3, quantidade 2\n"));
}

TESTE(historico_acumula_as_operacoes_em_ordem) {
    DiretorioTemporario base;
    RelogioFixo relogio = relogio_de_teste();
    Historico historico(base.arquivo("historico.log"), relogio);
    historico.registra("ana", "LOGIN", "");
    relogio.define(Data(2026, 10, 30), Horario(14, 5, 0));
    historico.registra("ana", "LOGOUT", "");
    const auto linhas = historico.ultimas_linhas(10);
    VERIFICA_IGUAL(linhas.size(), 2u);
    VERIFICA_IGUAL(linhas[0], std::string("2026-10-30 14:03:22 | ana | LOGIN | "));
    VERIFICA_IGUAL(linhas[1], std::string("2026-10-30 14:05:00 | ana | LOGOUT | "));
}

TESTE(historico_ultimas_linhas_respeita_o_limite) {
    DiretorioTemporario base;
    const RelogioFixo relogio = relogio_de_teste();
    Historico historico(base.arquivo("historico.log"), relogio);
    for (int i = 1; i <= 5; ++i) {
        historico.registra("u", "OP", std::to_string(i));
    }
    const auto ultimas = historico.ultimas_linhas(2);
    VERIFICA_IGUAL(ultimas.size(), 2u);
    VERIFICA_IGUAL(ultimas[0], std::string("2026-10-30 14:03:22 | u | OP | 4"));
    VERIFICA_IGUAL(ultimas[1], std::string("2026-10-30 14:03:22 | u | OP | 5"));
    VERIFICA_IGUAL(historico.ultimas_linhas(100).size(), 5u);
    VERIFICA_IGUAL(historico.ultimas_linhas(0).size(), 0u);
}

TESTE(historico_sem_arquivo_devolve_lista_vazia) {
    DiretorioTemporario base;
    const RelogioFixo relogio = relogio_de_teste();
    const Historico historico(base.arquivo("nao_existe.log"), relogio);
    VERIFICA(historico.ultimas_linhas(10).empty());
}

TESTE(historico_mantem_uma_linha_por_operacao) {
    DiretorioTemporario base;
    const RelogioFixo relogio = relogio_de_teste();
    Historico historico(base.arquivo("historico.log"), relogio);
    historico.registra("u", "OP", "primeira\nsegunda\r\nterceira");
    VERIFICA_IGUAL(historico.ultimas_linhas(10).size(), 1u);
}

TESTE(historico_falha_ao_gravar_devolve_falso_sem_lancar_excecao) {
    DiretorioTemporario base;
    const RelogioFixo relogio = relogio_de_teste();
    Historico historico(base.arquivo("pasta_inexistente/historico.log"), relogio);
    bool resultado = true;
    VERIFICA_NAO_LANCA(resultado = historico.registra("u", "OP", "x"));
    VERIFICA(!resultado);
}

TESTE(historico_ilegivel_lanca_erro_de_persistencia) {
    DiretorioTemporario base;
    std::filesystem::create_directories(base.arquivo("historico.log"));   // pasta no lugar do arquivo
    const RelogioFixo relogio = relogio_de_teste();
    const Historico historico(base.arquivo("historico.log"), relogio);
    VERIFICA_EXCECAO(historico.ultimas_linhas(5), ErroPersistencia);
}
