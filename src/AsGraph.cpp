#include "AsGraph.hpp"
#include <iostream>  // std::cout, std::cerr
#include <algorithm>  // std::find, std::min, std::max
#include <charconv>   // std::from_chars
#include <fstream>
#include <stdexcept>
#include <system_error>

//Le funzioni definite all'interno di namesapace sono visibili solo all'interno di questo file .cpp
namespace {

// Legge un intero non negativo che occupa tutto l'intervallo [begin, end).
// Restituisce false se il campo è vuoto, non numerico o fuori dal range di int.
bool parse_as(const char* begin, const char* end, int& out) {
    if (begin == end) return false;
    auto res = std::from_chars(begin, end, out);
    return res.ec == std::errc() && res.ptr == end && out >= 0;
}

}  // namespace


// Tabelle hash (dizionario con hash universale e liste di trabocco)


IdIndexTable::IdIndexTable(int size) : m(size), n(0), table(size) {}

int IdIndexTable::cerca(int key) const {
    for (const Entry& e : table[hash(key)])
        if (e.key == key) return e.value;
    return -1;
}

void IdIndexTable::inserisci(int key, int value) {
    //table[hash(key)]: Calcola l'indice del bucket tramite la funzione di hash 
    //e ne seleziona la lista di trabocco.
    table[hash(key)].push_back({key, value});
    ++n;
    if (n > m) rehash(2 * m);  // fattore di carico <= 1
}

void IdIndexTable::rehash(int new_m) {

    //Genera nuovi parametri casuali per la funzione di hash universale
    hasher.rehash();
    m = new_m;
    std::vector<std::vector<Entry>> new_table(m);
    for (const auto& bucket : table)
        for (const Entry& e : bucket) new_table[hash(e.key)].push_back(e);
    table = std::move(new_table);
}

EdgeTable::EdgeTable(int size) : m(size), n(0), table(size) {}

void EdgeTable::incrementa(long long key) {
    for (Entry& e : table[hash(key)])

    
    //Se l'arco key è già presente, incrementa il suo conteggio delle frequenze ++e.freq ed esce.
        if (e.key == key) {
            ++e.freq;
            return;
        }

    // altr. aggiunge una nuova Entry con frequenza iniziale 1 e incrementa n
    table[hash(key)].push_back({key, 1});
    ++n;
    if (n > m) rehash(2 * m);
}

void EdgeTable::rehash(int new_m) {
    hasher.rehash();
    m = new_m;
    std::vector<std::vector<Entry>> new_table(m);
    for (const auto& bucket : table)
        for (const Entry& e : bucket) new_table[hash(e.key)].push_back(e);
    table = std::move(new_table);
}
// AsGraph1: lettura del file e conteggio delle frequenze

// Incrementa la frequenza dell'arco non orientato {u,v}; i self-loop (u == v),
// compresi i duplicati consecutivi dovuti al prepending, vengono scartati.
void AsGraph::add_occurrence(int u, int v) {
    if (u == v) return;

    //EdgeTable::encode(...): Codifica la coppia di nodi in un unico intero a 64-bit (long long)
    edge_frequencies.incrementa(EdgeTable::encode(std::min(u, v), std::max(u, v)));
}

void AsGraph::count_frequencies(const std::string& filename) {

    // Apre il file specificato in sola lettura
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("impossibile aprire il file " + filename);

    std::string line;

    //Scorre il file riga per riga.
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        const char* begin = line.data();
        const char* end = begin + line.size();
        if (*(end - 1) == '\r') --end;  // fine riga in stile Windows

        //sp1: Cerca il primo carattere spazio ' ' nella riga per distinguere il formato dei dati
        const char* sp1 = std::find(begin, end, ' ');

        if (sp1 == end) {
            // Formato a coppie: AS1|AS2|relazione (la relazione viene ignorata)
            const char* sep1 = std::find(begin, end, '|');
            if (sep1 == end) continue;
            const char* sep2 = std::find(sep1 + 1, end, '|');

            int u, v;
            //Converte la prima e la seconda sottostringa nei due interi u e v
            if (parse_as(begin, sep1, u) && parse_as(sep1 + 1, sep2, v))
                add_occurrence(u, v);
        } else {
            // Formato a cammini: il cammino è il secondo campo separato da spazi
            const char* path_begin = sp1 + 1;
            const char* path_end = std::find(path_begin, end, ' ');

            int prev = -1;  // AS precedente nel cammino (-1 = nessuno)
            const char* field = path_begin;

            //Scorre tutti i nodi del cammino separati da |
            while (true) {
                const char* sep = std::find(field, path_end, '|');
                int cur;
                if (parse_as(field, sep, cur)) {

                    //Se esiste un nodo precedente prev, aggiunge l'arco tra prev e cur
                    if (prev != -1) add_occurrence(prev, cur);
                    prev = cur;
                } else {
                    prev = -1;  // token non valido: spezza il cammino
                }
                if (sep == path_end) break;
                field = sep + 1;
            }
        }
    }
}
// AsGraph2: mappatura degli ID e costruzione della lista di adiacenza

