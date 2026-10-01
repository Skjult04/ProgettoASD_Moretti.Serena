#include "AsGraph.hpp"
#include <algorithm>//Serve per usare std::find. std::min e std::max
#include <charconv> //Serve per usare la funzione std::from_chars
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace {

// Legge un intero non negativo che occupa tutto l'intervallo [begin, end).
// Restituisce false se il campo è vuoto, non numerico o fuori dal range di int.
//Funzione aiutante che prende un pezzo di testo (da begin a end) e cerca di trasformarlo in un numero intero salvandolo nella variabile out.
// Restituisce true se ci riesce ed è un numero non negativo, false se il testo è vuoto, contiene lettere o è un numero sballato
bool parse_as(const char* begin, const char* end, int& out) {
    if (begin == end) return false;
    auto res = std::from_chars(begin, end, out);
    return res.ec == std::errc() && res.ptr == end && out >= 0;
}

}  // namespace

// ---------------------------------------------------------------------------
// AsGraph1: lettura del file e conteggio delle frequenze
// ---------------------------------------------------------------------------
//Questa funzione apre il file di testo contenente i dati di internet.
void AsGraph::count_frequencies(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("impossibile aprire il file " + filename);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        //Inizializza un puntatore a caratteri begin che punta al primo elemento della stringa in memoria (line.data())
        const char* begin = line.data();
        const char* end = begin + line.size();
        
        //Verifica se l'ultimo carattere prima di end è il carattere di ritorno a capo Windows (\r).
        // Se presente, decrementa end di 1 per ignorarlo.
        if (*(end - 1) == '\r') --end;  // fine riga in stile Windows
        //Utilizza std::find per cercare la prima occorrenza del carattere spazio ' ' nell'intervallo [begin, end).
        const char* sp1 = std::find(begin, end, ' ');

        //Se nella riga non ci sono spazi
        if (sp1 == end) {
            // Formato a coppie: AS1|AS2|relazione (la relazione viene ignorata)
            //Cerca il primo separatore pipe '|' nell'intervallo [begin, end).
            const char* sep1 = std::find(begin, end, '|');
            if (sep1 == end) continue;
            const char* sep2 = std::find(sep1 + 1, end, '|');

            int u, v;

            //parse_as(begin, sep1, u) converte il primo ID (AS1)
            //parse_as(sep1 + 1, sep2, v) converte il secondo ID (AS2)
            if (!parse_as(begin, sep1, u) || !parse_as(sep1 + 1, sep2, v)) continue;

            //Se gli ID di u e v sono distinti, incrementa di 1 la frequenza 
            //dell'arco non orientato ordinato (min{u,v}, max{u,v}) nella tabella hash edge_frequencies
            if (u != v) edge_frequencies[{std::min(u, v), std::max(u, v)}]++;
        } else {
            // Formato a cammini: il percorso AS è il secondo campo separato da spazi
            const char* path_begin = sp1 + 1;
            const char* path_end = std::find(path_begin, end, ' ');

            //Inizializza la variabile prev a -1. 
            //Servirà a tenere traccia dell'AS precedente incontrato lungo il percorso
            int prev = -1;
            const char* field = path_begin;

            //Avvia un ciclo di parsing per scorrere tutti i token separati da pipe '|' all'interno del cammino
            while (true) {
                const char* sep = std::find(field, path_end, '|');
                int cur;
                if (parse_as(field, sep, cur)) {
                    // prev != cur scarta self-loop e duplicati consecutivi (prepending)
                    if (prev != -1 && prev != cur) edge_frequencies[{std::min(prev, cur), std::max(prev, cur)}]++;
                    prev = cur;
                } else {
                    prev = -1;  // token non valido: spezza il cammino, niente archi falsi
                }
                if (sep == path_end) break;
                field = sep + 1;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// AsGraph2: mappatura degli ID e costruzione della lista di adiacenza
// ---------------------------------------------------------------------------

//Questa funzione ausiliaria prende l'ID reale di un Autonomous System (as_number) 
//e restituisce un indice intero compatto e sequenziale (0, 1, 2,...) 
//creandolo se non esiste ancora
int AsGraph::get_or_create_index(int as_number) {
    
    //Cerca as_number in id_to_index (che associa l'ID reale dell'AS al suo indice interno)
    auto it = id_to_index.find(as_number);
    //Se l'ID è già presente, restituisce l'indice associato.
    if (it != id_to_index.end()) return it->second;

    int new_index = static_cast<int>(index_to_id.size());//conversione in int
    id_to_index[as_number] = new_index;
    index_to_id.push_back(as_number);
    adj.emplace_back();
    return new_index;
}

//conversione di edge_frequencies nella struttura dati finale del grafo (la lista di adiacenza adj).
void AsGraph::construct_graph() {
    // Ripartenza pulita: evita archi duplicati se la funzione è chiamata due volte.
    id_to_index.clear();
    index_to_id.clear();
    adj.clear();

    //Avvia un ciclo for per scorrere ogni coppia chiave-valore all'interno della tabella hash edge_frequencies.
    for (const auto& entry : edge_frequencies) {
        //Estrae gli ID reali degli AS u e v
        int u_id = entry.first.first;
        int v_id = entry.first.second;
        int freq = entry.second;
        
        //Converte gli ID reali u_id e v_id nei rispettivi indici interni contigui u e v
        int u = get_or_create_index(u_id);
        int v = get_or_create_index(v_id);

        //Aggiunge l'arco in entrambe le direzioni nella lista di adiacenza adj 
        adj[u].push_back({v, freq});
        adj[v].push_back({u, freq});
    }

    // Le frequenze ora sono nella lista di adiacenza: libera anche i bucket.
    std::unordered_map<std::pair<int, int>, int, EdgeHash>().swap(edge_frequencies);
}

// ---------------------------------------------------------------------------
// AsGraph3: estrazione della componente connessa più grande (LCC)
// ---------------------------------------------------------------------------
void AsGraph::extract_lcc() {

    //Numero totale dei nodi attuali nel grafo leggendo la dimensione delle liste di adiacenza (adj).
    const int n = static_cast<int>(adj.size());
    if (n == 0) return;

    // 1. BFS da ogni nodo non visitato; si tiene la componente più grande.
    //    "current" funge sia da coda (con indice head) sia da elenco dei nodi.
    std::vector<char> visited(n, 0);
    std::vector<int> largest, current;

    for (int start = 0; start < n; ++start) {
        if (visited[start]) continue;

        //Svuota il vettore current per iniziare la visita di una nuova componente
        current.clear();
        current.push_back(start);
        visited[start] = 1;

        // Implementazione efficiente di una BFS usando un array dinamico come coda.
        for (size_t head = 0; head < current.size(); ++head) {
            int u = current[head];

            //Scorre tutti i vicini del nodo u.
            for (const auto& edge : adj[u]) {
                int next = edge.first;
                if (!visited[next]) {
                    visited[next] = 1;
                    current.push_back(next);
                }
            }
        }

        if (current.size() > largest.size()) largest.swap(current);
    }

    // 2. Ricostruzione delle strutture dati con indici compatti 0..|LCC|-1.
    const size_t m = largest.size();
    std::vector<int> old_to_new(n, -1);
    std::vector<int> new_index_to_id;
    std::unordered_map<int, int> new_id_to_index;
    new_index_to_id.reserve(m);
    new_id_to_index.reserve(m);

    //Cicla su ciascun nodo della LCC (dove i diventerà il nuovo indice [0...m-1]
    for (size_t i = 0; i < m; ++i) {
        int old_idx = largest[i];
        int original_id = index_to_id[old_idx];

        // Salva la conversione dal vecchio indice a quello nuovo (i)
        old_to_new[old_idx] = static_cast<int>(i);
        new_index_to_id.push_back(original_id);
        new_id_to_index[original_id] = static_cast<int>(i);
    }
    //Crea le nuove liste di adiacenza per soli m nodi
    std::vector<std::vector<std::pair<int, int>>> new_adj(m);
    for (size_t i = 0; i < m; ++i) {
        const auto& old_neighbors = adj[largest[i]];
        new_adj[i].reserve(old_neighbors.size());

        //Scorre ogni arco uscente del vecchio nodo.
        for (const auto& edge : old_neighbors)

            //Aggiunge alla nuova lista di adiacenza l'arco ri-mappato
            new_adj[i].push_back({old_to_new[edge.first], edge.second});
    }

    // 3. Sostituzione delle vecchie strutture.

    //Sposta le nuove strutture al posto delle vecchie, liberando la memoria delle vecchie strutture.
    adj = std::move(new_adj);
    id_to_index = std::move(new_id_to_index);
    index_to_id = std::move(new_index_to_id);
}

// ---------------------------------------------------------------------------
// Getter
// ---------------------------------------------------------------------------

//Somma il numero totale di elementi presenti in adj e divide per 2.
int AsGraph::count_unique_edges() const {
    size_t total = 0;
    for (const auto& neighbors : adj) total += neighbors.size();
    return static_cast<int>(total / 2);  // ogni arco compare due volte
}

//Cerca un AS tramite il suo numero univoco (as_number) all'interno della mappa id_to_index.
int AsGraph::get_index(int as_number) const {
    auto it = id_to_index.find(as_number);
    return (it == id_to_index.end()) ? -1 : it->second;
}