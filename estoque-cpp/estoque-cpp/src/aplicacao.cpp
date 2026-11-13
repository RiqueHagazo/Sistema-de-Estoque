#include "aplicacao.hpp"

#include <algorithm>
#include <iostream>

#include "erros.hpp"
#include "terminal.hpp"
#include "texto.hpp"

namespace {

constexpr std::size_t LARGURA_TELA = 66;
constexpr std::size_t LARGURA_CELULA = 28;        // textos mais longos são cortados nas tabelas
constexpr std::size_t QUANTIDADE_HISTORICO = 20;  // linhas mostradas em "ver histórico"

std::string reais(double valor) {
    return "R$" + formata_valor(valor);
}

std::string celula(const std::string& texto) {
    return limita_texto(texto, LARGURA_CELULA);
}

}  // namespace

Aplicacao::Aplicacao(ServicoEstoque& servico, std::istream& entrada, std::ostream& saida)
    : servico(servico),
      entrada(entrada),
      saida(saida),
      entrada_interativa(&entrada == &std::cin && entrada_padrao_eh_terminal()) {}

int Aplicacao::executar() {
    try {
        if (!servico.existe_usuario_cadastrado()) {
            primeiro_acesso();
        }
        while (login()) {
            index();
        }
        saida << "\naté logo!\n";
        return 0;
    } catch (const FimDeEntrada&) {
        saida << "\n";   // Ctrl+D / fim do arquivo de entrada: encerra sem erro
        return 0;
    }
}

// ############################## ENTRADA E SAIDA ##############################

std::string Aplicacao::pergunta(const std::string& rotulo) {
    saida << rotulo << ": " << std::flush;
    std::string resposta;
    if (!std::getline(entrada, resposta)) {
        throw FimDeEntrada();
    }
    if (!resposta.empty() && resposta.back() == '\r') {
        resposta.pop_back();   // arquivos de entrada criados no Windows
    }
    if (!entrada_interativa) {
        saida << resposta << '\n';   // simulação: mostra o que "foi digitado"
    }
    return resposta;
}

std::string Aplicacao::pergunta_senha(const std::string& rotulo) {
    saida << rotulo << ": " << std::flush;
    std::string senha;
    {
        EcoDesativado sem_eco(entrada_interativa);   // a senha não aparece na tela
        if (!std::getline(entrada, senha)) {
            throw FimDeEntrada();
        }
    }
    if (!senha.empty() && senha.back() == '\r') {
        senha.pop_back();
    }
    saida << (entrada_interativa ? "" : "(oculta)") << '\n';
    return senha;
}

int Aplicacao::pergunta_inteiro(const std::string& rotulo, const std::string& campo) {
    return converte_inteiro(pergunta(rotulo), campo);
}

bool Aplicacao::confirma(const std::string& texto) {
    const std::string resposta = normaliza(remove_espacos(pergunta(texto + " (s/n)")));
    return resposta == "s" || resposta == "sim";
}

void Aplicacao::mensagem(const std::string& texto) {
    saida << ">> " << texto << '\n';
}

void Aplicacao::titulo(const std::string& texto) {
    const std::string linha(LARGURA_TELA, '=');
    const std::string direita =
        usuario_logado ? usuario_logado->get_usuario() + " (" + usuario_logado->get_nome_grupo() + ")" : "";
    saida << '\n' << linha << "\n " << texto;
    if (!direita.empty()) {
        const std::size_t usados = 1 + tamanho_utf8(texto) + tamanho_utf8(direita) + 1;
        saida << std::string(usados < LARGURA_TELA ? LARGURA_TELA - usados : 1, ' ') << direita;
    }
    saida << '\n' << linha << '\n';
}

