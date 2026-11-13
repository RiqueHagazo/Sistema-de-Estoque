#include <sstream>
#include <vector>

#include "aplicacao.hpp"
#include "auxiliar.hpp"
#include "servico_estoque.hpp"
#include "teste.hpp"
#include "texto.hpp"

namespace {

using Linhas = std::vector<std::string>;

Linhas operator+(Linhas a, const Linhas& b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

const Linhas PRIMEIRO_ACESSO = {"admin", "admin123", "admin123"};
const Linhas LOGIN_ADMIN = {"admin", "admin123"};
const Linhas LOGIN_MARIA = {"maria", "segredo"};
const Linhas SAIR = {"0", ""};   // "0" sai do menu principal; linha em branco encerra no login

bool contem(const std::string& texto, const std::string& trecho) {
    return texto.find(trecho) != std::string::npos;
}

/// Serviço sobre uma pasta temporária, com relógio fixo (30/10/2026 14:03:22), mais um executor de sessões simuladas.
struct Sessao {
    DiretorioTemporario pasta;
    BancoDado banco;
    RelogioFixo relogio;
    Historico historico;
    ServicoEstoque servico;

    Sessao()
        : banco(pasta.get_caminho()),
          relogio(Data(2026, 10, 30), Horario(14, 3, 22)),
          historico(banco.caminho_arquivo("historico.log"), relogio),
          servico(banco, historico, relogio, 10) {}

    /// Executa a aplicação "digitando" as linhas dadas; devolve tudo o que foi impresso.
    std::string executa(const Linhas& linhas, int* codigo_saida = nullptr, const std::string& final_da_entrada = "") {
        std::string texto;
        for (const std::string& linha : linhas) texto += linha + "\n";
        texto += final_da_entrada;
        std::istringstream entrada(texto);
        std::ostringstream saida;
        Aplicacao aplicacao(servico, entrada, saida);
        const int codigo = aplicacao.executar();
        if (codigo_saida != nullptr) *codigo_saida = codigo;
        return saida.str();
    }
};

/// Sessão com um administrador, um operador, duas categorias e três produtos já cadastrados.
struct SessaoPronta : Sessao {
    std::shared_ptr<Usuario> admin;
    std::shared_ptr<Usuario> operador;
    std::shared_ptr<Categoria> mercearia;
    std::shared_ptr<Categoria> laticinios;
    std::shared_ptr<Produto> arroz;     // código 1: não perecível, 20 un., R$ 19,99
    std::shared_ptr<Produto> leite;     // código 2: vence em 15/11/2026, 12 un., R$ 4,50
    std::shared_ptr<Produto> iogurte;   // código 3: venceu em 25/10/2026, 3 un., R$ 3,20

    SessaoPronta() {
        admin = servico.cadastrar_primeiro_administrador("admin", "admin123");
        operador = servico.cadastrar_usuario(*admin, "maria", "segredo", Usuario::GRUPO_OPERADOR);
        mercearia = servico.cadastrar_categoria(*admin, "mercearia");
        laticinios = servico.cadastrar_categoria(*admin, "laticínios");
        arroz = servico.cadastrar_produto(*admin, mercearia->get_id(), "arroz", 20, 19.99, Data(2026, 10, 1),
                                          std::nullopt, "pacote 5 kg");
        leite = servico.cadastrar_produto(*admin, laticinios->get_id(), "leite", 12, 4.5, Data(2026, 10, 1),
                                          Data(2026, 11, 15), "caixa 1 L");
        iogurte = servico.cadastrar_produto(*admin, laticinios->get_id(), "iogurte", 3, 3.2, Data(2026, 10, 1),
                                            Data(2026, 10, 25), "bandeja");
    }

    int estoque(int id) { return servico.busca_produto_id(id)->get_quantidade_produto(); }
};

}  // namespace

// ---------------------------------------------------------------- primeiro acesso e login

TESTE(aplicacao_primeiro_acesso_cria_o_administrador_e_segue_para_o_login) {
    Sessao s;
    int codigo = -1;
    const std::string saida = s.executa(PRIMEIRO_ACESSO + LOGIN_ADMIN + SAIR, &codigo);
    VERIFICA_IGUAL(codigo, 0);
    VERIFICA(contem(saida, "primeiro acesso"));
    VERIFICA(contem(saida, ">> cadastrado com sucesso"));
    VERIFICA(contem(saida, "bem vindo(a) admin!"));
    VERIFICA(contem(saida, "até logo!"));
    VERIFICA(s.servico.existe_usuario_cadastrado());
}

TESTE(aplicacao_primeiro_acesso_nao_aparece_se_ja_ha_usuarios) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + SAIR);
    VERIFICA(!contem(saida, "primeiro acesso"));
}

