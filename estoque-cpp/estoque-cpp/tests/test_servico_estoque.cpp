#include <filesystem>
#include <optional>

#include "auxiliar.hpp"
#include "erros.hpp"
#include "servico_estoque.hpp"
#include "teste.hpp"

namespace {

/// Sistema pronto: um administrador, um operador e uma categoria. O relógio marca 30/10/2026 14:03:22.
struct Cenario {
    DiretorioTemporario pasta;
    BancoDado banco;
    RelogioFixo relogio;
    Historico historico;
    ServicoEstoque servico;
    std::shared_ptr<Usuario> admin;
    std::shared_ptr<Usuario> operador;
    std::shared_ptr<Categoria> categoria;

    Cenario()
        : banco(pasta.get_caminho()),
          relogio(Data(2026, 10, 30), Horario(14, 3, 22)),
          historico(banco.caminho_arquivo("historico.log"), relogio),
          servico(banco, historico, relogio, 10) {   // custo baixo de senha: testes rápidos
        admin = servico.cadastrar_primeiro_administrador("admin", "admin123");
        operador = servico.cadastrar_usuario(*admin, "maria", "segredo", Usuario::GRUPO_OPERADOR);
        categoria = servico.cadastrar_categoria(*admin, "mercearia");
    }

    std::shared_ptr<Produto> produto(const std::string& nome, int quantidade, double valor,
                                     const std::optional<Data>& validade = std::nullopt) {
        return servico.cadastrar_produto(*admin, categoria->get_id(), nome, quantidade, valor, Data(2026, 10, 1),
                                         validade, "");
    }

    std::string historico_completo() {
        std::string texto;
        for (const std::string& linha : servico.ultimas_operacoes(*admin, 1000)) texto += linha + "\n";
        return texto;
    }
};

bool contem(const std::string& texto, const std::string& trecho) {
    return texto.find(trecho) != std::string::npos;
}

}  // namespace

// ---------------------------------------------------------------- primeiro acesso e login

TESTE(servico_primeiro_administrador_so_pode_ser_criado_uma_vez) {
    DiretorioTemporario pasta;
    BancoDado banco(pasta.get_caminho());
    RelogioFixo relogio(Data(2026, 10, 30), Horario(8, 0, 0));
    Historico historico(banco.caminho_arquivo("historico.log"), relogio);
    ServicoEstoque servico(banco, historico, relogio, 10);

    VERIFICA(!servico.existe_usuario_cadastrado());
    const auto admin = servico.cadastrar_primeiro_administrador("root", "senha1");
    VERIFICA(admin->pode_administrar());
    VERIFICA(servico.existe_usuario_cadastrado());
    VERIFICA_EXCECAO_MSG(servico.cadastrar_primeiro_administrador("outro", "senha2"), ErroRegraNegocio, "já existem usuários");
}

TESTE(servico_primeiro_administrador_valida_nome_e_senha) {
    DiretorioTemporario pasta;
    BancoDado banco(pasta.get_caminho());
    RelogioFixo relogio(Data(2026, 10, 30), Horario(8, 0, 0));
    Historico historico(banco.caminho_arquivo("historico.log"), relogio);
    ServicoEstoque servico(banco, historico, relogio, 10);
    VERIFICA_EXCECAO(servico.cadastrar_primeiro_administrador("", "senha1"), ErroValidacao);
    VERIFICA_EXCECAO(servico.cadastrar_primeiro_administrador("root", "abc"), ErroValidacao);   // curta demais
    VERIFICA(!servico.existe_usuario_cadastrado());
}

TESTE(servico_login_com_dados_corretos) {
    Cenario c;
    const auto usuario = c.servico.login("maria", "segredo");
    VERIFICA_IGUAL(usuario->get_usuario(), std::string("maria"));
    VERIFICA(!usuario->pode_administrar());
    VERIFICA(c.servico.login("admin", "admin123")->pode_administrar());
    VERIFICA_IGUAL(c.servico.login("  maria  ", "segredo")->get_usuario(), std::string("maria"));   // espaços nas pontas são ignorados
}

