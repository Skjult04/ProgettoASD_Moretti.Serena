#include "AsGraph.hpp"
#include <iostream>
#include <fstream>
#include <string>

int main() {
    // 1. Creiamo il file di test con i sample BGP forniti
    std::string filename = "sample_bgp_full.txt";
    std::ofstream out(filename);
    if (out.is_open()) {
        out << "# Sample BGP paths per test dell'esempio del documento\n"
            << "routeviews/test 1|2|5 192.168.0.0/24 i 0.0.0.0\n"
            << "routeviews/test 1|2|5 192.168.0.0/24 i 0.0.0.0\n"
            << "routeviews/test 1|3|4|5 192.168.0.0/24 i 0.0.0.0\n"
            << "routeviews/test 1|3|5 192.168.0.0/24 i 0.0.0.0\n"
            << "routeviews/test 2|3|4 192.168.0.0/24 i 0.0.0.0\n"
            << "routeviews/test 3|4|5 192.168.0.0/24 i 0.0.0.0\n";
        out.close();
        std::cout << "File '" << filename << "' creato con successo.\n\n";
    }

    AsGraph graph;

    try {
        std::cout << "--- FASE 1: Lettura file e conteggio frequenze ---\n";
        graph.count_frequencies(filename);
        std::cout << "Conteggio completato.\n\n";

        std::cout << "--- FASE 2: Costruzione del grafo (mapping ID e Adiacenze) ---\n";
        graph.construct_graph();
        std::cout << "Grafo costruito.\n\n";

        std::cout << "--- FASE 3: Estrazione della Componente Connessa Piu' Grande (LCC) ---\n";
        graph.extract_lcc();
        std::cout << "Estrazione completata.\n\n";

        // Verifica finale utilizzando il metodo getter che hai fornito
        int total_edges = graph.count_unique_edges();
        std::cout << "=== RISULTATI FINALI ===\n";
        std::cout << "Totale archi univoci nella LCC: " << total_edges << " (Atteso: 7)\n\n";

        // Per vedere effettivamente la struttura del grafo stampiamo le adiacenze
        std::cout << "Struttura del Grafo (Lista di Adiacenza):\n";
        graph.print_graph_debug();

    } catch (const std::exception& e) {
        std::cerr << "ERRORE DURANTE L'ESECUZIONE: " << e.what() << '\n';
    }

    return 0;
}