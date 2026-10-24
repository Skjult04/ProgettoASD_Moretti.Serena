#include "ExperimentalAnalysis.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

// ExperimentalAnalysis1: statistiche del grafo

GraphStats compute_stats(const AsGraph& g) {
    return {g.count_nodes(), g.count_unique_edges()};
}

// ExperimentalAnalysis2: distribuzione delle frequenze

void build_histogram(const AsGraph& g,
                     std::vector<std::pair<int, int>>& histogram,
                     std::vector<int>& distinctFreq) {
    histogram.clear();
    distinctFreq.clear();

    const auto& adj = g.get_adj();
    const int n = g.count_nodes();

    // Ogni arco compare in adj due volte: si conta solo quando v > u.
    std::vector<int> weights;
    weights.reserve(g.count_unique_edges());
    for (int u = 0; u < n; ++u)
        for (const auto& [v, w] : adj[u])
            if (v > u) weights.push_back(w);

    std::sort(weights.begin(), weights.end());

    // Scansione delle run di valori uguali.
    for (size_t i = 0; i < weights.size();) {
        size_t j = i;
        while (j < weights.size() && weights[j] == weights[i]) ++j;
        histogram.push_back({weights[i], static_cast<int>(j - i)});
        distinctFreq.push_back(weights[i]);
        i = j;
    }
}

// ExperimentalAnalysis3: misura dei tempi

double ms_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - start)
        .count();
}

// ExperimentalAnalysis4: esperimenti sulle query

// Due rand() combinate: con RAND_MAX = 32767 una sola chiamata non coprirebbe
// tutti i nodi di un grafo AS.
int random_index(int n) {
    return static_cast<int>((rand() * (RAND_MAX + 1LL) + rand()) % n);
}

void run_queries(const AsGraph& g, const MiniMax& mm, int Q,
                 std::vector<std::pair<int, int>>& queries,
                 std::vector<int>& costs, double& totalMs) {
    queries.clear();
    costs.clear();
    totalMs = 0.0;

    const int n = g.count_nodes();
    if (n < 2 || Q <= 0) return;

    // Le coppie sono generate prima di cronometrare.
    queries.reserve(Q);
    while (static_cast<int>(queries.size()) < Q) {
        int u = random_index(n);
        int v = random_index(n);
        if (u == v) continue;
        queries.push_back({u, v});
    }

    costs.resize(Q);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < Q; ++i)
        costs[i] = mm.query(queries[i].first, queries[i].second);
    totalMs = ms_since(t0);
}

int query_by_id(const AsGraph& g, const MiniMax& mm, int as_u, int as_v) {
    return mm.query(g.get_index(as_u), g.get_index(as_v));
}

// ExperimentalAnalysis5: verifica di correttezza

// true se v è raggiungibile da u usando solo archi di frequenza <= t.
bool connected_upto(const AsGraph& g, int u, int v, int t,
                    std::vector<char>& visited, std::vector<int>& queue) {
    if (u == v) return true;

    const auto& adj = g.get_adj();
    std::fill(visited.begin(), visited.end(), 0);
    queue.clear();
    queue.push_back(u);
    visited[u] = 1;

    for (size_t head = 0; head < queue.size(); ++head) {
        int x = queue[head];
        for (const auto& [y, w] : adj[x]) {
            if (w > t || visited[y]) continue;
            if (y == v) return true;
            visited[y] = 1;
            queue.push_back(y);
        }
    }
    return false;
}

// Precondizioni: distinctFreq non vuoto, u e v indici validi della LCC.
int minimax_by_search(const AsGraph& g, const std::vector<int>& distinctFreq,
                      int u, int v, std::vector<char>& visited,
                      std::vector<int>& queue) {
    // Senza questo controllo la ricerca restituirebbe distinctFreq[0].
    if (u == v) return 0;

    int lo = 0;
    int hi = static_cast<int>(distinctFreq.size()) - 1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (connected_upto(g, u, v, distinctFreq[mid], visited, queue))
            hi = mid;
        else
            lo = mid + 1;
    }
    return distinctFreq[lo];
}

// Numero di discordanze sulle prime min(S, Q) coppie (atteso: 0).
int verify_queries(const AsGraph& g, const std::vector<int>& distinctFreq,
                   const std::vector<std::pair<int, int>>& queries,
                   const std::vector<int>& costs, int S) {
    if (distinctFreq.empty() || queries.empty() || S <= 0) return 0;

    const int limit = std::min(S, static_cast<int>(queries.size()));
    std::vector<char> visited(g.count_nodes(), 0);
    std::vector<int> queue;

    int mismatches = 0;
    for (int i = 0; i < limit; ++i) {
        int expected = minimax_by_search(g, distinctFreq, queries[i].first,
                                         queries[i].second, visited, queue);
        if (expected != costs[i]) ++mismatches;
    }
    return mismatches;
}

// ExperimentalAnalysis6: report dei risultati