int AsGraph::get_or_create_index(int as_number) {
    int idx = id_to_index.cerca(as_number);

     //Se l'AS era già stato registrato, restituisce direttamente l'indice esistente.
    if (idx != -1) return idx;

    //Se è un nuovo AS, assegna come nuovo indice la dimensione attuale del vettore
    idx = static_cast<int>(index_to_id.size());
    id_to_index.inserisci(as_number, idx);
    index_to_id.push_back(as_number);
    adj.emplace_back();
    return idx;
}

void AsGraph::construct_graph() {
    // Ripartenza pulita: evita archi duplicati se la funzione è chiamata due volte.
    id_to_index = IdIndexTable();
    index_to_id.clear();
    adj.clear();

    for (const auto& bucket : edge_frequencies.buckets())
        for (const auto& e : bucket) {
            int u = get_or_create_index(EdgeTable::first(e.key));
            int v = get_or_create_index(EdgeTable::second(e.key));

            //Inserimento nella lista di adiacenza
            adj[u].push_back({v, e.freq});
            adj[v].push_back({u, e.freq});
        }

    // Le frequenze ora sono in adj: libera la tabella.
    edge_frequencies = EdgeTable();
}


// AsGraph3: estrazione della componente connessa più grande (LCC)

void AsGraph::extract_lcc() {

    //Ottiene il numero totale di nodi n
    const int n = static_cast<int>(adj.size());
    if (n == 0) return;

    // 1. BFS iterativa da ogni nodo non visitato; si tiene la componente più grande.
    //    "current" funge sia da coda (con indice head) sia da elenco dei nodi.
    std::vector<char> visited(n, 0);
    std::vector<int> largest, current;

    for (int start = 0; start < n; ++start) {
        if (visited[start]) continue;

        current.clear();
        current.push_back(start);
        visited[start] = 1;

        for (size_t head = 0; head < current.size(); ++head) {
            int u = current[head];

            //Esplora tutti i vicini di u
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

    // 2. Ricostruzione delle strutture con indici compatti 0..|LCC|-1.
    const size_t m = largest.size();
    std::vector<int> old_to_new(n, -1);
    std::vector<int> new_index_to_id;
    IdIndexTable new_id_to_index(static_cast<int>(m));
    new_index_to_id.reserve(m);

    //Assegna a ogni nodo della LCC (che aveva indice old_idx) il nuovo indice i
    for (size_t i = 0; i < m; ++i) {
        int old_idx = largest[i];
        old_to_new[old_idx] = static_cast<int>(i);
        new_index_to_id.push_back(index_to_id[old_idx]);
        new_id_to_index.inserisci(index_to_id[old_idx], static_cast<int>(i));
    }

    //Ricostruisce la lista di adiacenza new_adj
    std::vector<std::vector<std::pair<int, int>>> new_adj(m);
    for (size_t i = 0; i < m; ++i) {
        const auto& old_neighbors = adj[largest[i]];
        new_adj[i].reserve(old_neighbors.size());
        for (const auto& edge : old_neighbors)
            new_adj[i].push_back({old_to_new[edge.first], edge.second});
    }

    // 3. Sostituzione delle vecchie strutture.
    adj = std::move(new_adj);
    id_to_index = std::move(new_id_to_index);
    index_to_id = std::move(new_index_to_id);
}


// Getter

int AsGraph::count_unique_edges() const {
    size_t total = 0;
    for (const auto& neighbors : adj) total += neighbors.size();
    return static_cast<int>(total / 2);  // ogni arco compare due volte
}

void AsGraph::print_graph_debug() const {
    const int n = static_cast<int>(adj.size());
    std::cout << "Numero totale di nodi AS: " << n << "\n";
    
    for (int i = 0; i < n; ++i) {
        // index_to_id[i] ci dà il vero AS Number partendo dall'indice interno 'i'
        std::cout << "Nodo AS " << index_to_id[i] << " (indice interno " << i << ") e' collegato a:\n";
        
        for (const auto& edge : adj[i]) {
            int neighbor_internal_index = edge.first;
            int frequency = edge.second;
            int neighbor_as = index_to_id[neighbor_internal_index];
            
            std::cout << "  -> AS " << neighbor_as 
                      << " (frequenza: " << frequency << ")\n";
        }
    }
}


