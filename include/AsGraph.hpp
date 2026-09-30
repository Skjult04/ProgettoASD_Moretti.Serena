#ifndef AS_GRAPH_HPP
#define AS_GRAPH_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// Funtore di hash per std::pair<int, int>: la libreria standard non ne fornisce
// uno, quindi serve per usare una coppia come chiave di unordered_map.
// Combina gli hash dei due interi con la formula stile Boost.
// Nota: l'hash NON normalizza l'ordine della coppia. Le chiavi vanno sempre
// inserite già ordinate, cioè con first <= second (vedi edge_frequencies).
struct EdgeHash {
	std::size_t operator()(const std::pair<int, int>& edge) const {
		std::size_t h1 = std::hash<int>{}(edge.first);
		std::size_t h2 = std::hash<int>{}(edge.second);
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};

// Modulo AsGraph: costruzione del grafo AS pesato e non orientato a partire da
// file BGP, ed estrazione della componente connessa più grande (LCC).
//
// Uso tipico (le tre fasi vanno chiamate in quest'ordine):
//     AsGraph g;
//     g.count_frequencies("file.all-paths");   // AsGraph1
//     g.construct_graph();                     // AsGraph2
//     g.extract_lcc();                         // AsGraph3
class AsGraph {
private:
	// AsGraph1: frequenza di ogni arco non orientato, con accesso in tempo medio O(1).
	// Invariante: la chiave è (min(u,v), max(u,v)) con u != v, così (u,v) e (v,u)
	// sono la stessa chiave. Viene svuotata alla fine di construct_graph().
	std::unordered_map<std::pair<int, int>, int, EdgeHash> edge_frequencies;

	// AsGraph2: mappatura tra ID originale dell'AS e indice interno 0..V-1.
	// Invariante: id_to_index[index_to_id[i]] == i per ogni i.
	std::unordered_map<int, int> id_to_index;
	std::vector<int> index_to_id;

	// AsGraph2: lista di adiacenza; adj[u] = lista di (indice interno di v, peso).
	// Invariante: adj.size() == index_to_id.size(); ogni arco è presente in
	// entrambe le liste dei suoi estremi.
	std::vector<std::vector<std::pair<int, int>>> adj;

	// AsGraph2: restituisce l'indice interno dell'AS; se l'AS è nuovo gli assegna
	// il prossimo indice libero e aggiunge una lista di adiacenza vuota.
	int get_or_create_index(int as_number);

public:
	// AsGraph1: legge il file e accumula in edge_frequencies la frequenza degli
	// archi. Precondizione: edge_frequencies è vuoto.
	void count_frequencies(const std::string& filename);

	// AsGraph2: costruisce mappatura e lista di adiacenza da edge_frequencies,
	// poi libera edge_frequencies. Precondizione: count_frequencies() già chiamata.
	void construct_graph();

	// AsGraph3: mantiene solo la componente connessa più grande. Dopo la chiamata
	// adj, id_to_index e index_to_id sono ricostruiti con indici compatti 0..V'-1.
	// Precondizione: construct_graph() già chiamata.
	void extract_lcc();

	// Numero di nodi del grafo corrente.
	int count_nodes() const { return static_cast<int>(adj.size()); }

	// Numero di archi non orientati del grafo corrente (calcolato da adj).
	int count_unique_edges() const;

	// Lista di adiacenza del grafo corrente: (indice interno del vicino, peso).
	const std::vector<std::vector<std::pair<int, int>>>& get_adj() const { return adj; }

	// ID originale dell'AS dato l'indice interno 0..V-1.
	int get_original_id(int internal_id) const { return index_to_id[internal_id]; }

	// Indice interno dato l'ID originale dell'AS; -1 se l'AS non è nel grafo.
	int get_index(int as_number) const;
};

#endif  // AS_GRAPH_HPP
