#include <iostream> // Biblioteca para entrada e saída padrão
#include <fstream>  // Biblioteca para manipulação de arquivos
#include <sstream>  // Biblioteca para manipulação de strings
#include <vector>   // Biblioteca para usar vetores
#include <map>      // Biblioteca para usar mapas (dicionários)
#include <ctime>    // Biblioteca para manipulação de tempo
#include <iomanip>  // Biblioteca para manipulação de formatação de entrada/saída

// Estrutura para armazenar informações de uma transação
struct Transacao
{
    int dia, mes, ano;                  // Data da transação
    int agencia_origem, conta_origem;   // Dados da conta de origem
    double valor;                       // Valor da transação
    int agencia_destino, conta_destino; // Dados da conta de destino
};

// Estrutura para armazenar informações consolidadas de movimentações
struct MovimentacaoConsolidada
{
    int agencia, conta;               // Agência e conta
    double subtotal_especie = 0.0;    // Subtotal de transações em espécie
    double subtotal_eletronica = 0.0; // Subtotal de transações eletrônicas
    int total_transacoes = 0;         // Total de transações
};

// Função para carregar transações de um arquivo CSV
void carregarTransacoes(const std::string &arquivoCSV, std::vector<Transacao> &transacoes)
{
    std::ifstream file(arquivoCSV); // Abre o arquivo CSV
    std::string linha, campo;       // Variáveis para armazenar linha e campo

    // Lê cada linha do arquivo CSV
    while (std::getline(file, linha))
    {
        std::stringstream ss(linha); // Cria um stream para a linha
        Transacao t;                 // Cria uma transação

        // Lê cada campo da linha, separado por vírgulas, e atribui aos membros da transação
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
        t.agencia_destino = campo.empty() ? 0 : std::stoi(campo);

        std::getline(ss, campo, ',');
        t.conta_destino = campo.empty() ? 0 : std::stoi(campo);

        // Adiciona a transação ao vetor de transações
        transacoes.push_back(t);
    }
}

// Função para consolidar movimentações de um determinado mês e ano
void consolidarMovimentacao(const std::vector<Transacao> &transacoes, int mes, int ano, std::map<int, MovimentacaoConsolidada> &consolidacao)
{
    // Percorre todas as transações
    for (const auto &t : transacoes)
    {
        // Verifica se a transação é do mês e ano especificados
        if (t.mes == mes && t.ano == ano)
        {
            int chave = t.agencia_origem * 1000000 + t.conta_origem; // Cria uma chave única combinando agência e conta
            consolidacao[chave].agencia = t.agencia_origem;
            consolidacao[chave].conta = t.conta_origem;

            // Verifica se a transação é em espécie (sem conta de destino)
            if (t.agencia_destino == 0 && t.conta_destino == 0)
            {
                consolidacao[chave].subtotal_especie += t.valor;
            }
            else
            {
                // Transação eletrônica (com conta de destino)
                consolidacao[chave].subtotal_eletronica += t.valor;
            }
            consolidacao[chave].total_transacoes++; // Incrementa o total de transações
        }
    }
}

// Função para salvar a consolidação em um arquivo binário
void salvarConsolidacaoBinaria(const std::map<int, MovimentacaoConsolidada> &consolidacao, int mes, int ano)
{
    std::ofstream file("consolidacao_" + std::to_string(mes) + "_" + std::to_string(ano) + ".bin", std::ios::binary); // Abre o arquivo binário para escrita

    // Escreve cada entrada da consolidação no arquivo binário
    for (const auto &entry : consolidacao)
    {
        file.write(reinterpret_cast<const char *>(&entry.second), sizeof(MovimentacaoConsolidada));
    }
}