TESTE(servico_login_invalido_lanca_erro_de_autenticacao_com_a_mensagem_do_original) {
    Cenario c;
    VERIFICA_EXCECAO_MSG(c.servico.login("maria", "errada"), ErroAutenticacao, "login/senha inválidos");
    VERIFICA_EXCECAO_MSG(c.servico.login("fantasma", "segredo"), ErroAutenticacao, "login/senha inválidos");
    VERIFICA_EXCECAO(c.servico.login("", ""), ErroAutenticacao);
    VERIFICA_EXCECAO(c.servico.login("maria", ""), ErroAutenticacao);
}

TESTE(servico_registra_login_logout_e_tentativas_falhas_sem_a_senha) {
    Cenario c;
    c.servico.login("maria", "segredo");
    c.servico.logout(*c.operador);
    VERIFICA_EXCECAO(c.servico.login("maria", "senha-errada-secreta"), ErroAutenticacao);
    const std::string texto = c.historico_completo();
    VERIFICA(contem(texto, "| maria | LOGIN | operador"));
    VERIFICA(contem(texto, "| maria | LOGOUT |"));
    VERIFICA(contem(texto, "| maria | LOGIN_FALHOU |"));
    VERIFICA(!contem(texto, "senha-errada-secreta"));
    VERIFICA(!contem(texto, "segredo"));
}

// ---------------------------------------------------------------- usuários e permissões

TESTE(servico_so_administrador_cadastra_usuarios) {
    Cenario c;
    VERIFICA_EXCECAO_MSG(c.servico.cadastrar_usuario(*c.operador, "novo", "senha1", 0), ErroPermissao, "somente administradores");
    const auto novo = c.servico.cadastrar_usuario(*c.admin, "novo", "senha1", Usuario::GRUPO_ADMINISTRADOR);
    VERIFICA(novo->pode_administrar());
    VERIFICA(c.servico.login("novo", "senha1")->pode_administrar());
}

TESTE(servico_cadastro_de_usuario_valida_os_dados) {
    Cenario c;
    VERIFICA_EXCECAO(c.servico.cadastrar_usuario(*c.admin, "x", "abc", 0), ErroValidacao);                      // senha curta
    VERIFICA_EXCECAO(c.servico.cadastrar_usuario(*c.admin, "x", std::string(129, 'a'), 0), ErroValidacao);       // senha longa
    VERIFICA_EXCECAO(c.servico.cadastrar_usuario(*c.admin, "", "senha1", 0), ErroValidacao);                    // sem nome
    VERIFICA_EXCECAO(c.servico.cadastrar_usuario(*c.admin, "x", "senha1", 5), ErroValidacao);                   // grupo inexistente
    VERIFICA_EXCECAO_MSG(c.servico.cadastrar_usuario(*c.admin, "maria", "senha1", 0), ErroIntegridade, "usuário já existe");
}

TESTE(servico_so_administrador_lista_e_exclui_usuarios) {
    Cenario c;
    VERIFICA_EXCECAO(c.servico.lista_usuarios(*c.operador), ErroPermissao);
    VERIFICA_EXCECAO(c.servico.excluir_usuario(*c.operador, "admin"), ErroPermissao);
    VERIFICA_IGUAL(c.servico.lista_usuarios(*c.admin).size(), 2u);
    c.servico.excluir_usuario(*c.admin, "maria");
    VERIFICA_IGUAL(c.servico.lista_usuarios(*c.admin).size(), 1u);
    VERIFICA_EXCECAO(c.servico.login("maria", "segredo"), ErroAutenticacao);
}

TESTE(servico_nao_exclui_usuario_inexistente_nem_o_proprio_autor) {
    Cenario c;
    VERIFICA_EXCECAO(c.servico.excluir_usuario(*c.admin, "ninguem"), ErroNaoEncontrado);
    VERIFICA_EXCECAO_MSG(c.servico.excluir_usuario(*c.admin, "admin"), ErroRegraNegocio, "próprio usuário");
    VERIFICA_IGUAL(c.servico.lista_usuarios(*c.admin).size(), 2u);
}

