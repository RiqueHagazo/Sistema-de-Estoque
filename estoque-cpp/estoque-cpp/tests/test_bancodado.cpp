#include <filesystem>

#include "auxiliar.hpp"
#include "bancodado.hpp"
#include "erros.hpp"
#include "teste.hpp"

namespace fs = std::filesystem;

namespace {
Json linha_com_nome(const std::string& nome) {
    Json linha = Json::objeto();
    linha.define("nome", nome);
    return linha;
}
}  // namespace

// ---------------------------------------------------------------- BancoDado

TESTE(banco_cria_a_pasta_de_dados_se_nao_existir) {
    DiretorioTemporario base;
    const std::string pasta = base.arquivo("a/b/dados");
    BancoDado banco(pasta);
    VERIFICA(fs::is_directory(pasta));
    VERIFICA_IGUAL(banco.get_diretorio(), pasta);
}

TESTE(banco_rejeita_pasta_em_branco) {
    VERIFICA_EXCECAO(BancoDado(""), ErroValidacao);
    VERIFICA_EXCECAO(BancoDado("   "), ErroValidacao);
}

TESTE(banco_rejeita_pasta_que_na_verdade_e_um_arquivo) {
    DiretorioTemporario base;
    escreve_arquivo(base.arquivo("arquivo"), "x");
    VERIFICA_EXCECAO(BancoDado(base.arquivo("arquivo")), ErroPersistencia);
    VERIFICA_EXCECAO(BancoDado(base.arquivo("arquivo/filha")), ErroPersistencia);
}

TESTE(banco_so_aceita_nomes_de_tabela_seguros) {
    DiretorioTemporario base;
    BancoDado banco(base.get_caminho());
    const char* invalidos[] = {"", "../fora", "a/b", "Maiuscula", "com espaco", "a.b", "tabela1", "..", "a\\b"};
    for (const char* nome : invalidos) {
        bool lancou = false;
        try {
            banco.get_tabela(nome);
        } catch (const ErroValidacao&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar o nome: [" << nome << "]");
    }
    VERIFICA_NAO_LANCA(banco.get_tabela("usuarios"));
    VERIFICA_NAO_LANCA(banco.get_tabela("tabela_com_underscore"));
}

TESTE(banco_cada_tabela_vira_um_arquivo_json) {
    DiretorioTemporario base;
    BancoDado banco(base.get_caminho());
    const Tabela tabela = banco.get_tabela(TABELA_PRODUTO);
    VERIFICA_IGUAL(tabela.get_nome(), std::string("produto"));
    VERIFICA_IGUAL(tabela.get_caminho(), base.arquivo("produto.json"));
    VERIFICA_IGUAL(banco.caminho_arquivo("historico.log"), base.arquivo("historico.log"));
}

// ---------------------------------------------------------------- Tabela: operações básicas

TESTE(tabela_sem_arquivo_esta_vazia_e_nao_cria_o_arquivo) {
    DiretorioTemporario base;
    const Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA(tabela.select_todas().empty());
    VERIFICA(!fs::exists(base.arquivo("categoria.json")));
}

TESTE(tabela_insere_com_ids_sequenciais_a_partir_de_1) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_IGUAL(tabela.insere(linha_com_nome("a")), 1);
    VERIFICA_IGUAL(tabela.insere(linha_com_nome("b")), 2);
    VERIFICA_IGUAL(tabela.insere(linha_com_nome("c")), 3);
    const auto linhas = tabela.select_todas();
    VERIFICA_IGUAL(linhas.size(), 3u);
    VERIFICA_IGUAL(linhas[1].campo("id").como_int(), 2);
    VERIFICA_IGUAL(linhas[1].campo("nome").como_texto(), std::string("b"));
}

TESTE(tabela_grava_o_id_mesmo_que_a_linha_ja_traga_um) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    Json linha = linha_com_nome("x");
    linha.define("id", 0);
    VERIFICA_IGUAL(tabela.insere(linha), 1);
    VERIFICA_IGUAL(tabela.select_todas()[0].campo("id").como_int(), 1);
}

