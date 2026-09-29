#ifndef AS_GRAPH_HPP //serve ad escludere il contenuto del file header se è già stato incluso in precedenza, evitando duplicazioni e conflitti di definizione.
#define AS_GRAPH_HPP //definisce l'etichetta AS_GRAPH_HPP nella memoria del preprocessore, indicando che il file header è stato incluso.

#include <string>
#include <map> //Struttura dati che permette di associare chiavi a valori, in questo caso associa coppie di interi (archi) a frequenze. 
#include <unordered_map> //Tabella hash che calcola un valore per la chiave e posizione l'elemento in un bucket corrispondente.
#include <vector>
#include <utility> //mette a disposizione il tipo di dato std::pair, che rappresenta una coppia di valori, utile per memorizzare archi e frequenze.

class AsGraph {
private:
    // Sottomodulo AsGraph1:
    std::map<std::pair<int, int>, int> edge_frequencies;

    // Sottomodulo AsGraph2:
    std::unordered_map<int, int> id_to_index; // ID originale AS -> Indice 0..V-1
    std::vector<int> index_to_id;             // Indice 0..V-1 -> ID originale AS
    
    //Sottomodulo AsGraph2:
    // Lista di adiacenza: adj[u] = lista di coppie (indice_interno, peso/frequenza)
    std::vector<std::vector<std::pair<int, int>>> adj;
    
    //Sottomodulo AsGraph2:
    // Helper interno per mappare un ID AS a un indice progressivo 0..V-1
    int helper(int as_number);

public:
    AsGraph() = default; //Costruttore di default per la classe AsGraph, che viene chiamato quando si crea un oggetto di questa classe senza parametri.

    // Sottomodulo AsGraph1: Lettura del file e conteggio delle frequenze degli archi
    void count_frequencies(const std::string& filename);

    // Sottomodulo AsGraph2: Costruzione del grafo tramite mappatura degli ID e lista di adiacenza
    void construct_graph();

    // Sottomodulo AsGraph3: Estrazione della componente connessa più grande (LCC)
    void extract_lcc();

    // Getter utilità per gli altri moduli (MiniMax, Experimental_Analysis)

    // Restituisce il numero di nodi nel grafo (dopo eventuale estrazione della LCC)
    int count_nodes() const { return static_cast<int>(adj.size()); } // static_cast<int> serve a convertire il tipo di dato size_t in int, garantendo che il valore restituito sia coerente con il tipo di ritorno della funzione.

    //Conta il numero di archi unici nel grafo (dopo eventuale estrazione della LCC)
    int count_unique_edges() const;

     // Restituisce la lista di adiacenza del grafo (dopo eventuale estrazione della LCC)
    const std::vector<std::vector<std::pair<int, int>>>& get_adj() const { return adj; }
    
    // Restituisce l'ID originale AS dato un indice interno 0..V-1 (dopo eventuale estrazione della LCC)
    int get_original_id(int internal_id) const { return index_to_id[internal_id]; }

    int get_index(int as_number) const;   // -1 se l'ID non è nel grafo
};

#endif