TESTE(servico_nao_deixa_o_sistema_sem_administrador) {
    Cenario c;
    // outra sessão apaga o "admin"; quem ainda está com o objeto antigo não pode apagar o único administrador restante
    const auto segundo = c.servico.cadastrar_usuario(*c.admin, "admin2", "senha1", Usuario::GRUPO_ADMINISTRADOR);
    UsuarioDAO outra_sessao(c.banco);
    outra_sessao.excluir_usuario("admin");
    VERIFICA_EXCECAO_MSG(c.servico.excluir_usuario(*c.admin, "admin2"), ErroRegraNegocio, "único administrador");
    VERIFICA(c.servico.login("admin2", "senha1")->pode_administrar());
}

// ---------------------------------------------------------------- categorias e produtos

TESTE(servico_cadastra_categoria_e_nao_aceita_repetida) {
    Cenario c;
    const auto nova = c.servico.cadastrar_categoria(*c.operador, "limpeza");   // qualquer usuário logado pode, como no original
    VERIFICA(nova->foi_persistida());
    VERIFICA_IGUAL(c.servico.lista_categorias().size(), 2u);
    VERIFICA_EXCECAO(c.servico.cadastrar_categoria(*c.operador, "LIMPEZA"), ErroIntegridade);
    VERIFICA_EXCECAO(c.servico.cadastrar_categoria(*c.operador, "  "), ErroValidacao);
}

TESTE(servico_cadastra_produto_perecivel_e_nao_perecivel) {
    Cenario c;
    const auto leite = c.produto("leite", 10, 4.5, Data(2026, 11, 20));
    const auto arroz = c.produto("arroz", 20, 19.99);
    VERIFICA_IGUAL(leite->get_tipo(), std::string("perecivel"));
    VERIFICA_IGUAL(arroz->get_tipo(), std::string("nao perecivel"));
    VERIFICA_IGUAL(c.servico.lista_produtos().size(), 2u);
    VERIFICA_IGUAL(c.servico.busca_produto("ARROZ").size(), 1u);
    VERIFICA_IGUAL(c.servico.busca_produto_id(leite->get_id())->get_nome_produto(), std::string("leite"));
}

TESTE(servico_cadastro_de_produto_com_categoria_invalida_usa_a_mensagem_do_original) {
    Cenario c;
    VERIFICA_EXCECAO_MSG(c.servico.cadastrar_produto(*c.admin, 999, "x", 1, 1.0, Data(2026, 10, 1), std::nullopt, ""),
                         ErroValidacao, "categoria inválida");
    VERIFICA_EXCECAO(c.servico.cadastrar_produto(*c.admin, 0, "x", 1, 1.0, Data(2026, 10, 1), std::nullopt, ""), ErroValidacao);
    VERIFICA(c.servico.lista_produtos().empty());
}

TESTE(servico_cadastro_de_produto_valida_os_dados) {
    Cenario c;
    const int id = c.categoria->get_id();
    const Data chegada(2026, 10, 1);
    VERIFICA_EXCECAO(c.servico.cadastrar_produto(*c.admin, id, "", 1, 1.0, chegada, std::nullopt, ""), ErroValidacao);
    VERIFICA_EXCECAO(c.servico.cadastrar_produto(*c.admin, id, "x", -1, 1.0, chegada, std::nullopt, ""), ErroValidacao);
    VERIFICA_EXCECAO(c.servico.cadastrar_produto(*c.admin, id, "x", 1, -1.0, chegada, std::nullopt, ""), ErroValidacao);
    VERIFICA_EXCECAO(c.servico.cadastrar_produto(*c.admin, id, "x", 1, 1.0, chegada, Data(2026, 9, 1), ""), ErroValidacao);
    VERIFICA(c.servico.lista_produtos().empty());
}

TESTE(servico_exclui_produto) {
    Cenario c;
    const auto produto = c.produto("leite", 10, 4.5);
    c.servico.excluir_produto(*c.operador, produto->get_id());
    VERIFICA(c.servico.lista_produtos().empty());
    VERIFICA_EXCECAO(c.servico.excluir_produto(*c.operador, produto->get_id()), ErroNaoEncontrado);
}