TESTE(aplicacao_primeiro_acesso_exige_senhas_iguais_e_validas) {
    Sessao s;
    const Linhas tentativas = {"admin", "abcd", "abce",    // senhas diferentes
                               "admin", "abc", "abc",      // senha curta demais
                               "admin", "abcd", "abcd"};   // agora vale
    const std::string saida = s.executa(tentativas + Linhas{"admin", "abcd"} + SAIR);
    VERIFICA(contem(saida, ">> as senhas não conferem"));
    VERIFICA(contem(saida, ">> erro: a senha deve ter entre 4 e 128 caracteres"));
    VERIFICA(contem(saida, "bem vindo(a) admin!"));
}

TESTE(aplicacao_login_invalido_mostra_a_mensagem_do_original_e_pede_de_novo) {
    SessaoPronta s;
    const std::string saida = s.executa(Linhas{"maria", "errada", "fantasma", "x"} + LOGIN_MARIA + SAIR);
    VERIFICA(contem(saida, ">> login/senha inválidos"));
    VERIFICA(contem(saida, "bem vindo(a) maria!"));
}

TESTE(aplicacao_nunca_mostra_a_senha_digitada) {
    SessaoPronta s;
    const std::string saida = s.executa(Linhas{"maria", "senha-errada-123"} + LOGIN_MARIA + SAIR);
    VERIFICA(!contem(saida, "senha-errada-123"));
    VERIFICA(!contem(saida, "segredo"));
    VERIFICA(contem(saida, "senha: (oculta)"));
}

TESTE(aplicacao_aceita_arquivos_de_entrada_com_fim_de_linha_do_windows) {
    SessaoPronta s;
    std::string texto = "admin\r\nadmin123\r\n0\r\n\r\n";
    std::istringstream entrada(texto);
    std::ostringstream saida;
    Aplicacao aplicacao(s.servico, entrada, saida);
    VERIFICA_IGUAL(aplicacao.executar(), 0);
    VERIFICA(contem(saida.str(), "bem vindo(a) admin!"));
}

// ---------------------------------------------------------------- permissões (polimorfismo na prática)

TESTE(aplicacao_operador_nao_ve_as_telas_de_administrador) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_MARIA + Linhas{"5", "6", "9"} + SAIR);
    VERIFICA(!contem(saida, "5) registro"));
    VERIFICA(!contem(saida, "6) administrativo"));
    VERIFICA(contem(saida, ">> somente administradores"));
    VERIFICA(contem(saida, ">> opção inválida"));
}

TESTE(aplicacao_administrador_ve_registro_e_administrativo) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + SAIR);
    VERIFICA(contem(saida, "5) registro"));
    VERIFICA(contem(saida, "6) administrativo"));
}

// ---------------------------------------------------------------- categorias e produtos

