#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "ExperimentalAnalysis.hpp"

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::cerr << "FALLITO: " << what << "\n";
        ++failures;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string file =
        argc > 1 ? argv[1] : "data/sample_paths.txt";

    srand(12345);
    AsGraph g;
    g.count_frequencies(file);
    g.construct_graph();
    g.extract_lcc();

    check(g.count_nodes() == 5, "5 nodi");
    check(g.count_unique_edges() == 7, "7 archi");

    std::vector<std::pair<int, int>> histogram;
    std::vector<int> distinctFreq;
    build_histogram(g, histogram, distinctFreq);
    const std::vector<std::pair<int, int>> expected = {{1, 2}, {2, 4}, {3, 1}};
    check(histogram == expected, "istogramma 1,2 / 2,4 / 3,1");

    MiniMax mm(g.get_adj());
    check(query_by_id(g, mm, 1, 5) == 2, "costo(1,5) == 2");
    check(query_by_id(g, mm, 2, 5) == 1, "costo(2,5) == 1");
    check(query_by_id(g, mm, 3, 4) == 2, "costo(3,4) == 2");
    check(query_by_id(g, mm, 1, 1) == 0, "costo(u,u) == 0");
    check(query_by_id(g, mm, 1, 99) == MiniMax::NIL, "AS fuori dal grafo -> NIL");

    // Confronto esaustivo con il metodo indipendente dall'MST.
    const int n = g.count_nodes();
    std::vector<char> visited(n, 0);
    std::vector<int> queue;
    for (int u = 0; u < n; ++u)
        for (int v = 0; v < n; ++v)
            check(mm.query(u, v) ==
                      minimax_by_search(g, distinctFreq, u, v, visited, queue),
                  "query == minimax_by_search su tutte le coppie");

    if (failures == 0) std::cout << "tutti i test superati\n";
    return failures == 0 ? 0 : 1;
}