void Aplicacao::imprime_tabela(const std::vector<std::string>& cabecalhos,
                               const std::vector<std::vector<std::string>>& linhas) {
    std::vector<std::size_t> larguras;
    for (const std::string& cabecalho : cabecalhos) {
        larguras.push_back(tamanho_utf8(cabecalho));
    }
    for (const std::vector<std::string>& linha : linhas) {
        for (std::size_t i = 0; i < linha.size() && i < larguras.size(); ++i) {
            larguras[i] = std::max(larguras[i], tamanho_utf8(linha[i]));
        }
    }

    auto imprime_linha = [&](const std::vector<std::string>& celulas) {
        for (std::size_t i = 0; i < larguras.size(); ++i) {
            if (i > 0) {
                saida << " | ";
            }
            const std::string texto = i < celulas.size() ? celulas[i] : "";
            saida << (i + 1 < larguras.size() ? preenche(texto, larguras[i]) : texto);   // sem espaços sobrando no fim
        }
        saida << '\n';
    };

    imprime_linha(cabecalhos);
    for (std::size_t i = 0; i < larguras.size(); ++i) {
        saida << (i > 0 ? "-+-" : "") << std::string(larguras[i], '-');
    }
    saida << '\n';
    for (const std::vector<std::string>& linha : linhas) {
        imprime_linha(linha);
    }
}

void Aplicacao::trata_erros(const std::function<void()>& acao) {
    try {
        acao();
    } catch (const FimDeEntrada&) {
        throw;   // não é um erro: é o fim da entrada
    } catch (const ErroEstoque& e) {
        mensagem(std::string("erro: ") + e.what());
    } catch (const std::exception& e) {
        mensagem(std::string("erro inesperado: ") + e.what());
    }
}

// ############################## TELAS ##############################

void Aplicacao::exibe_produtos(const std::vector<std::shared_ptr<Produto>>& lista, const std::string& se_vazia) {
    if (lista.empty()) {
        saida << se_vazia << '\n';
        return;
    }
    std::vector<std::vector<std::string>> linhas;
    for (const std::shared_ptr<Produto>& produto : lista) {
        linhas.push_back({std::to_string(produto->get_id()), celula(produto->get_nome_produto()),
                          std::to_string(produto->get_quantidade_produto()), reais(produto->get_valor_produto()),
                          celula(produto->get_descricao_produto()), formata_data(produto->get_chegada_produto()),
                          produto->validade_formatada(), celula(produto->get_categoria_produto().get_nome())});
    }
    imprime_tabela({"código", "nome", "quantidade", "valor", "descrição", "chegada", "validade", "categoria"}, linhas);
}

void Aplicacao::exibe_produto_detalhado(const Produto& produto) {
    const std::vector<std::pair<std::string, std::string>> campos = {
        {"Código do produto", std::to_string(produto.get_id())},
        {"Categoria", produto.get_categoria_produto().get_nome()},
        {"Nome", produto.get_nome_produto()},
        {"Quantidade em estoque", std::to_string(produto.get_quantidade_produto())},
        {"Valor", reais(produto.get_valor_produto())},
        {"Descrição", produto.get_descricao_produto()},
        {"Data de chegada", formata_data(produto.get_chegada_produto())},
        {"Data de validade", produto.validade_formatada()}};
    for (const auto& campo : campos) {
        saida << preenche(campo.first, 22) << ": " << campo.second << '\n';
    }
}

void Aplicacao::exibe_categorias(const std::vector<std::shared_ptr<Categoria>>& lista) {
    if (lista.empty()) {
        saida << "nenhuma categoria cadastrada\n";
        return;
    }
    std::vector<std::vector<std::string>> linhas;
    for (const std::shared_ptr<Categoria>& categoria : lista) {
        linhas.push_back({std::to_string(categoria->get_id()), celula(categoria->get_nome())});
    }
    imprime_tabela({"id da categoria", "nome"}, linhas);
}

void Aplicacao::exibe_vendas(const std::vector<std::shared_ptr<Vendas>>& lista) {
    if (lista.empty()) {
        saida << "nenhuma venda registrada\n";
        return;
    }
    std::vector<std::vector<std::string>> linhas;
    for (const std::shared_ptr<Vendas>& venda : lista) {
        const Produto& produto = venda->get_produto_venda();
        linhas.push_back({celula(produto.get_nome_produto()), celula(produto.get_categoria_produto().get_nome()),
                          celula(venda->get_vendedor()), formata_data(venda->get_data_venda()),
                          venda->get_horario_venda().para_texto(), std::to_string(venda->get_quantidade_venda()),
                          reais(venda->get_valor_venda())});
    }
    imprime_tabela({"produto", "categoria", "vendedor", "data da venda", "horario da venda", "quantidade", "valor"}, linhas);
}