TESTE(aplicacao_cadastra_categoria_e_produto_e_lista) {
    Sessao s;
    const Linhas roteiro = PRIMEIRO_ACESSO + LOGIN_ADMIN +
                           Linhas{"2", "1", "bebidas", "0"} +                                   // categoria > adicionar
                           Linhas{"1", "1", "1", "suco", "7", "6,50", "garrafa", "15/10/2026", "20/12/2026", "0"} + SAIR;
    const std::string saida = s.executa(roteiro);
    VERIFICA(contem(saida, ">> salvo"));
    VERIFICA(!contem(saida, "opção inválida"));
    VERIFICA(contem(saida, "R$6.50"));
    VERIFICA(contem(saida, "20/12/2026"));
    VERIFICA(contem(saida, "bebidas"));

    const auto produtos = s.servico.lista_produtos();
    VERIFICA_IGUAL(produtos.size(), 1u);
    VERIFICA_IGUAL(produtos[0]->get_nome_produto(), std::string("suco"));
    VERIFICA_IGUAL(produtos[0]->get_quantidade_produto(), 7);
    VERIFICA(produtos[0]->get_validade_produto().value() == Data(2026, 12, 20));
    VERIFICA(produtos[0]->get_chegada_produto() == Data(2026, 10, 15));
}

TESTE(aplicacao_data_de_chegada_em_branco_usa_hoje_e_validade_em_branco_nao_tem_validade) {
    SessaoPronta s;
    const Linhas roteiro = LOGIN_ADMIN + Linhas{"1", "1", "1", "sal", "9", "2", "", "", "", "0"} + SAIR;
    s.executa(roteiro);
    const auto sal = s.servico.busca_produto("sal");
    VERIFICA_IGUAL(sal.size(), 1u);
    VERIFICA(sal[0]->get_chegada_produto() == Data(2026, 10, 30));   // "hoje" segundo o relógio
    VERIFICA(!sal[0]->get_validade_produto().has_value());
}

TESTE(aplicacao_produto_com_categoria_invalida_usa_a_mensagem_do_original) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"1", "1", "99", "x", "1", "1", "", "", "", "0"} + SAIR);
    VERIFICA(contem(saida, ">> erro: categoria inválida"));
    const std::string saida2 = s.executa(LOGIN_ADMIN + Linhas{"1", "1", "abc", "0"} + SAIR);   // nem é número
    VERIFICA(contem(saida2, ">> erro: categoria inválida"));
    VERIFICA_IGUAL(s.servico.lista_produtos().size(), 3u);
}

TESTE(aplicacao_produto_sem_categorias_cadastradas_orienta_o_usuario) {
    Sessao s;
    const std::string saida = s.executa(PRIMEIRO_ACESSO + LOGIN_ADMIN + Linhas{"1", "1", "0"} + SAIR);
    VERIFICA(contem(saida, "cadastre uma categoria antes de adicionar produtos"));
}

TESTE(aplicacao_entradas_invalidas_nao_derrubam_o_programa) {
    SessaoPronta s;
    int codigo = -1;
    // cada formulário para no primeiro campo inválido, então só consome as linhas até ele
    const Linhas roteiro = LOGIN_ADMIN + Linhas{"1",
                                                 "1", "1", "x", "abc",                              // quantidade inválida
                                                 "1", "1", "x", "5", "doze reais",                  // valor inválido
                                                 "1", "1", "x", "5", "2", "", "31/02/2026",         // data inexistente
                                                 "1", "1", "x", "5", "2", "", "01/10/2026", "01/09/2026",   // validade antes da chegada
                                                 "1", "1", "x", "-5", "2", "", "", "",              // quantidade negativa
                                                 "0"} +
                           SAIR;
    const std::string saida = s.executa(roteiro, &codigo);
    VERIFICA_IGUAL(codigo, 0);
    VERIFICA(contem(saida, "entrada inválida em 'quantidade'"));
    VERIFICA(contem(saida, "entrada inválida em 'valor'"));
    VERIFICA(contem(saida, "data inexistente: 31/2/2026"));
    VERIFICA(contem(saida, "anterior à data de chegada"));
    VERIFICA(contem(saida, "quantidade inválida: use um valor entre 0"));
    VERIFICA(!contem(saida, "opção inválida"));              // nenhuma linha do roteiro sobrou
    VERIFICA_IGUAL(s.servico.lista_produtos().size(), 3u);   // nada foi cadastrado
}

TESTE(aplicacao_pesquisa_produto_por_nome) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"1", "2", "ARROZ", "2", "feijao", "0"} + SAIR);
    VERIFICA(contem(saida, "resultado da pesquisa"));
    VERIFICA(contem(saida, "nenhum produto encontrado"));
}

