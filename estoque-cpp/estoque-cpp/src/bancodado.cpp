#include "bancodado.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>

#include "erros.hpp"
#include "texto.hpp"

namespace fs = std::filesystem;

// ############################## TABELA ##############################

Tabela::Tabela(const std::string& caminho, const std::string& nome) : caminho(caminho), nome(nome) {}

const std::string& Tabela::get_nome() const { return nome; }
const std::string& Tabela::get_caminho() const { return caminho; }

Tabela::Conteudo Tabela::le() const {
    Conteudo conteudo;

    std::error_code erro;
    if (!fs::exists(caminho, erro)) {
        return conteudo;   // tabela nova: ainda não existe arquivo
    }
    if (!fs::is_regular_file(caminho, erro)) {
        throw ErroPersistencia("tabela '" + nome + "': " + caminho + " não é um arquivo comum");
    }

    std::ifstream arquivo(caminho, std::ios::binary);
    if (!arquivo) {
        throw ErroPersistencia("tabela '" + nome + "': não foi possível abrir " + caminho);
    }
    const std::string texto((std::istreambuf_iterator<char>(arquivo)), std::istreambuf_iterator<char>());
    if (arquivo.bad()) {
        throw ErroPersistencia("tabela '" + nome + "': falha ao ler " + caminho);
    }

    auto estrutura_invalida = [&](const std::string& motivo) {
        return ErroPersistencia("tabela '" + nome + "': " + motivo + " (" + caminho + ")");
    };

    try {
        const Json raiz = Json::de_texto(texto);
        if (!raiz.eh_objeto() || !raiz.tem_campo("linhas") || !raiz.campo("linhas").eh_array()) {
            throw estrutura_invalida("estrutura inesperada: esperado um objeto com o campo 'linhas'");
        }

        std::set<int> ids_vistos;
        int maior_id = 0;
        for (const Json& linha : raiz.campo("linhas").itens()) {
            if (!linha.eh_objeto() || !linha.tem_campo("id")) {
                throw estrutura_invalida("linha sem o campo 'id'");
            }
            const int id = linha.campo("id").como_int();
            if (id < 1) {
                throw estrutura_invalida("id inválido (" + std::to_string(id) + ")");
            }
            if (!ids_vistos.insert(id).second) {
                throw estrutura_invalida("id duplicado (" + std::to_string(id) + ")");
            }
            maior_id = std::max(maior_id, id);
            conteudo.linhas.push_back(linha);
        }

        // O contador nunca pode ficar abaixo do maior id existente (arquivo editado à mão, por exemplo).
        const int declarado = raiz.tem_campo("proximo_id") ? raiz.campo("proximo_id").como_int() : 1;
        if (maior_id == std::numeric_limits<int>::max()) {
            throw estrutura_invalida("limite de registros atingido");
        }
        conteudo.proximo_id = std::max(declarado, maior_id + 1);
    } catch (const ErroJson& e) {
        throw ErroJson("tabela '" + nome + "': " + e.what());
    }
    return conteudo;
}

void Tabela::grava(const Conteudo& conteudo) const {
    Json linhas = Json::array();
    for (const Json& linha : conteudo.linhas) {
        linhas.adiciona(linha);
    }
    Json raiz = Json::objeto();
    raiz.define("proximo_id", conteudo.proximo_id).define("linhas", linhas);
    const std::string texto = raiz.para_texto(2) + "\n";

    // 1) grava tudo em um arquivo temporário; 2) troca pelo definitivo de uma só vez
    const std::string temporario = caminho + ".tmp";
    {
        std::ofstream saida(temporario, std::ios::binary | std::ios::trunc);
        if (!saida) {
            throw ErroPersistencia("tabela '" + nome + "': não foi possível criar " + temporario);
        }
        saida << texto;
        saida.flush();
        if (!saida) {
            saida.close();
            std::error_code ignorado;
            fs::remove(temporario, ignorado);
            throw ErroPersistencia("tabela '" + nome + "': falha ao gravar " + temporario + " (disco cheio?)");
        }
    }
    std::error_code erro;
    fs::rename(temporario, caminho, erro);
    if (erro) {
        std::error_code ignorado;
        fs::remove(temporario, ignorado);
        throw ErroPersistencia("tabela '" + nome + "': não foi possível gravar " + caminho + ": " + erro.message());
    }
}

std::vector<Json> Tabela::select_todas() const {
    return le().linhas;
}

int Tabela::insere(Json linha) {
    if (!linha.eh_objeto()) {
        throw ErroValidacao("a linha da tabela '" + nome + "' deve ser um objeto json");
    }
    Conteudo conteudo = le();
    const int id = conteudo.proximo_id;
    if (id == std::numeric_limits<int>::max()) {
        throw ErroPersistencia("tabela '" + nome + "': limite de registros atingido");
    }
    linha.define("id", id);
    conteudo.linhas.push_back(linha);
    conteudo.proximo_id = id + 1;
    grava(conteudo);
    return id;
}

bool Tabela::atualiza(const Json& linha) {
    if (!linha.eh_objeto() || !linha.tem_campo("id")) {
        throw ErroValidacao("a linha da tabela '" + nome + "' precisa do campo 'id'");
    }
    const int id = linha.campo("id").como_int();
    Conteudo conteudo = le();
    for (Json& existente : conteudo.linhas) {
        if (existente.campo("id").como_int() == id) {
            existente = linha;
            grava(conteudo);
            return true;
        }
    }
    return false;
}

bool Tabela::exclui(int id) {
    Conteudo conteudo = le();
    for (auto it = conteudo.linhas.begin(); it != conteudo.linhas.end(); ++it) {
        if (it->campo("id").como_int() == id) {
            conteudo.linhas.erase(it);
            grava(conteudo);
            return true;
        }
    }
    return false;
}

// ############################## BANCO DE DADO ##############################

BancoDado::BancoDado(const std::string& diretorio) : diretorio(remove_espacos(diretorio)) {
    if (this->diretorio.empty()) {
        throw ErroValidacao("a pasta de dados não pode ficar em branco");
    }
    std::error_code erro;
    fs::create_directories(this->diretorio, erro);
    if (erro || !fs::is_directory(this->diretorio, erro)) {
        throw ErroPersistencia("não foi possível criar a pasta de dados '" + this->diretorio + "'" +
                               (erro ? ": " + erro.message() : ""));
    }
}

Tabela BancoDado::get_tabela(const std::string& nome) const {
    const bool valido = !nome.empty() && std::all_of(nome.begin(), nome.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || c == '_';
    });
    if (!valido) {
        throw ErroValidacao("nome de tabela inválido: '" + nome + "'");
    }
    return Tabela(caminho_arquivo(nome + ".json"), nome);
}

std::string BancoDado::caminho_arquivo(const std::string& nome_arquivo) const {
    return (fs::path(diretorio) / nome_arquivo).string();
}

const std::string& BancoDado::get_diretorio() const { return diretorio; }