TESTE(servico_nao_exclui_produto_com_vendas) {
    Cenario c;
    const auto produto = c.produto("leite", 10, 4.5);
    c.servico.realiza_venda(*c.operador, produto->get_id(), 1);
    VERIFICA_EXCECAO(c.servico.excluir_produto(*c.admin, produto->get_id()), ErroIntegridade);
    VERIFICA_IGUAL(c.servico.lista_produtos().size(), 1u);
}

TESTE(servico_repoe_estoque) {
    Cenario c;
    const auto produto = c.produto("leite", 10, 4.5);
    c.servico.repoe_estoque(*c.operador, produto->get_id(), 15);
    VERIFICA_IGUAL(c.servico.busca_produto_id(produto->get_id())->get_quantidade_produto(), 25);
    VERIFICA_EXCECAO(c.servico.repoe_estoque(*c.operador, produto->get_id(), 0), ErroValidacao);
    VERIFICA_EXCECAO(c.servico.repoe_estoque(*c.operador, produto->get_id(), Produto::QUANTIDADE_MAXIMA), ErroRegraNegocio);
    VERIFICA_EXCECAO(c.servico.repoe_estoque(*c.operador, 99, 5), ErroNaoEncontrado);
    VERIFICA_IGUAL(c.servico.busca_produto_id(produto->get_id())->get_quantidade_produto(), 25);
}

// ---------------------------------------------------------------- vendas

TESTE(servico_venda_desconta_estoque_calcula_valor_e_usa_a_hora_do_relogio) {
    Cenario c;
    const auto produto = c.produto("arroz", 10, 19.99);
    const auto venda = c.servico.realiza_venda(*c.operador, produto->get_id(), 3);

    VERIFICA_IGUAL(venda->get_vendedor(), std::string("maria"));
    VERIFICA_IGUAL(venda->get_quantidade_venda(), 3);
    VERIFICA(venda->get_valor_venda() == 59.97);
    VERIFICA(venda->get_data_venda() == Data(2026, 10, 30));
    VERIFICA(venda->get_horario_venda() == Horario(14, 3, 22));
    VERIFICA(venda->foi_persistida());
    VERIFICA_IGUAL(c.servico.busca_produto_id(produto->get_id())->get_quantidade_produto(), 7);
}

TESTE(servico_venda_pode_zerar_o_estoque_mas_nao_ultrapassa_lo) {
    Cenario c;
    const auto produto = c.produto("leite", 5, 4.0);
    c.servico.realiza_venda(*c.operador, produto->get_id(), 5);
    VERIFICA_IGUAL(c.servico.busca_produto_id(produto->get_id())->get_quantidade_produto(), 0);
    VERIFICA_EXCECAO(c.servico.realiza_venda(*c.operador, produto->get_id(), 1), ErroEstoqueInsuficiente);
}

TESTE(servico_venda_recusada_nao_altera_nada) {
    Cenario c;
    const auto produto = c.produto("leite", 5, 4.0);
    VERIFICA_EXCECAO_MSG(c.servico.realiza_venda(*c.operador, produto->get_id(), 6), ErroEstoqueInsuficiente,
                         "produtos insuficientes ou igual a zero");
    VERIFICA_EXCECAO(c.servico.realiza_venda(*c.operador, produto->get_id(), 0), ErroEstoqueInsuficiente);
    VERIFICA_EXCECAO(c.servico.realiza_venda(*c.operador, produto->get_id(), -3), ErroEstoqueInsuficiente);
    VERIFICA_IGUAL(c.servico.busca_produto_id(produto->get_id())->get_quantidade_produto(), 5);
    VERIFICA(c.servico.registro_vendas(*c.admin).empty());
}

TESTE(servico_venda_de_produto_inexistente_lanca_erro) {
    Cenario c;
    VERIFICA_EXCECAO(c.servico.realiza_venda(*c.operador, 42, 1), ErroNaoEncontrado);
}