TESTE(aplicacao_exclusao_de_produto_pede_confirmacao) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"1", "3", "3", "n", "3", "3", "s", "0"} + SAIR);
    VERIFICA(contem(saida, ">> operação cancelada"));
    VERIFICA(contem(saida, ">> produto excluido"));
    VERIFICA(!contem(saida, "opção inválida"));
    VERIFICA_IGUAL(s.servico.lista_produtos().size(), 2u);
    VERIFICA(s.servico.busca_produto("iogurte").empty());
}

TESTE(aplicacao_exclusao_de_produto_inexistente_ou_com_vendas_mostra_erro) {
    SessaoPronta s;
    s.servico.realiza_venda(*s.operador, s.arroz->get_id(), 1);
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"1", "3", "77", "3", "1", "s", "3", "x", "0"} + SAIR);
    VERIFICA(contem(saida, ">> erro: produto não encontrado"));
    VERIFICA(contem(saida, ">> erro: o produto possui vendas registradas e não pode ser excluído"));
    VERIFICA(contem(saida, "entrada inválida em 'código'"));
    VERIFICA_IGUAL(s.servico.lista_produtos().size(), 3u);
}

TESTE(aplicacao_repoe_estoque) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"1", "4", "1", "30", "4", "1", "0", "0"} + SAIR);
    VERIFICA(contem(saida, ">> estoque atualizado"));
    VERIFICA(!contem(saida, "opção inválida"));
    VERIFICA(contem(saida, "a quantidade de reposição deve ser maior que zero"));
    VERIFICA_IGUAL(s.estoque(s.arroz->get_id()), 50);
}

TESTE(aplicacao_cadastra_categoria_e_recusa_repetida) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_MARIA + Linhas{"2", "1", "limpeza", "1", "LIMPEZA", "1", "", "0"} + SAIR);
    VERIFICA(contem(saida, ">> salvo"));
    VERIFICA(!contem(saida, "opção inválida"));
    VERIFICA(contem(saida, ">> erro: categoria já existe"));
    VERIFICA(contem(saida, "erro: nome da categoria não pode ficar em branco"));
    VERIFICA_IGUAL(s.servico.lista_categorias().size(), 3u);
}

// ---------------------------------------------------------------- vendas (retirada)

TESTE(aplicacao_administrador_vende_e_ve_o_registro_em_seguida) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"3", "1", "2"} + SAIR);
    VERIFICA(contem(saida, "Código do produto     : 1"));
    VERIFICA(contem(saida, "Quantidade em estoque : 20"));
    VERIFICA(contem(saida, ">> realizada com sucesso"));
    VERIFICA(contem(saida, "valor da venda: R$39.98"));
    VERIFICA(contem(saida, "registro de vendas"));   // como no original, o administrador vai para o registro
    VERIFICA(contem(saida, "30/10/2026"));
    VERIFICA(contem(saida, "14:03:22"));
    VERIFICA_IGUAL(s.estoque(s.arroz->get_id()), 18);
}

TESTE(aplicacao_operador_vende_sem_ver_o_registro) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_MARIA + Linhas{"3", "2", "4"} + SAIR);
    VERIFICA(contem(saida, ">> realizada com sucesso"));
    VERIFICA(contem(saida, "valor da venda: R$18.00"));
    VERIFICA(!contem(saida, "registro de vendas"));
    VERIFICA_IGUAL(s.estoque(s.leite->get_id()), 8);
}

TESTE(aplicacao_venda_acima_do_estoque_mostra_a_mensagem_do_original) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_MARIA + Linhas{"3", "3", "4", "3", "3", "0", "3", "3", "-1"} + SAIR);
    VERIFICA(contem(saida, ">> erro: produtos insuficientes ou igual a zero"));
    VERIFICA(!contem(saida, "opção inválida"));
    VERIFICA_IGUAL(s.estoque(s.iogurte->get_id()), 3);
    VERIFICA(s.servico.registro_vendas(*s.admin).empty());
}