TESTE(tabela_persiste_os_dados_em_arquivo) {
    DiretorioTemporario base;
    {
        Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
        tabela.insere(linha_com_nome("persistida"));
    }
    // outra "sessão": novos objetos lendo o mesmo arquivo
    const Tabela outra = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_IGUAL(outra.select_todas().size(), 1u);
    VERIFICA_IGUAL(outra.select_todas()[0].campo("nome").como_texto(), std::string("persistida"));
}

TESTE(tabela_nao_reaproveita_ids_apos_exclusao) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    tabela.insere(linha_com_nome("a"));
    tabela.insere(linha_com_nome("b"));
    VERIFICA(tabela.exclui(2));
    VERIFICA_IGUAL(tabela.insere(linha_com_nome("c")), 3);   // como o auto_increment do MySQL
}

TESTE(tabela_exclui_e_informa_se_existia) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    tabela.insere(linha_com_nome("a"));
    tabela.insere(linha_com_nome("b"));
    VERIFICA(tabela.exclui(1));
    VERIFICA(!tabela.exclui(1));
    VERIFICA(!tabela.exclui(99));
    VERIFICA_IGUAL(tabela.select_todas().size(), 1u);
    VERIFICA_IGUAL(tabela.select_todas()[0].campo("nome").como_texto(), std::string("b"));
}

TESTE(tabela_atualiza_a_linha_de_mesmo_id) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    tabela.insere(linha_com_nome("antigo"));
    Json nova = linha_com_nome("novo");
    nova.define("id", 1);
    VERIFICA(tabela.atualiza(nova));
    VERIFICA_IGUAL(tabela.select_todas()[0].campo("nome").como_texto(), std::string("novo"));

    nova.define("id", 42);
    VERIFICA(!tabela.atualiza(nova));   // id inexistente: nada muda
    VERIFICA_IGUAL(tabela.select_todas().size(), 1u);
}

TESTE(tabela_valida_as_linhas_recebidas) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_EXCECAO(tabela.insere(Json(5)), ErroValidacao);
    VERIFICA_EXCECAO(tabela.insere(Json::array()), ErroValidacao);
    VERIFICA_EXCECAO(tabela.atualiza(linha_com_nome("sem id")), ErroValidacao);
    VERIFICA(!fs::exists(base.arquivo("categoria.json")));   // nada foi gravado
}

TESTE(tabela_preserva_acentos_aspas_e_barras) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    const std::string nome = "pão \"quente\" \\ ação/novo\ttab";
    tabela.insere(linha_com_nome(nome));
    VERIFICA_IGUAL(tabela.select_todas()[0].campo("nome").como_texto(), nome);
}

// ---------------------------------------------------------------- Tabela: arquivo gravado

TESTE(tabela_grava_json_legivel_e_sem_arquivo_temporario) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    tabela.insere(linha_com_nome("bebidas"));
    tabela.insere(linha_com_nome("limpeza"));
    const std::string conteudo = le_arquivo(base.arquivo("categoria.json"));
    VERIFICA_IGUAL(conteudo,
                   std::string("{\n"
                               "  \"proximo_id\": 3,\n"
                               "  \"linhas\": [\n"
                               "    {\"nome\": \"bebidas\", \"id\": 1},\n"
                               "    {\"nome\": \"limpeza\", \"id\": 2}\n"
                               "  ]\n"
                               "}\n"));
    VERIFICA(!fs::exists(base.arquivo("categoria.json.tmp")));
}

// ---------------------------------------------------------------- Tabela: defesa contra arquivos ruins

TESTE(tabela_arquivo_corrompido_lanca_erro_e_nao_e_sobrescrito) {
    DiretorioTemporario base;
    const std::string caminho = base.arquivo("categoria.json");
    escreve_arquivo(caminho, "{ isto nao e json");
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_EXCECAO_MSG(tabela.select_todas(), ErroJson, "tabela 'categoria'");
    VERIFICA_EXCECAO(tabela.insere(linha_com_nome("x")), ErroJson);
    VERIFICA_EXCECAO(tabela.exclui(1), ErroJson);
    VERIFICA_IGUAL(le_arquivo(caminho), std::string("{ isto nao e json"));   // o arquivo ruim foi preservado
}