void Aplicacao::exibe_usuarios(const std::vector<std::shared_ptr<Usuario>>& lista) {
    std::vector<std::vector<std::string>> linhas;
    for (const std::shared_ptr<Usuario>& usuario : lista) {
        linhas.push_back({celula(usuario->get_usuario()), usuario->get_nome_grupo()});
    }
    imprime_tabela({"usuário", "grupo"}, linhas);
}

// ############################## LOGIN ##############################

void Aplicacao::primeiro_acesso() {
    titulo("primeiro acesso");
    saida << "nenhum usuário cadastrado: crie o administrador do sistema.\n";
    while (true) {
        const std::string user = pergunta("usuário");
        const std::string senha = pergunta_senha("senha");
        const std::string repeticao = pergunta_senha("repita a senha");
        if (senha != repeticao) {
            mensagem("as senhas não conferem");
            continue;
        }
        try {
            servico.cadastrar_primeiro_administrador(user, senha);
            mensagem("cadastrado com sucesso");
            return;
        } catch (const ErroRegraNegocio& e) {
            mensagem(std::string("erro: ") + e.what());   // alguém já criou usuários: segue para o login
            return;
        } catch (const ErroEstoque& e) {
            mensagem(std::string("erro: ") + e.what());
        }
    }
}

bool Aplicacao::login() {
    titulo("digite usuário e senha");
    saida << "(deixe o usuário em branco para encerrar)\n";
    while (true) {
        const std::string user = pergunta("usuário");
        if (remove_espacos(user).empty()) {
            return false;
        }
        const std::string senha = pergunta_senha("senha");
        try {
            usuario_logado = servico.login(user, senha);
            return true;
        } catch (const ErroEstoque& e) {
            mensagem(e.what());   // 'login/senha inválidos'
        }
    }
}

void Aplicacao::logout() {
    if (usuario_logado) {
        servico.logout(*usuario_logado);
    }
    usuario_logado.reset();
}

// ############################## INDEX ##############################

void Aplicacao::index() {
    struct Opcao {
        const char* codigo;
        const char* rotulo;
        bool somente_administrador;
        void (Aplicacao::*acao)();
    };
    static const Opcao OPCOES[] = {
        {"1", "produtos", false, &Aplicacao::produtos},
        {"2", "categoria", false, &Aplicacao::categoria},
        {"3", "retirada", false, &Aplicacao::vendas},
        {"4", "alertas", false, &Aplicacao::alertas},
        {"5", "registro", true, &Aplicacao::registro_vendas},
        {"6", "administrativo", true, &Aplicacao::administrativo},
    };

    while (usuario_logado) {
        titulo("home");
        saida << "bem vindo(a) " << usuario_logado->get_usuario() << "!\n\n";
        for (const Opcao& opcao : OPCOES) {
            // o usuário decide (polimorfismo) quais telas enxerga
            if (!opcao.somente_administrador || usuario_logado->pode_administrar()) {
                saida << "  " << opcao.codigo << ") " << opcao.rotulo << '\n';
            }
        }
        saida << "  0) sair\n";

        const std::string escolha = remove_espacos(pergunta("opção"));
        if (escolha == "0") {
            logout();
            return;
        }
        trata_erros([&] {
            for (const Opcao& opcao : OPCOES) {
                if (escolha == opcao.codigo) {
                    if (opcao.somente_administrador && !usuario_logado->pode_administrar()) {
                        mensagem("somente administradores");
                        return;
                    }
                    (this->*opcao.acao)();
                    return;
                }
            }
            mensagem("opção inválida");
        });
    }
}

// ############################## ADMINISTRATIVO ##############################

void Aplicacao::administrativo() {
    while (true) {
        titulo("administrativo");
        exibe_usuarios(servico.lista_usuarios(*usuario_logado));
        saida << "\n  1) cadastrar usuário\n  2) excluir usuário\n  3) ver histórico de operações\n  0) voltar\n";
        const std::string escolha = remove_espacos(pergunta("opção"));
        if (escolha == "0") {
            return;
        }
        trata_erros([&] {
            if (escolha == "1") cadastrar_usuario();
            else if (escolha == "2") excluir_usuario();
            else if (escolha == "3") historico();
            else mensagem("opção inválida");
        });
    }
}