TESTE(aplicacao_venda_de_produto_inexistente_ou_codigo_invalido) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_MARIA + Linhas{"3", "99", "3", "abc"} + SAIR);
    VERIFICA(contem(saida, ">> erro: produto não encontrado"));
    VERIFICA(contem(saida, "entrada inválida em 'código'"));
}

// ---------------------------------------------------------------- registro de vendas

TESTE(aplicacao_registro_pesquisa_por_periodo_e_mostra_o_valor_total) {
    SessaoPronta s;
    s.servico.realiza_venda(*s.operador, s.arroz->get_id(), 2);                    // 39,98 em 30/10/2026
    s.relogio.define(Data(2026, 11, 5), Horario(9, 0, 0));
    s.servico.realiza_venda(*s.operador, s.leite->get_id(), 4);                    // 18,00 em 05/11/2026
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"5", "1", "01/11/2026", "30/11/2026", "0"} + SAIR);
    VERIFICA(contem(saida, "valor por periodo: R$18.00"));
}

TESTE(aplicacao_registro_recusa_periodo_invertido_e_datas_invalidas) {
    SessaoPronta s;
    const Linhas roteiro = LOGIN_ADMIN + Linhas{"5",
                                                 "1", "30/11/2026", "01/11/2026",   // período invertido
                                                 "1", "",                           // data em branco
                                                 "1", "2026-13-01",                 // mês 13
                                                 "0"} +
                           SAIR;
    const std::string saida = s.executa(roteiro);
    VERIFICA(contem(saida, ">> erro: período inválido: a data inicial é posterior à data final"));
    VERIFICA(contem(saida, ">> erro: data inválida: use dd/mm/aaaa"));
    VERIFICA(contem(saida, ">> erro: data inexistente: 1/13/2026"));
    VERIFICA(!contem(saida, "opção inválida"));
}

TESTE(aplicacao_registro_sem_vendas) {
    SessaoPronta s;
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"5", "0"} + SAIR);
    VERIFICA(contem(saida, "nenhuma venda registrada"));
}

// ---------------------------------------------------------------- administrativo

TESTE(aplicacao_administrativo_cadastra_e_exclui_usuarios) {
    SessaoPronta s;
    const Linhas roteiro = LOGIN_ADMIN + Linhas{"6",
                                                 "1", "joao", "senha1", "0",        // cadastra
                                                 "1", "joao", "outra", "1",         // repetido
                                                 "1", "x", "abc", "0",              // senha curta
                                                 "1", "y", "senha1", "7",           // grupo inexistente
                                                 "2", "joao", "s",                  // exclui
                                                 "2", "maria", "n",                 // desiste
                                                 "2", "ninguem", "s",               // não existe
                                                 "2", "admin", "s",                 // o próprio usuário
                                                 "0"} + SAIR;
    const std::string saida = s.executa(roteiro);
    VERIFICA(contem(saida, ">> cadastrado com sucesso"));
    VERIFICA(!contem(saida, "opção inválida"));
    VERIFICA(contem(saida, ">> erro: usuário já existe"));
    VERIFICA(contem(saida, ">> erro: a senha deve ter entre 4 e 128 caracteres"));
    VERIFICA(contem(saida, ">> erro: grupo inválido"));
    VERIFICA(contem(saida, ">> excluido"));
    VERIFICA(contem(saida, ">> operação cancelada"));
    VERIFICA(contem(saida, ">> erro: usuário não encontrado"));
    VERIFICA(contem(saida, ">> erro: não é possível excluir o próprio usuário"));
    const auto usuarios = s.servico.lista_usuarios(*s.admin);
    VERIFICA_IGUAL(usuarios.size(), 2u);   // admin e maria continuam; joao foi excluído
}

TESTE(aplicacao_administrativo_mostra_o_historico) {
    SessaoPronta s;
    s.servico.realiza_venda(*s.operador, s.arroz->get_id(), 1);
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"6", "3", "0"} + SAIR);
    VERIFICA(contem(saida, "histórico de operações"));
    VERIFICA(contem(saida, "| maria | VENDA | venda 1 de 1 x arroz, total 19.99"));
    VERIFICA(contem(saida, "| admin | LOGIN | administrador"));
}

