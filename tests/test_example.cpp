#include "AsGraph.hpp"
#include <iostream>
#include <cassert> //Invece di limitarsi a stampare i risultati, usa la macro assert()
                  //In caso di errore, l programma si bloccherà immediatamente indicando la riga esatta dell'errore.
int main() {
    AsGraph graph;

    // 1. Carica il file di sample
    std::cout << "Caricamento dataset di test...\n";
    graph.count_frequencies("data/sample_paths.txt");

    // 2. Costruisci il grafo
    graph.construct_graph();

    // 3. Estrai la LCC (nel nostro esempio tutti i nodi sono connessi)
    graph.extract_lcc();

    // 4. Verification delle proprietà
    std::cout << "Numero di archi unici: " << graph.count_unique_edges() << " (Atteso: 7)\n";
    assert(graph.count_unique_edges() == 7);

    // Verifica la presenza di tutti i nodi 1..5
    for (int id = 1; id <= 5; ++id) {
        int idx = graph.get_index(id);
        std::cout << "AS " << id << " -> indice compatto: " << idx << "\n";
        assert(idx != -1);
    }

    std::cout << "Test del caricamento completato con successo!\n";
    return 0;
}