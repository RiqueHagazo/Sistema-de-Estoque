#include "auxiliar.hpp"
#include "erros.hpp"
#include "teste.hpp"

TESTE(categoria_dao_grava_e_atribui_o_id) {
    Ambiente ambiente;
    const auto bebidas = ambiente.nova_categoria("bebidas");
    const auto limpeza = ambiente.nova_categoria("limpeza");
    VERIFICA_IGUAL(bebidas->get_id(), 1);
    VERIFICA_IGUAL(limpeza->get_id(), 2);
    VERIFICA(bebidas->foi_persistida());
}

TESTE(categoria_dao_persiste_para_outra_sessao) {
    Ambiente ambiente;
    ambiente.nova_categoria("frios");
    BancoDado outro_banco(ambiente.pasta.get_caminho());
    const CategoriaDAO outro_dao(outro_banco);
    VERIFICA_IGUAL(outro_dao.select_todas_categorias().size(), 1u);
    VERIFICA_IGUAL(outro_dao.select_todas_categorias()[0]->get_nome(), std::string("frios"));
}

TESTE(categoria_dao_lista_em_ordem_alfabetica_ignorando_caixa_e_acentos) {
    Ambiente ambiente;
    for (const char* nome : {"Zebra", "água", "bebidas", "Açúcar"}) {
        ambiente.nova_categoria(nome);
    }
    const auto lista = ambiente.categorias.select_todas_categorias();
    VERIFICA_IGUAL(lista.size(), 4u);
    VERIFICA_IGUAL(lista[0]->get_nome(), std::string("Açúcar"));
    VERIFICA_IGUAL(lista[1]->get_nome(), std::string("água"));
    VERIFICA_IGUAL(lista[2]->get_nome(), std::string("bebidas"));
    VERIFICA_IGUAL(lista[3]->get_nome(), std::string("Zebra"));
}

TESTE(categoria_dao_sem_categorias_devolve_lista_vazia) {
    Ambiente ambiente;
    VERIFICA(ambiente.categorias.select_todas_categorias().empty());
}

TESTE(categoria_dao_rejeita_nome_repetido_ignorando_caixa_e_acentos) {
    Ambiente ambiente;
    ambiente.nova_categoria("Pão");
    VERIFICA_EXCECAO_MSG(ambiente.nova_categoria("pão"), ErroIntegridade, "categoria já existe");
    VERIFICA_EXCECAO(ambiente.nova_categoria("PAO"), ErroIntegridade);
    VERIFICA_IGUAL(ambiente.categorias.select_todas_categorias().size(), 1u);
}

TESTE(categoria_dao_nao_grava_duas_vezes_o_mesmo_objeto) {
    Ambiente ambiente;
    const auto categoria = ambiente.nova_categoria("bebidas");
    VERIFICA_EXCECAO(ambiente.categorias.insert_categoria(*categoria), ErroRegraNegocio);
    VERIFICA_IGUAL(ambiente.categorias.select_todas_categorias().size(), 1u);
}

TESTE(categoria_dao_busca_por_id) {
    Ambiente ambiente;
    ambiente.nova_categoria("a");
    const auto segunda = ambiente.nova_categoria("b");
    VERIFICA_IGUAL(ambiente.categorias.busca_categoria_id(segunda->get_id())->get_nome(), std::string("b"));
    VERIFICA_EXCECAO_MSG(ambiente.categorias.busca_categoria_id(99), ErroNaoEncontrado, "categoria não encontrada");
}

TESTE(categoria_dao_arquivo_corrompido_lanca_erro_de_persistencia) {
    Ambiente ambiente;
    escreve_arquivo(ambiente.pasta.arquivo("categoria.json"), "lixo");
    VERIFICA_EXCECAO(ambiente.categorias.select_todas_categorias(), ErroPersistencia);
    VERIFICA_EXCECAO(ambiente.nova_categoria("x"), ErroPersistencia);
}

TESTE(categoria_dao_linha_invalida_vira_erro_de_persistencia) {
    Ambiente ambiente;
    escreve_arquivo(ambiente.pasta.arquivo("categoria.json"), R"({"proximo_id": 2, "linhas": [{"id": 1, "nome": ""}]})");
    VERIFICA_EXCECAO(ambiente.categorias.select_todas_categorias(), ErroPersistencia);
}