// ---------------------------------------------------------------- alertas

TESTE(aplicacao_alertas_mostram_vencidos_a_vencer_e_estoque_baixo) {
    SessaoPronta s;
    s.servico.cadastrar_produto(*s.admin, s.laticinios->get_id(), "queijo", 8, 9.0, Data(2026, 10, 1), Data(2026, 11, 3), "");
    const std::string saida = s.executa(LOGIN_MARIA + Linhas{"4"} + SAIR);
    const auto posicao = [&](const std::string& trecho) { return saida.find(trecho); };
    VERIFICA(posicao("produtos vencidos") < posicao("produtos que vencem em até 7 dias"));
    VERIFICA(posicao("produtos que vencem em até 7 dias") < posicao("produtos com estoque de até 5 unidades"));
    VERIFICA(contem(saida, "iogurte"));   // vencido e com estoque baixo
    VERIFICA(contem(saida, "queijo"));    // vence em 4 dias
}

TESTE(aplicacao_alertas_sem_ocorrencias) {
    Sessao s;
    const std::string saida = s.executa(PRIMEIRO_ACESSO + LOGIN_ADMIN + Linhas{"4"} + SAIR);
    VERIFICA(contem(saida, "nenhum produto vencido"));
    VERIFICA(contem(saida, "nenhum produto perto do vencimento"));
    VERIFICA(contem(saida, "nenhum produto com estoque baixo"));
}

// ---------------------------------------------------------------- robustez

TESTE(aplicacao_encerra_sem_erro_quando_a_entrada_acaba) {
    SessaoPronta s;
    int codigo = -1;
    s.executa(Linhas{}, &codigo);                                       // entrada vazia
    VERIFICA_IGUAL(codigo, 0);
    s.executa(Linhas{"admin"}, &codigo);                                // acaba no pedido de senha
    VERIFICA_IGUAL(codigo, 0);
    s.executa(LOGIN_ADMIN + Linhas{"1"}, &codigo);                      // acaba dentro do menu de produtos
    VERIFICA_IGUAL(codigo, 0);
    s.executa(LOGIN_ADMIN + Linhas{"1", "1", "1", "nome"}, &codigo);    // acaba no meio de um formulário
    VERIFICA_IGUAL(codigo, 0);
    s.executa(Linhas{}, &codigo, "admin");                              // última linha sem quebra de linha
    VERIFICA_IGUAL(codigo, 0);
}

TESTE(aplicacao_continua_funcionando_com_arquivo_de_dados_corrompido) {
    SessaoPronta s;
    escreve_arquivo(s.pasta.arquivo("produto.json"), "{ corrompido");
    int codigo = -1;
    // "1" (produtos) falha e volta ao home; "2" (categoria) funciona; "0" volta ao home
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"1", "2", "0"} + SAIR, &codigo);
    VERIFICA_IGUAL(codigo, 0);
    VERIFICA(contem(saida, ">> erro: tabela 'produto'"));
    VERIFICA(contem(saida, "id da categoria"));   // o resto do sistema segue utilizável
    VERIFICA(!contem(saida, "opção inválida"));
    VERIFICA_IGUAL(le_arquivo(s.pasta.arquivo("produto.json")), std::string("{ corrompido"));   // e o arquivo não foi sobrescrito
}