TESTE(tabela_rejeita_estruturas_invalidas) {
    const char* invalidos[] = {
        "[]", "5", "null", "{}", R"({"linhas": 5})", R"({"linhas": [5]})", R"({"linhas": [{"nome": "sem id"}]})",
        R"({"linhas": [{"id": "x"}]})", R"({"linhas": [{"id": 0}]})", R"({"linhas": [{"id": -3}]})",
        R"({"linhas": [{"id": 1}, {"id": 1}]})", R"({"linhas": [{"id": 1.5}]})", R"({"proximo_id": "a", "linhas": []})",
        ""};
    for (const char* conteudo : invalidos) {
        DiretorioTemporario base;
        escreve_arquivo(base.arquivo("categoria.json"), conteudo);
        const Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
        bool lancou = false;
        try {
            tabela.select_todas();
        } catch (const ErroPersistencia&) {
            lancou = true;
        }
        if (!lancou) FALHA("deveria rejeitar o conteúdo: [" << conteudo << "]");
    }
}

TESTE(tabela_corrige_contador_menor_que_o_maior_id) {
    DiretorioTemporario base;
    escreve_arquivo(base.arquivo("categoria.json"), R"({"proximo_id": 1, "linhas": [{"id": 7, "nome": "x"}]})");
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_IGUAL(tabela.insere(linha_com_nome("y")), 8);   // nunca repete o id 7
}

TESTE(tabela_aceita_arquivo_sem_proximo_id) {
    DiretorioTemporario base;
    escreve_arquivo(base.arquivo("categoria.json"), R"({"linhas": [{"id": 3, "nome": "x"}]})");
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_IGUAL(tabela.insere(linha_com_nome("y")), 4);
}

TESTE(tabela_pasta_no_lugar_do_arquivo_lanca_erro) {
    DiretorioTemporario base;
    fs::create_directories(base.arquivo("categoria.json"));
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_EXCECAO_MSG(tabela.select_todas(), ErroPersistencia, "não é um arquivo comum");
}

TESTE(tabela_falha_ao_gravar_preserva_o_arquivo_anterior) {
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    tabela.insere(linha_com_nome("original"));
    const std::string antes = le_arquivo(base.arquivo("categoria.json"));

    fs::create_directories(base.arquivo("categoria.json.tmp"));   // impede a criação do temporário
    VERIFICA_EXCECAO_MSG(tabela.insere(linha_com_nome("novo")), ErroPersistencia, "não foi possível criar");
    VERIFICA_IGUAL(le_arquivo(base.arquivo("categoria.json")), antes);   // dados antigos intactos
}

TESTE(tabela_recusa_arquivo_que_ja_usou_o_maior_id_possivel) {
    DiretorioTemporario base;
    escreve_arquivo(base.arquivo("categoria.json"), R"({"linhas": [{"id": 2147483647, "nome": "x"}]})");
    const Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_EXCECAO_MSG(tabela.select_todas(), ErroPersistencia, "limite de registros");
}

TESTE(tabela_recusa_inserir_quando_o_contador_chegou_ao_limite) {
    DiretorioTemporario base;
    escreve_arquivo(base.arquivo("categoria.json"), R"({"proximo_id": 2147483647, "linhas": []})");
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    VERIFICA_EXCECAO_MSG(tabela.insere(linha_com_nome("x")), ErroPersistencia, "limite de registros");
}

TESTE(tabela_trata_falha_de_gravacao_por_disco_cheio) {
    if (!fs::exists("/dev/full")) return;   // dispositivo que sempre responde "sem espaço": só existe em Linux
    DiretorioTemporario base;
    Tabela tabela = BancoDado(base.get_caminho()).get_tabela("categoria");
    tabela.insere(linha_com_nome("original"));
    const std::string antes = le_arquivo(base.arquivo("categoria.json"));

    fs::create_symlink("/dev/full", base.arquivo("categoria.json.tmp"));   // o arquivo temporário "não tem espaço"
    VERIFICA_EXCECAO_MSG(tabela.insere(linha_com_nome("novo")), ErroPersistencia, "falha ao gravar");
    VERIFICA_IGUAL(le_arquivo(base.arquivo("categoria.json")), antes);   // dados antigos preservados
    VERIFICA(!fs::exists(base.arquivo("categoria.json.tmp")));          // e o temporário foi removido
}