void Aplicacao::cadastrar_usuario() {
    const std::string user = pergunta("usuário");
    const std::string senha = pergunta_senha("senha");
    const int grupo = pergunta_inteiro("grupo (0 = operador, 1 = administrador)", "grupo");
    servico.cadastrar_usuario(*usuario_logado, user, senha, grupo);
    mensagem("cadastrado com sucesso");
}

void Aplicacao::excluir_usuario() {
    const std::string nome_usuario = pergunta("usuário a excluir");
    if (!confirma("excluir o usuário '" + remove_espacos(nome_usuario) + "'?")) {
        mensagem("operação cancelada");
        return;
    }
    servico.excluir_usuario(*usuario_logado, nome_usuario);
    mensagem("excluido");
}

void Aplicacao::historico() {
    titulo("histórico de operações");
    const std::vector<std::string> linhas = servico.ultimas_operacoes(*usuario_logado, QUANTIDADE_HISTORICO);
    if (linhas.empty()) {
        saida << "nenhuma operação registrada\n";
    }
    for (const std::string& linha : linhas) {
        saida << linha << '\n';
    }
}

// ############################## PRODUTO ##############################

void Aplicacao::produtos() {
    while (true) {
        titulo("produtos");
        exibe_produtos(servico.lista_produtos(), "nenhum produto cadastrado");
        saida << "\n  1) adicionar +\n  2) pesquisar por nome\n  3) excluir\n  4) repor estoque\n  0) voltar\n";
        const std::string escolha = remove_espacos(pergunta("opção"));
        if (escolha == "0") {
            return;
        }
        trata_erros([&] {
            if (escolha == "1") cadastro_produtos();
            else if (escolha == "2") buscar_produto();
            else if (escolha == "3") excluir_produto();
            else if (escolha == "4") repor_estoque();
            else mensagem("opção inválida");
        });
    }
}

void Aplicacao::cadastro_produtos() {
    titulo("cadastrar produto");
    const std::vector<std::shared_ptr<Categoria>> lista_categoria = servico.lista_categorias();
    if (lista_categoria.empty()) {
        mensagem("cadastre uma categoria antes de adicionar produtos");
        return;
    }
    exibe_categorias(lista_categoria);

    int categoria_produto = 0;
    try {
        categoria_produto = pergunta_inteiro("categoria (código)", "categoria");
    } catch (const ErroValidacao&) {
        throw ErroValidacao("categoria inválida");   // mesma mensagem do original
    }
    const std::string nome = pergunta("nome");
    const int quantidade = pergunta_inteiro("quantidade", "quantidade");
    const double valor = converte_real(pergunta("valor"), "valor");
    const std::string descricao = pergunta("descrição (opcional)");

    const std::string texto_chegada = remove_espacos(pergunta("data de chegada (dd/mm/aaaa, enter = hoje)"));
    const Data chegada = texto_chegada.empty() ? servico.hoje() : Data::de_texto(texto_chegada);
    const std::string texto_validade = remove_espacos(pergunta("data de validade (dd/mm/aaaa, enter = sem validade)"));
    std::optional<Data> validade;
    if (!texto_validade.empty()) {
        validade = Data::de_texto(texto_validade);
    }

    servico.cadastrar_produto(*usuario_logado, categoria_produto, nome, quantidade, valor, chegada, validade, descricao);
    mensagem("salvo");
}

void Aplicacao::excluir_produto() {
    const int id_produto = pergunta_inteiro("código do produto a excluir", "código");
    const std::shared_ptr<Produto> produto = servico.busca_produto_id(id_produto);
    if (!confirma("excluir o produto '" + produto->get_nome_produto() + "'?")) {
        mensagem("operação cancelada");
        return;
    }
    servico.excluir_produto(*usuario_logado, id_produto);
    mensagem("produto excluido");
}

