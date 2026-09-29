#ifndef AS_GRAPH_HPP
#define AS_GRAPH_HPP

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <utility>

class AsGraph {
private:
    // Step 1.1: Mappa temporanea per frequenze archi (min_AS, max_AS) -> Frequenza
    std::map<std::pair<int, int>, int> edge_frequencies;

    // Step 1.2: Tabelle di mappatura ID e Lista di Adiacenza
    std::unordered_map<int, int> id_to_index; // ID originale AS -> Indice 0..V-1
    std::vector<int> index_to_id;             // Indice 0..V-1 -> ID originale AS
    
    // Lista di adiacenza: adj[u] = lista di coppie (v_id, peso/frequenza)
    std::vector<std::vector<std::pair<int, int>>> adj;

    // Helper interno per la Fase 2
    int helper(int as_number);

public:
    AsGraph() = default;

    // Step 1.3: Lettura file e conteggio frequenze
    void count_frequencies(const std::string& filename);

    // Step 1.4: Costruzione del grafo mappato
    void construct_graph();

    // Step 1.5: Filtro per la Componente Connessa Più Grande (LCC)
    void extract_lcc();

    // Getter utilità per gli altri moduli (MiniMax, Experimental_Analysis)
    int count_nodes() const { return static_cast<int>(adj.size()); }
    size_t count_unique_edges() const { return edge_frequencies.size(); }
    const std::vector<std::vector<std::pair<int, int>>>& get_adj() const { return adj; }
    int get_original_id(int internal_id) const { return index_to_id[internal_id]; }
};

#endif