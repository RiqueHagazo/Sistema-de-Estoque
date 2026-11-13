#include <iostream>
#include <string>

#include "aplicacao.hpp"
#include "bancodado.hpp"
#include "datahora.hpp"
#include "erros.hpp"
#include "historico.hpp"
#include "servico_estoque.hpp"

namespace {

const char* PASTA_PADRAO = "dados";

void mostra_ajuda(const char* programa) {
    std::cout << "uso: " << programa << " [pasta_de_dados]\n\n"
              << "  pasta_de_dados   onde ficam os arquivos .json e o histórico (padrão: " << PASTA_PADRAO << ")\n\n"
              << "para simular uma sessão a partir de um arquivo de texto:\n"
              << "  " << programa << " pasta_de_dados < exemplos/entrada_demo.txt\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        std::string pasta = PASTA_PADRAO;
        if (argc > 2) {
            std::cerr << "uso: " << argv[0] << " [pasta_de_dados]\n";
            return 2;
        }
        if (argc == 2) {
            const std::string argumento = argv[1];
            if (argumento == "-h" || argumento == "--ajuda" || argumento == "--help") {
                mostra_ajuda(argv[0]);
                return 0;
            }
            pasta = argumento;
        }

        BancoDado banco(pasta);
        RelogioSistema relogio;
        Historico historico(banco.caminho_arquivo("historico.log"), relogio);
        ServicoEstoque servico(banco, historico, relogio);
        Aplicacao aplicacao(servico, std::cin, std::cout);
        return aplicacao.executar();
    } catch (const ErroEstoque& e) {
        std::cerr << "erro: " << e.what() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "erro inesperado: " << e.what() << '\n';
        return 3;
    } catch (...) {
        std::cerr << "erro desconhecido\n";
        return 3;
    }
}
