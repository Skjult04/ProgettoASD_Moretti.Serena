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