void write_histogram_csv(const std::string& filename,
                         const std::vector<std::pair<int, int>>& histogram) {
    std::ofstream out(filename);
    if (!out.is_open())
        throw std::runtime_error("impossibile scrivere il file " + filename);
    for (const auto& [f, c] : histogram) out << f << ',' << c << '\n';
}

void print_report(const GraphStats& full, const GraphStats& lcc,
                  const std::vector<std::pair<int, int>>& histogram,
                  const Timings& t, int Q, double queryMs,
                  const std::vector<int>& costs, int S, int mismatches) {
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "=== Grafo ===\n";
    std::cout << "Grafo completo: " << full.nodes << " nodi, " << full.edges
              << " archi\n";
    std::cout << "LCC:            " << lcc.nodes << " nodi, " << lcc.edges
              << " archi\n";
    if (full.nodes > 0 && full.edges > 0) {
        std::cout << "La LCC contiene il " << 100.0 * lcc.nodes / full.nodes
                  << "% dei nodi e il " << 100.0 * lcc.edges / full.edges
                  << "% degli archi\n";
    }

    if (lcc.edges > 0 && !histogram.empty()) {
        const int fmax = histogram.back().first;
        const int ones =
            histogram.front().first == 1 ? histogram.front().second : 0;
        std::cout << "\n=== Frequenze (LCC) ===\n";
        std::cout << "Frequenza massima: " << fmax << "\n";
        std::cout << "Archi con frequenza 1: " << ones << " ("
                  << 100.0 * ones / lcc.edges << "%)\n";
        std::cout << "Frequenze distinte: " << histogram.size()
                  << " (istogramma completo in istogramma.csv)\n";
    }

    std::cout << "\n=== Tempi (ms) ===\n";
    std::cout << "Lettura e conteggio frequenze: " << t.read << "\n";
    std::cout << "Costruzione del grafo:         " << t.build << "\n";
    std::cout << "Estrazione della LCC:          " << t.lcc << "\n";
    std::cout << "Costruzione di MiniMax:        " << t.minimax << "\n";

    if (Q > 0 && !costs.empty()) {
        const auto [mn, mx] = std::minmax_element(costs.begin(), costs.end());
        double sum = 0.0;
        for (int c : costs) sum += c;

        std::cout << "\n=== Query minimax ===\n";
        std::cout << "Query eseguite: " << Q << "\n";
        std::cout << "Costo ottimo minimo / massimo / medio: " << *mn << " / "
                  << *mx << " / " << sum / costs.size() << "\n";
        std::cout << "Tempo medio per query: " << queryMs * 1000.0 / Q
                  << " us\n";
        if (S > 0)
            std::cout << "Verifica con BFS: " << mismatches
                      << " discordanze su " << S << " query\n";
        else
            std::cout << "Verifica con BFS: non eseguita\n";
    }
}
// run_experiments

void run_experiments(const std::string& filename, int Q, int S,
                     unsigned int seed) {
    // srand prima di creare AsGraph, che usa rand() per l'hash universale.
    srand(seed);
    AsGraph g;

    Timings t{0.0, 0.0, 0.0, 0.0};
    std::vector<std::pair<int, int>> histogram;
    std::vector<int> distinctFreq;
    std::vector<std::pair<int, int>> queries;
    std::vector<int> costs;
    double queryMs = 0.0;

    auto t0 = std::chrono::steady_clock::now();
    g.count_frequencies(filename);
    t.read = ms_since(t0);

    t0 = std::chrono::steady_clock::now();
    g.construct_graph();
    t.build = ms_since(t0);

    const GraphStats full = compute_stats(g);

    t0 = std::chrono::steady_clock::now();
    g.extract_lcc();
    t.lcc = ms_since(t0);

    const GraphStats lcc = compute_stats(g);
    if (lcc.nodes == 0) {
        print_report(full, lcc, histogram, t, 0, 0.0, costs, 0, 0);
        return;
    }

    build_histogram(g, histogram, distinctFreq);

    t0 = std::chrono::steady_clock::now();
    MiniMax mm(g.get_adj());
    t.minimax = ms_since(t0);

    run_queries(g, mm, Q, queries, costs, queryMs);

    const int executed = static_cast<int>(queries.size());
    const int verified = std::max(0, std::min(S, executed));
    const int mismatches = verify_queries(g, distinctFreq, queries, costs, S);

    write_histogram_csv("istogramma.csv", histogram);
    print_report(full, lcc, histogram, t, executed, queryMs, costs, verified,
                 mismatches);

    // Query d'esempio con gli ID originali degli AS.
    const int examples = std::min(5, executed);
    if (examples > 0) std::cout << "\n=== Esempi ===\n";
    for (int i = 0; i < examples; ++i) {
        int as_u = g.get_original_id(queries[i].first);
        int as_v = g.get_original_id(queries[i].second);
        std::cout << "AS" << as_u << " -> AS" << as_v
                  << ": costo minimax ottimo " << query_by_id(g, mm, as_u, as_v)
                  << "\n";
    }
}