TESTE(servico_venda_devolve_o_estoque_se_nao_conseguir_gravar_a_venda) {
    Cenario c;
    const auto produto = c.produto("leite", 10, 4.0);
    std::filesystem::create_directories(c.pasta.arquivo("vendas.json"));   // quebra a tabela de vendas
    VERIFICA_EXCECAO(c.servico.realiza_venda(*c.operador, produto->get_id(), 4), ErroPersistencia);
    VERIFICA_IGUAL(c.servico.busca_produto_id(produto->get_id())->get_quantidade_produto(), 10);   // nada "sumiu"
}

TESTE(servico_vendas_em_horarios_diferentes_ficam_registradas) {
    Cenario c;
    const auto produto = c.produto("leite", 100, 2.0);
    c.servico.realiza_venda(*c.operador, produto->get_id(), 1);
    c.relogio.define(Data(2026, 11, 2), Horario(9, 30, 0));
    c.servico.realiza_venda(*c.admin, produto->get_id(), 2);

    const auto todas = c.servico.registro_vendas(*c.admin);
    VERIFICA_IGUAL(todas.size(), 2u);
    VERIFICA_IGUAL(todas[0]->get_vendedor(), std::string("maria"));
    VERIFICA_IGUAL(todas[1]->get_vendedor(), std::string("admin"));
    VERIFICA(todas[1]->get_data_venda() == Data(2026, 11, 2));

    VERIFICA_IGUAL(c.servico.pesquisa_periodo(*c.admin, Data(2026, 11, 1), Data(2026, 11, 30)).size(), 1u);
    VERIFICA_PROXIMO(c.servico.valor_por_periodo(*c.admin, Data(2026, 11, 1), Data(2026, 11, 30)), 4.0, 1e-9);
    VERIFICA_PROXIMO(c.servico.valor_por_periodo(*c.admin, Data(2026, 10, 1), Data(2026, 11, 30)), 6.0, 1e-9);
}

TESTE(servico_relatorios_de_vendas_sao_so_para_administradores) {
    Cenario c;
    const Data inicio(2026, 1, 1);
    const Data fim(2026, 12, 31);
    VERIFICA_EXCECAO_MSG(c.servico.registro_vendas(*c.operador), ErroPermissao, "somente administradores");
    VERIFICA_EXCECAO(c.servico.pesquisa_periodo(*c.operador, inicio, fim), ErroPermissao);
    VERIFICA_EXCECAO(c.servico.valor_por_periodo(*c.operador, inicio, fim), ErroPermissao);
    VERIFICA_EXCECAO(c.servico.ultimas_operacoes(*c.operador, 10), ErroPermissao);
    VERIFICA_NAO_LANCA(c.servico.registro_vendas(*c.admin));
}

// ---------------------------------------------------------------- alertas

TESTE(servico_alerta_de_produtos_vencidos) {
    Cenario c;                                                    // hoje: 30/10/2026
    c.produto("leite", 3, 4.0, Data(2026, 10, 29));               // venceu ontem
    c.produto("iogurte", 2, 3.0, Data(2026, 10, 30));             // vence hoje: ainda vale
    c.produto("pao", 0, 1.0, Data(2026, 10, 20));                 // vencido, mas sem estoque: não alerta
    c.produto("queijo", 1, 9.0, Data(2026, 10, 15));              // venceu há mais tempo
    c.produto("arroz", 9, 20.0);                                  // não perecível: nunca vence
    const auto vencidos = c.servico.produtos_vencidos();
    VERIFICA_IGUAL(vencidos.size(), 2u);
    VERIFICA_IGUAL(vencidos[0]->get_nome_produto(), std::string("queijo"));   // do mais antigo para o mais recente
    VERIFICA_IGUAL(vencidos[1]->get_nome_produto(), std::string("leite"));
}