TESTE(aplicacao_tabelas_ficam_alinhadas_mesmo_com_acentos) {
    SessaoPronta s;
    s.servico.cadastrar_produto(*s.admin, s.laticinios->get_id(), "açúcar refinado", 9, 4.0, Data(2026, 10, 1), std::nullopt, "pacote de 1 kg");
    const std::string saida = s.executa(LOGIN_ADMIN + Linhas{"1", "0"} + SAIR);

    // em toda linha da tabela de produtos, a primeira coluna termina na mesma posição (contada em caracteres)
    std::istringstream linhas(saida);
    std::string linha;
    std::vector<std::size_t> posicoes;
    bool dentro_da_tabela = false;
    while (std::getline(linhas, linha)) {
        if (linha.rfind("código | nome", 0) == 0) dentro_da_tabela = true;
        if (dentro_da_tabela && linha.empty()) break;
        if (dentro_da_tabela && linha.find(" | ") != std::string::npos) {
            const std::string coluna_nome_e_resto = linha.substr(linha.find(" | ") + 3);
            posicoes.push_back(tamanho_utf8(linha.substr(0, linha.find(" | "))) +
                               tamanho_utf8(coluna_nome_e_resto.substr(0, coluna_nome_e_resto.find(" | "))));
        }
    }
    VERIFICA(posicoes.size() >= 5u);
    for (const std::size_t posicao : posicoes) {
        VERIFICA_IGUAL(posicao, posicoes[0]);
    }
}

TESTE(aplicacao_registra_login_e_logout_no_historico) {
    SessaoPronta s;
    s.executa(LOGIN_MARIA + SAIR);
    const auto linhas = s.servico.ultimas_operacoes(*s.admin, 100);
    std::string texto;
    for (const std::string& linha : linhas) texto += linha + "\n";
    VERIFICA(contem(texto, "| maria | LOGIN |"));
    VERIFICA(contem(texto, "| maria | LOGOUT |"));
}

TESTE(aplicacao_sessao_simulada_completa_por_arquivo_de_texto) {
    // o mesmo roteiro que `make demo` executa, a partir de um arquivo com as "teclas digitadas"
    Sessao s;
    std::string roteiro;
    for (const std::string& linha : PRIMEIRO_ACESSO + LOGIN_ADMIN + Linhas{"2", "1", "bebidas", "0", "1", "1", "1", "suco", "10", "5", "", "", "", "0", "3", "1", "4"} + SAIR)
        roteiro += linha + "\n";
    escreve_arquivo(s.pasta.arquivo("entrada.txt"), roteiro);

    std::istringstream entrada(le_arquivo(s.pasta.arquivo("entrada.txt")));
    std::ostringstream saida;
    Aplicacao aplicacao(s.servico, entrada, saida);
    VERIFICA_IGUAL(aplicacao.executar(), 0);
    VERIFICA(contem(saida.str(), ">> realizada com sucesso"));
    VERIFICA_IGUAL(s.servico.busca_produto("suco")[0]->get_quantidade_produto(), 6);
}

TESTE(aplicacao_submenus_rejeitam_opcao_invalida_sem_sair) {
    SessaoPronta s;
    const Linhas roteiro = LOGIN_ADMIN + Linhas{"1", "x", "0",      // produtos
                                                 "2", "9", "0",      // categoria
                                                 "5", "7", "0",      // registro
                                                 "6", "8", "0"} +    // administrativo
                           SAIR;
    int codigo = -1;
    const std::string saida = s.executa(roteiro, &codigo);
    VERIFICA_IGUAL(codigo, 0);
    std::size_t ocorrencias = 0;
    for (std::size_t pos = saida.find(">> opção inválida"); pos != std::string::npos;
         pos = saida.find(">> opção inválida", pos + 1)) {
        ++ocorrencias;
    }
    VERIFICA_IGUAL(ocorrencias, 4u);
}

TESTE(aplicacao_historico_vazio_quando_nao_ha_registros) {
    DiretorioTemporario pasta;
    BancoDado banco(pasta.get_caminho());
    RelogioFixo relogio(Data(2026, 10, 30), Horario(14, 3, 22));
    Historico historico(pasta.arquivo("sem_pasta/historico.log"), relogio);   // nada consegue ser registrado
    ServicoEstoque servico(banco, historico, relogio, 10);
    servico.cadastrar_primeiro_administrador("admin", "admin123");
    std::istringstream entrada("admin\nadmin123\n6\n3\n0\n0\n\n");
    std::ostringstream saida;
    Aplicacao aplicacao(servico, entrada, saida);
    VERIFICA_IGUAL(aplicacao.executar(), 0);
    VERIFICA(contem(saida.str(), "nenhuma operação registrada"));
}
