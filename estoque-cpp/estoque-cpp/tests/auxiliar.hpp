#ifndef AUXILIAR_HPP
#define AUXILIAR_HPP

/**
 * @file auxiliar.hpp
 * @brief Utilitários compartilhados pelos testes: pasta temporária, relógio falso e ambiente com DAOs.
 */

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>

#ifdef _WIN32
#include <process.h>
#define OBTEM_PID _getpid
#else
#include <unistd.h>
#define OBTEM_PID getpid
#endif

#include "bancodado.hpp"
#include "categoria_dao.hpp"
#include "crypto.hpp"
#include "datahora.hpp"
#include "produto_dao.hpp"
#include "usuario_dao.hpp"
#include "vendas_dao.hpp"

/** @brief Cria uma pasta temporária exclusiva do teste e a apaga ao sair do escopo (RAII). */
class DiretorioTemporario {
public:
    DiretorioTemporario() {
        static std::atomic<int> contador{0};
        caminho = (std::filesystem::temp_directory_path() /
                   ("estoque_teste_" + std::to_string(OBTEM_PID()) + "_" + std::to_string(contador++)))
                      .string();
        std::filesystem::remove_all(caminho);
        std::filesystem::create_directories(caminho);
    }
    ~DiretorioTemporario() {
        std::error_code ec;
        std::filesystem::remove_all(caminho, ec);
    }
    DiretorioTemporario(const DiretorioTemporario&) = delete;
    DiretorioTemporario& operator=(const DiretorioTemporario&) = delete;

    const std::string& get_caminho() const { return caminho; }
    std::string arquivo(const std::string& nome) const { return caminho + "/" + nome; }

private:
    std::string caminho;
};

/** @brief Relógio de mentira: sempre devolve a data/hora configurada. */
class RelogioFixo : public Relogio {
public:
    RelogioFixo(const Data& data, const Horario& horario) : data_atual(data), horario_atual(horario) {}
    Data hoje() const override { return data_atual; }
    Horario agora() const override { return horario_atual; }
    void define(const Data& data, const Horario& horario) {
        data_atual = data;
        horario_atual = horario;
    }

private:
    Data data_atual;
    Horario horario_atual;
};

inline void escreve_arquivo(const std::string& caminho, const std::string& conteudo) {
    std::ofstream saida(caminho, std::ios::binary | std::ios::trunc);
    saida << conteudo;
}

inline std::string le_arquivo(const std::string& caminho) {
    std::ifstream entrada(caminho, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(entrada)), std::istreambuf_iterator<char>());
}

/** @brief Pasta temporária + banco + DAOs prontos para uso nos testes. */
struct Ambiente {
    DiretorioTemporario pasta;
    BancoDado banco;
    CategoriaDAO categorias;
    ProdutoDAO produtos;
    VendasDAO vendas;
    UsuarioDAO usuarios;

    Ambiente()
        : banco(pasta.get_caminho()), categorias(banco), produtos(banco), vendas(banco, produtos), usuarios(banco) {}

    std::shared_ptr<Categoria> nova_categoria(const std::string& nome) {
        auto categoria = std::make_shared<Categoria>(Entidade::ID_NAO_DEFINIDO, nome);
        categorias.insert_categoria(*categoria);
        return categoria;
    }

    std::shared_ptr<Produto> novo_produto(const Categoria& categoria, const std::string& nome, int quantidade,
                                          double valor, const std::optional<Data>& validade = std::nullopt) {
        auto produto = Produto::cria(Entidade::ID_NAO_DEFINIDO, nome, quantidade, valor, Data(2026, 10, 1), validade, "",
                                     categoria);
        produtos.insere_produto(*produto);
        return produto;
    }

    std::shared_ptr<Vendas> nova_venda(const std::shared_ptr<Produto>& produto, const std::string& vendedor,
                                       int quantidade, const Data& data, const Horario& horario = Horario(10, 0, 0)) {
        auto venda = std::make_shared<Vendas>(Entidade::ID_NAO_DEFINIDO, vendedor, produto, quantidade,
                                              produto->calcula_valor(quantidade), data, horario);
        vendas.registra_vendas(*venda);
        return venda;
    }

    std::shared_ptr<Usuario> novo_usuario(const std::string& nome, const std::string& senha, int grupo) {
        auto usuario = Usuario::cria(grupo, Entidade::ID_NAO_DEFINIDO, nome, cryptar(senha, 10));
        usuarios.cadastrar_usuario(*usuario);
        return usuario;
    }
};

#endif