TESTE(servico_alerta_de_produtos_a_vencer) {
    Cenario c;
    c.produto("leite", 3, 4.0, Data(2026, 10, 29));    // já vencido: fora
    c.produto("iogurte", 2, 3.0, Data(2026, 10, 30));  // 0 dias
    c.produto("queijo", 4, 9.0, Data(2026, 11, 6));    // 7 dias
    c.produto("manteiga", 4, 9.0, Data(2026, 11, 7));  // 8 dias: fora
    c.produto("arroz", 9, 20.0);                       // sem validade: fora
    const auto a_vencer = c.servico.produtos_a_vencer(7);
    VERIFICA_IGUAL(a_vencer.size(), 2u);
    VERIFICA_IGUAL(a_vencer[0]->get_nome_produto(), std::string("iogurte"));
    VERIFICA_IGUAL(a_vencer[1]->get_nome_produto(), std::string("queijo"));
    VERIFICA_IGUAL(c.servico.produtos_a_vencer(6).size(), 1u);
    VERIFICA_IGUAL(c.servico.produtos_a_vencer(0).size(), 1u);
    VERIFICA_EXCECAO(c.servico.produtos_a_vencer(-1), ErroValidacao);
}

TESTE(servico_alerta_de_estoque_baixo) {
    Cenario c;
    c.produto("sal", 5, 2.0);
    c.produto("acucar", 6, 4.0);
    c.produto("farinha", 0, 5.0);
    c.produto("leite", 2, 4.0, Data(2026, 12, 1));
    const auto baixos = c.servico.produtos_estoque_baixo(ServicoEstoque::ESTOQUE_MINIMO);
    VERIFICA_IGUAL(baixos.size(), 3u);
    VERIFICA_IGUAL(baixos[0]->get_nome_produto(), std::string("farinha"));   // do mais escasso
    VERIFICA_IGUAL(baixos[1]->get_nome_produto(), std::string("leite"));
    VERIFICA_IGUAL(baixos[2]->get_nome_produto(), std::string("sal"));
    VERIFICA_IGUAL(c.servico.produtos_estoque_baixo(0).size(), 1u);
    VERIFICA_EXCECAO(c.servico.produtos_estoque_baixo(-1), ErroValidacao);
}

// ---------------------------------------------------------------- histórico

TESTE(servico_registra_as_operacoes_no_historico) {
    Cenario c;
    const auto produto = c.produto("leite", 10, 4.0);
    c.servico.realiza_venda(*c.operador, produto->get_id(), 2);
    c.servico.repoe_estoque(*c.admin, produto->get_id(), 5);
    c.servico.excluir_usuario(*c.admin, "maria");
    const std::string texto = c.historico_completo();
    VERIFICA(contem(texto, "| sistema | PRIMEIRO_ACESSO |"));
    VERIFICA(contem(texto, "| admin | CADASTRO_USUARIO | usuário maria (operador)"));
    VERIFICA(contem(texto, "| admin | CADASTRO_CATEGORIA | categoria 1 (mercearia)"));
    VERIFICA(contem(texto, "| admin | CADASTRO_PRODUTO | produto 1 (leite), quantidade 10"));
    VERIFICA(contem(texto, "| maria | VENDA | venda 1 de 2 x leite, total 8.00"));
    VERIFICA(contem(texto, "| admin | REPOSICAO | produto 1 (leite), +5 (estoque: 13)"));
    VERIFICA(contem(texto, "| admin | EXCLUSAO_USUARIO | usuário maria (operador)"));
    VERIFICA(contem(texto, "2026-10-30 14:03:22"));
}

TESTE(servico_continua_funcionando_se_o_historico_nao_puder_ser_gravado) {
    DiretorioTemporario pasta;
    BancoDado banco(pasta.get_caminho());
    RelogioFixo relogio(Data(2026, 10, 30), Horario(8, 0, 0));
    Historico historico(pasta.arquivo("pasta_que_nao_existe/historico.log"), relogio);   // gravação sempre falha
    ServicoEstoque servico(banco, historico, relogio, 10);
    VERIFICA_NAO_LANCA(servico.cadastrar_primeiro_administrador("admin", "admin123"));
    VERIFICA(servico.existe_usuario_cadastrado());
}
