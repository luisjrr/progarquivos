#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>
#include <iomanip>

// Estrutura para armazenar os dados de cada transação
struct Transacao
{
    int dia, mes, ano;
    std::string agencia_origem, conta_origem;
    double valor;
    std::string agencia_destino, conta_destino;
};

// Estrutura para armazenar os dados consolidados por agência e conta
struct MovimentacaoConsolidada
{
    std::string agencia, conta;
    double subtotal_especie = 0.0;
    double subtotal_eletronica = 0.0;
    int total_transacoes = 0;
};

// Função para carregar transações de um arquivo CSV
void carregarTransacoes(const std::string &arquivoCSV, std::vector<Transacao> &transacoes)
{
    std::ifstream file(arquivoCSV); // Abre o arquivo CSV para leitura
    std::string linha, campo;
    while (std::getline(file, linha))
    { // Lê cada linha do arquivo CSV
        std::stringstream ss(linha);
        Transacao t;
        std::getline(ss, campo, ',');
        t.dia = std::stoi(campo); // Lê e converte o dia
        std::getline(ss, campo, ',');
        t.mes = std::stoi(campo); // Lê e converte o mês
        std::getline(ss, campo, ',');
        t.ano = std::stoi(campo); // Lê e converte o ano
        std::getline(ss, campo, ',');
        t.agencia_origem = campo; // Lê a agência de origem
        std::getline(ss, campo, ',');
        t.conta_origem = campo; // Lê a conta de origem
        std::getline(ss, campo, ',');
        t.valor = std::stod(campo); // Lê e converte o valor
        std::getline(ss, campo, ',');
        t.agencia_destino = campo; // Lê a agência de destino
        std::getline(ss, campo, ',');
        t.conta_destino = campo; // Lê a conta de destino
        transacoes.push_back(t); // Adiciona a transação ao vetor de transações
    }
}

// Função para consolidar as transações por mês e ano
void consolidarMovimentacao(const std::vector<Transacao> &transacoes, int mes, int ano, std::map<std::string, MovimentacaoConsolidada> &consolidacao)
{
    for (const auto &t : transacoes)
    {
        if (t.mes == mes && t.ano == ano)
        {                                                                // Verifica se a transação é do mês e ano especificados
            std::string chave = t.agencia_origem + "-" + t.conta_origem; // Cria uma chave única para a agência e conta de origem
            consolidacao[chave].agencia = t.agencia_origem;              // Armazena a agência de origem
            consolidacao[chave].conta = t.conta_origem;                  // Armazena a conta de origem
            if (t.agencia_destino.empty() && t.conta_destino.empty())
            {
                // Se não há agência e conta de destino, é uma transação em espécie
                consolidacao[chave].subtotal_especie += t.valor;
            }
            else
            {
                // Caso contrário, é uma transação eletrônica
                consolidacao[chave].subtotal_eletronica += t.valor;
            }
            consolidacao[chave].total_transacoes++; // Incrementa o total de transações
        }
    }
}

// Função para salvar a consolidação em um arquivo binário
void salvarConsolidacaoBinaria(const std::map<std::string, MovimentacaoConsolidada> &consolidacao, int mes, int ano)
{
    std::ofstream binFile("consolidadas" + std::to_string(mes) + std::to_string(ano) + ".bin", std::ios::binary); // Cria o arquivo binário
    for (const auto &entry : consolidacao)
    {
        binFile.write(reinterpret_cast<const char *>(&entry.second), sizeof(MovimentacaoConsolidada)); // Escreve cada movimentação no arquivo
    }
}