// Função para carregar a consolidação de um arquivo binário
bool carregarConsolidacaoBinaria(std::map<int, MovimentacaoConsolidada> &consolidacao, int mes, int ano)
{
    std::ifstream file("consolidacao_" + std::to_string(mes) + "_" + std::to_string(ano) + ".bin", std::ios::binary); // Abre o arquivo binário para leitura

    // Verifica se o arquivo foi aberto com sucesso
    if (!file)
    {
        return false;
    }

    MovimentacaoConsolidada mov;
    // Lê cada entrada do arquivo binário e adiciona ao mapa de consolidação
    while (file.read(reinterpret_cast<char *>(&mov), sizeof(MovimentacaoConsolidada)))
    {
        int chave = mov.agencia * 1000000 + mov.conta; // Cria uma chave única combinando agência e conta
        consolidacao[chave] = mov;
    }

    return true;
}

// Função para atualizar o log com uma mensagem
void atualizarLog(const std::string &mensagem)
{
    std::ofstream logFile("log.txt", std::ios::app);                                                       // Abre o arquivo de log em modo append
    std::time_t now = std::time(nullptr);                                                                  // Obtém o horário atual
    logFile << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << " - " << mensagem << std::endl; // Escreve a mensagem no log com timestamp
}

// Função para consultar a movimentação consolidada de um determinado mês e ano
void consultarMovimentacao(int mes, int ano)
{
    std::map<int, MovimentacaoConsolidada> consolidacao;
    if (!carregarConsolidacaoBinaria(consolidacao, mes, ano))
    {
        atualizarLog("Consolidacao nao encontrada para " + std::to_string(mes) + "/" + std::to_string(ano));
        return;
    }

    for (const auto &entry : consolidacao)
    {
        const MovimentacaoConsolidada &consolidado = entry.second;
        std::cout << "Agencia: " << consolidado.agencia << ", Conta: " << consolidado.conta << std::endl;
        std::cout << "Subtotal Transacoes em Especie: " << consolidado.subtotal_especie << std::endl;
        std::cout << "Subtotal Transacoes Eletronicas: " << consolidado.subtotal_eletronica << std::endl;
        std::cout << "Total Transacoes: " << consolidado.total_transacoes << std::endl;
    }
}

// Função para filtrar movimentações consolidadas com base em critérios fornecidos
void filtrarMovimentacao(int mes, int ano, double x, double y, const std::string &tipoFiltro)
{
    std::map<int, MovimentacaoConsolidada> consolidacao;
    if (!carregarConsolidacaoBinaria(consolidacao, mes, ano))
    {
        atualizarLog("Consolidacao nao encontrada para " + std::to_string(mes) + "/" + std::to_string(ano));
        return;
    }

    int count = 0;
    for (const auto &entry : consolidacao)
    {
        bool condicao = (tipoFiltro == "E") ? (entry.second.subtotal_especie >= x && entry.second.subtotal_eletronica >= y) : (entry.second.subtotal_especie >= x || entry.second.subtotal_eletronica >= y);
        if (condicao)
        {
            std::cout << "Agencia: " << entry.second.agencia << ", Conta: " << entry.second.conta
                      << ", Especie: " << entry.second.subtotal_especie
                      << ", Eletronica: " << entry.second.subtotal_eletronica
                      << ", Total Transacoes: " << entry.second.total_transacoes << std::endl;
            count++;
        }
    }

    atualizarLog("Filtragem realizada para " + std::to_string(mes) + "/" + std::to_string(ano) +
                 " com X=" + std::to_string(x) + ", Y=" + std::to_string(y) + ", Tipo: " + tipoFiltro +
                 ". Registros encontrados: " + std::to_string(count));
}

// Função principal
int main()
{
    std::vector<Transacao> transacoes;                // Vetor para armazenar as transações
    carregarTransacoes("transacoes.csv", transacoes); // Carrega as transações do arquivo CSV

    int mes, ano;
    std::cout << "Digite o mes e o ano para a consulta: ";
    std::cin >> mes >> ano;
    consultarMovimentacao(mes, ano); // Consulta a movimentação consolidada

    double x, y;
    std::string tipoFiltro;
    std::cout << "Digite os valores de X e Y para a filtragem e o tipo de filtro (E/OU): ";
    std::cin >> x >> y >> tipoFiltro;
    filtrarMovimentacao(mes, ano, x, y, tipoFiltro); // Filtra as movimentações consolidadas

    return 0;
}