void Aplicacao::buscar_produto() {
    const std::string nome_buscado = pergunta("pesquisar por produto");
    titulo("resultado da pesquisa");
    exibe_produtos(servico.busca_produto(nome_buscado), "nenhum produto encontrado");
}

void Aplicacao::repor_estoque() {
    const int id_produto = pergunta_inteiro("código do produto", "código");
    const int quantidade = pergunta_inteiro("quantidade a acrescentar", "quantidade");
    servico.repoe_estoque(*usuario_logado, id_produto, quantidade);
    mensagem("estoque atualizado");
}

// ############################## CATEGORIA ##############################

void Aplicacao::categoria() {
    while (true) {
        titulo("categoria");
        exibe_categorias(servico.lista_categorias());
        saida << "\n  1) adicionar +\n  0) voltar\n";
        const std::string escolha = remove_espacos(pergunta("opção"));
        if (escolha == "0") {
            return;
        }
        trata_erros([&] {
            if (escolha == "1") cadastrar_categoria();
            else mensagem("opção inválida");
        });
    }
}

void Aplicacao::cadastrar_categoria() {
    const std::string nome = pergunta("nome da categoria");
    servico.cadastrar_categoria(*usuario_logado, nome);
    mensagem("salvo");
}

// ############################## VENDAS ##############################

void Aplicacao::vendas() {
    titulo("retirada");
    const int codigo = pergunta_inteiro("digite o codigo do produto", "código");
    const std::shared_ptr<Produto> produto = servico.busca_produto_id(codigo);
    saida << '\n';
    exibe_produto_detalhado(*produto);
    saida << '\n';
    const int quantidade_vendida = pergunta_inteiro("Número de produtos vendidos", "número de produtos vendidos");

    const std::shared_ptr<Vendas> venda = servico.realiza_venda(*usuario_logado, produto->get_id(), quantidade_vendida);
    mensagem("realizada com sucesso");
    saida << "valor da venda: " << reais(venda->get_valor_venda()) << '\n';
    if (usuario_logado->pode_administrar()) {
        // como no original, quem administra vê o registro de vendas logo após a venda
        titulo("registro de vendas");
        exibe_vendas(servico.registro_vendas(*usuario_logado));
    }
}

void Aplicacao::registro_vendas() {
    titulo("registro de vendas");
    exibe_vendas(servico.registro_vendas(*usuario_logado));
    while (true) {
        saida << "\n  1) pesquisar por período\n  0) voltar\n";
        const std::string escolha = remove_espacos(pergunta("opção"));
        if (escolha == "0") {
            return;
        }
        trata_erros([&] {
            if (escolha == "1") pesquisa_periodo();
            else mensagem("opção inválida");
        });
    }
}

void Aplicacao::pesquisa_periodo() {
    const Data data_inicial = Data::de_texto(pergunta("data inicial (dd/mm/aaaa)"));
    const Data data_final = Data::de_texto(pergunta("data final (dd/mm/aaaa)"));
    const std::vector<std::shared_ptr<Vendas>> vendas_periodo =
        servico.pesquisa_periodo(*usuario_logado, data_inicial, data_final);
    const double valor_final = servico.valor_por_periodo(*usuario_logado, data_inicial, data_final);
    saida << '\n';
    exibe_vendas(vendas_periodo);
    saida << "\nvalor por periodo: " << reais(valor_final) << '\n';
}

// ############################## ALERTAS ##############################

void Aplicacao::alertas() {
    titulo("alertas");
    saida << "\nprodutos vencidos\n";
    exibe_produtos(servico.produtos_vencidos(), "nenhum produto vencido");
    saida << "\nprodutos que vencem em até " << ServicoEstoque::DIAS_ALERTA_VENCIMENTO << " dias\n";
    exibe_produtos(servico.produtos_a_vencer(ServicoEstoque::DIAS_ALERTA_VENCIMENTO), "nenhum produto perto do vencimento");
    saida << "\nprodutos com estoque de até " << ServicoEstoque::ESTOQUE_MINIMO << " unidades\n";
    exibe_produtos(servico.produtos_estoque_baixo(ServicoEstoque::ESTOQUE_MINIMO), "nenhum produto com estoque baixo");
}
