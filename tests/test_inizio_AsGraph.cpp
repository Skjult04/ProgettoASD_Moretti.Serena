#include "AsGraph.hpp"
#include <iostream>
#include <algorithm>

int main() {
    std::cout << "=== TEST IdIndexTable ===\n";
    
    IdIndexTable id_table(4);
    
    // I nodi AS estratti dal file di testo sono: 1, 2, 3, 4, 5
    int as_nodes[] = {1, 2, 3, 4, 5};
    
    for (int i = 0; i < 5; ++i) {
        id_table.inserisci(as_nodes[i], i);
        std::cout << "Inserito AS " << as_nodes[i] << " con indice interno " << i << "\n";
    }
    
    std::cout << "\nVerifica delle ricerche nella IdIndexTable:\n";
    std::cout << "Cerco AS 3: " << id_table.cerca(3) << " (Atteso: 2)\n";
    std::cout << "Cerco AS 5: " << id_table.cerca(5) << " (Atteso: 4)\n";
    std::cout << "Cerco AS 99: " << id_table.cerca(99) << " (Atteso: -1)\n";


    std::cout << "\n=== TEST EdgeTable ===\n";
    
    // Inizializziamo con una dimensione ridotta per testare i rehash
    EdgeTable edge_table(4);
    
    // Funzione lambda di supporto per formattare la chiave u < v e incrementare
    auto add_edge = [&](int u, int v) {
        long long key = EdgeTable::encode(std::min(u, v), std::max(u, v));
        edge_table.incrementa(key);
    };

    /* 
     * Simuliamo l'estrazione degli archi dalle route BGP di test:
     * 1) 1|2|5       -> (1,2), (2,5)
     * 2) 1|2|5       -> (1,2), (2,5)
     * 3) 1|3|4|5     -> (1,3), (3,4), (4,5)
     * 4) 1|3|5       -> (1,3), (3,5)
     * 5) 2|3|4       -> (2,3), (3,4)
     * 6) 3|4|5       -> (3,4), (4,5)
     */
    
    // Path 1 e 2
    add_edge(1, 2); add_edge(2, 5);
    add_edge(1, 2); add_edge(2, 5);
    // Path 3
    add_edge(1, 3); add_edge(3, 4); add_edge(4, 5);
    // Path 4
    add_edge(1, 3); add_edge(3, 5);
    // Path 5
    add_edge(2, 3); add_edge(3, 4);
    // Path 6
    add_edge(3, 4); add_edge(4, 5);

    std::cout << "Frequenze degli archi calcolate (ordine dipendente dall'hash):\n";
    
    // Estraiamo e stampiamo il contenuto della tabella hash
    const auto& buckets = edge_table.buckets();
    int edge_count = 0;
    
    for (const auto& bucket : buckets) {
        for (const auto& entry : bucket) {
            int u = EdgeTable::first(entry.key);
            int v = EdgeTable::second(entry.key);
            std::cout << "Arco (" << u << ", " << v << ") -> Frequenza: " << entry.freq << "\n";
            edge_count++;
        }
    }
    
    std::cout << "\nTotale archi univoci processati: " << edge_count << " (Atteso: 7)\n";

    return 0;
}