// Função para carregar a consolidação de um arquivo binário
bool carregarConsolidacaoBinaria(std::map<std::string, MovimentacaoConsolidada> &consolidacao, int mes, int ano)
{
    std::ifstream binFile("consolidadas" + std::to_string(mes) + std::to_string(ano) + ".bin", std::ios::binary); // Abre o arquivo binário
    if (!binFile)
        return false; // Retorna falso se o arquivo não puder ser aberto
    MovimentacaoConsolidada mov;
    while (binFile.read(reinterpret_cast<char *>(&mov), sizeof(MovimentacaoConsolidada)))
    {                                                      // Lê cada movimentação do arquivo
        std::string chave = mov.agencia + "-" + mov.conta; // Cria a chave única
        consolidacao[chave] = mov;                         // Armazena a movimentação no mapa
    }
    return true; // Retorna verdadeiro se a leitura foi bem-sucedida
}

// Função para atualizar o arquivo de log com uma mensagem
void atualizarLog(const std::string &mensagem)
{
    std::ofstream logFile("log.txt", std::ios::app); // Abre o arquivo de log para adicionar
    std::time_t t = std::time(nullptr);
    logFile << std::put_time(std::localtime(&t), "%c") << ": " << mensagem << std::endl; // Escreve a mensagem com a data e hora atuais
}

// Função para realizar a consulta de movimentação consolidada
void realizarConsulta(const std::vector<Transacao> &transacoes, int mes, int ano)
{
    std::map<std::string, MovimentacaoConsolidada> consolidacao;
    if (!carregarConsolidacaoBinaria(consolidacao, mes, ano))
    {                                                                                               // Verifica se a consolidação já existe
        consolidarMovimentacao(transacoes, mes, ano, consolidacao);                                 // Consolida as movimentações
        salvarConsolidacaoBinaria(consolidacao, mes, ano);                                          // Salva a consolidação em um arquivo binário
        atualizarLog("Consulta realizada para " + std::to_string(mes) + "/" + std::to_string(ano)); // Atualiza o log
    }
    else
    {
        atualizarLog("Consulta carregada de arquivo binário para " + std::to_string(mes) + "/" + std::to_string(ano)); // Atualiza o log
    }
    // Exibe os resultados da consulta
    for (const auto &entry : consolidacao)
    {
        std::cout << "Agência: " << entry.second.agencia << ", Conta: " << entry.second.conta
                  << ", Especie: " << entry.second.subtotal_especie
                  << ", Eletrônica: " << entry.second.subtotal_eletronica
                  << ", Total Transações: " << entry.second.total_transacoes << std::endl;
    }
}

// Função para filtrar as movimentações consolidadas com base nos critérios especificados
void filtrarMovimentacao(int mes, int ano, double x, double y, const std::string &tipoFiltro)
{
    std::map<std::string, MovimentacaoConsolidada> consolidacao;
    if (!carregarConsolidacaoBinaria(consolidacao, mes, ano))
    {                                                                                                        // Verifica se a consolidação já existe
        atualizarLog("Consolidacao não encontrada para " + std::to_string(mes) + "/" + std::to_string(ano)); // Atualiza o log
        return;
    }
    int count = 0;
    // Filtra e exibe as movimentações que atendem aos critérios
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
    // Atualiza o log com os resultados da filtragem
    atualizarLog("Filtragem realizada para " + std::to_string(mes) + "/" + std::to_string(ano) +
                 " com X=" + std::to_string(x) + ", Y=" + std::to_string(y) + ", Tipo: " + tipoFiltro +
                 ". Registros encontrados: " + std::to_string(count));
}

// Função principal
int main()
{
    std::vector<Transacao> transacoes;
    carregarTransacoes("transacoes.csv", transacoes); // Carrega as transações do arquivo CSV

    int mes, ano;
    std::cout << "Digite o mês e o ano para a consulta: ";
    std::cin >> mes >> ano;
    realizarConsulta(transacoes, mes, ano); // Realiza a consulta para o mês e ano especificados

    double x, y;
    std::string tipoFiltro;
    std::cout << "Digite os valores de X e Y para a filtragem e o tipo de filtro (E/OU): ";
    std::cin >> x >> y >> tipoFiltro;
    filtrarMovimentacao(mes, ano, x, y, tipoFiltro); // Realiza a filtragem com os critérios especificados

    return 0;
}