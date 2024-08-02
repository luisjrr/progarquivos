#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>
#include <iomanip>

struct Transacao
{
    int dia, mes, ano;
    int agencia_origem, conta_origem;
    double valor;
    int agencia_destino, conta_destino;
};

struct MovimentacaoConsolidada
{
    int agencia, conta;
    double subtotal_especie = 0.0;
    double subtotal_eletronica = 0.0;
    int total_transacoes = 0;
};

void carregarTransacoes(const std::string &arquivoCSV, std::vector<Transacao> &transacoes)
{
    std::ifstream file(arquivoCSV);
    std::string linha, campo;
    while (std::getline(file, linha))
    {
        std::stringstream ss(linha);
        Transacao t;
        std::getline(ss, campo, ',');
        t.dia = std::stoi(campo);
        std::getline(ss, campo, ',');
        t.mes = std::stoi(campo);
        std::getline(ss, campo, ',');
        t.ano = std::stoi(campo);
        std::getline(ss, campo, ',');
        t.agencia_origem = std::stoi(campo);
        std::getline(ss, campo, ',');
        t.conta_origem = std::stoi(campo);
        std::getline(ss, campo, ',');
        t.valor = std::stod(campo);
        std::getline(ss, campo, ',');
        t.agencia_destino = std::stoi(campo);
        std::getline(ss, campo, ',');
        t.conta_destino = std::stoi(campo);
        transacoes.push_back(t);
    }
}

void consolidarMovimentacao(const std::vector<Transacao> &transacoes, int mes, int ano, std::map<int, MovimentacaoConsolidada> &consolidacao)
{
    for (const auto &t : transacoes)
    {
        if (t.mes == mes && t.ano == ano)
        {
            int chave = t.agencia_origem * 1000000 + t.conta_origem; // Cria uma chave única combinando agência e conta
            consolidacao[chave].agencia = t.agencia_origem;
            consolidacao[chave].conta = t.conta_origem;
            if (t.agencia_destino == 0 && t.conta_destino == 0)
            {
                consolidacao[chave].subtotal_especie += t.valor;
            }
            else
            {
                consolidacao[chave].subtotal_eletronica += t.valor;
            }
            consolidacao[chave].total_transacoes++;
        }
    }
}

void salvarConsolidacaoBinaria(const std::map<int, MovimentacaoConsolidada> &consolidacao, int mes, int ano)
{
    std::ofstream binFile("consolidadas" + std::to_string(mes) + std::to_string(ano) + ".bin", std::ios::binary);
    for (const auto &entry : consolidacao)
    {
        binFile.write(reinterpret_cast<const char *>(&entry.second), sizeof(MovimentacaoConsolidada));
    }
}

bool carregarConsolidacaoBinaria(std::map<int, MovimentacaoConsolidada> &consolidacao, int mes, int ano)
{
    std::ifstream binFile("consolidadas" + std::to_string(mes) + std::to_string(ano) + ".bin", std::ios::binary);
    if (!binFile)
        return false;
    MovimentacaoConsolidada mov;
    while (binFile.read(reinterpret_cast<char *>(&mov), sizeof(MovimentacaoConsolidada)))
    {
        int chave = mov.agencia * 1000000 + mov.conta;
        consolidacao[chave] = mov;
    }
    return true;
}

void atualizarLog(const std::string &mensagem)
{
    std::ofstream logFile("log.txt", std::ios::app);
    std::time_t t = std::time(nullptr);
    logFile << std::put_time(std::localtime(&t), "%c") << ": " << mensagem << std::endl;
}

void realizarConsulta(const std::vector<Transacao> &transacoes, int mes, int ano)
{
    std::map<int, MovimentacaoConsolidada> consolidacao;
    if (!carregarConsolidacaoBinaria(consolidacao, mes, ano))
    {
        consolidarMovimentacao(transacoes, mes, ano, consolidacao);
        salvarConsolidacaoBinaria(consolidacao, mes, ano);
        atualizarLog("Consulta realizada para " + std::to_string(mes) + "/" + std::to_string(ano));
    }
    else
    {
        atualizarLog("Consulta carregada de arquivo binário para " + std::to_string(mes) + "/" + std::to_string(ano));
    }
    for (const auto &entry : consolidacao)
    {
        std::cout << "Agência: " << entry.second.agencia << ", Conta: " << entry.second.conta
                  << ", Especie: " << entry.second.subtotal_especie
                  << ", Eletrônica: " << entry.second.subtotal_eletronica
                  << ", Total Transações: " << entry.second.total_transacoes << std::endl;
    }
}

void filtrarMovimentacao(int mes, int ano, double x, double y, const std::string &tipoFiltro)
{
    std::map<int, MovimentacaoConsolidada> consolidacao;
    if (!carregarConsolidacaoBinaria(consolidacao, mes, ano))
    {
        atualizarLog("Consolidacao não encontrada para " + std::to_string(mes) + "/" + std::to_string(ano));
        return;
    }
    int count = 0;
    for (const auto &entry : consolidacao)
    {
        bool condicao = (tipoFiltro == "E") ? (entry.second.subtotal_especie >= x && entry.second.subtotal_eletronica >= y) : (entry.second.subtotal_especie >= x || entry.second.subtotal_eletronica >= y);
        if (condicao)
        {
            std::cout << "Agência: " << entry.second.agencia << ", Conta: " << entry.second.conta
                      << ", Especie: " << entry.second.subtotal_especie
                      << ", Eletrônica: " << entry.second.subtotal_eletronica
                      << ", Total Transações: " << entry.second.total_transacoes << std::endl;
            count++;
        }
    }
    atualizarLog("Filtragem realizada para " + std::to_string(mes) + "/" + std::to_string(ano) +
                 " com X=" + std::to_string(x) + ", Y=" + std::to_string(y) + ", Tipo: " + tipoFiltro +
                 ". Registros encontrados: " + std::to_string(count));
}

int main()
{
    std::vector<Transacao> transacoes;
    carregarTransacoes("transacoes.csv", transacoes);

    int mes, ano;
    std::cout << "Digite o mês e o ano para a consulta: ";
    std::cin >> mes >> ano;
    realizarConsulta(transacoes, mes, ano);

    double x, y;
    std::string tipoFiltro;
    std::cout << "Digite os valores de X e Y para a filtragem e o tipo de filtro (E/OU): ";
    std::cin >> x >> y >> tipoFiltro;
    filtrarMovimentacao(mes, ano, x, y, tipoFiltro);

    return 0;
}