#ifndef EXPERIMENTAL_ANALYSIS_HPP
#define EXPERIMENTAL_ANALYSIS_HPP

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "AsGraph.hpp"
#include "MiniMax.hpp"

// Modulo ExperimentalAnalysis: statistiche, istogramma delle frequenze, tempi,
// query casuali, verifica di correttezza e report.

// ExperimentalAnalysis1
struct GraphStats {
    int nodes;
    int edges;
};

// Tempi delle fasi, in millisecondi.
struct Timings {
    double read, build, lcc, minimax;
};

// ExperimentalAnalysis1: numero di nodi e di archi del grafo corrente.
GraphStats compute_stats(const AsGraph& g);

// ExperimentalAnalysis2: histogram = coppie (frequenza, numero di archi) in
// ordine crescente di frequenza; distinctFreq = le frequenze presenti.
void build_histogram(const AsGraph& g,
                     std::vector<std::pair<int, int>>& histogram,
                     std::vector<int>& distinctFreq);

// ExperimentalAnalysis3: millisecondi trascorsi da start.
double ms_since(std::chrono::steady_clock::time_point start);

// ExperimentalAnalysis4: query casuali.
// Precondizione di random_index: n > 0.
int random_index(int n);
void run_queries(const AsGraph& g, const MiniMax& mm, int Q,
                 std::vector<std::pair<int, int>>& queries,
                 std::vector<int>& costs, double& totalMs);
int query_by_id(const AsGraph& g, const MiniMax& mm, int as_u, int as_v);

// ExperimentalAnalysis5: verifica con ricerca binaria sulle frequenze e BFS.
bool connected_upto(const AsGraph& g, int u, int v, int t,
                    std::vector<char>& visited, std::vector<int>& queue);
int minimax_by_search(const AsGraph& g, const std::vector<int>& distinctFreq,
                      int u, int v, std::vector<char>& visited,
                      std::vector<int>& queue);
int verify_queries(const AsGraph& g, const std::vector<int>& distinctFreq,
                   const std::vector<std::pair<int, int>>& queries,
                   const std::vector<int>& costs, int S);

// ExperimentalAnalysis6: output dei risultati.
void write_histogram_csv(const std::string& filename,
                         const std::vector<std::pair<int, int>>& histogram);
void print_report(const GraphStats& full, const GraphStats& lcc,
                  const std::vector<std::pair<int, int>>& histogram,
                  const Timings& t, int Q, double queryMs,
                  const std::vector<int>& costs, int S, int mismatches);

// Esegue l'intero esperimento (vedi README, flusso di run_experiments).
void run_experiments(const std::string& filename, int Q, int S,
                     unsigned int seed);

#endif
