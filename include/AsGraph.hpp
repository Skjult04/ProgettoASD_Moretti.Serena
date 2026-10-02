#ifndef AS_GRAPH_HPP
#define AS_GRAPH_HPP

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

// Hash universale (stesso schema visto a lezione): h(x) = ((a*x + b) mod p)
// con p primo, a in [1, p-1], b in [0, p-1] scelti a caso.
// La chiave viene ridotta modulo p prima del prodotto: con p < 2^30 e a < p
// il prodotto a*(x mod p) resta sotto 2^60, quindi niente overflow in long long.
// Chiavi diverse con lo stesso valore mod p finiscono nello stesso bucket, ma
// ogni tabella confronta sempre la chiave completa, quindi la correttezza non
// dipende da questa riduzione.
class UniversalHash {
private:

	//long long tipo per numeri molto grandi (64 bit) 
	//per evitare overflow durante le operazioni di hashing
	long long p, a, b;

public:
	
UniversalHash(long long _p = 998244353)
		: p(_p), a(rand() % (_p - 1) + 1), b(rand() % _p) {}

	//La riduzione preventiva x % p 
	//garantisce che il prodotto non superi il limite di un intero a 64-bit
		long long hash(long long x) const { return (a * (x % p) + b) % p; }

	
		//Rigenera casualmente i parametri a e b 
		//per cambiare la funzione di hash in caso di troppe collisioni.
		void rehash() {
		a = rand() % (p - 1) + 1;
		b = rand() % p;
	}
};


// Tabella hash int -> int con liste di trabocco (dizionario della lezione, ma
// con un valore associato alla chiave). Usata per id_to_index.
// Invariante: n = numero di coppie memorizzate; si raddoppia m quando n > m,
// quindi il fattore di carico resta <= 1 e le operazioni costano O(1) in media.

//Mappa l'ID originale dell'AS (chiave intera) al suo indice interno (valore intero)
class IdIndexTable {
private:
	struct Entry {
		int key;
		int value;
	};
	int m;  // numero di bucket
	int n;  // numero di coppie presenti
	std::vector<std::vector<Entry>> table;
	UniversalHash hasher;

	int hash(int x) const { return static_cast<int>(hasher.hash(x) % m); }
	void rehash(int new_m);

public:
	explicit IdIndexTable(int size = 1024);

	// Valore associato a key; -1 se la chiave non c'è (i valori sono indici >= 0).
	int cerca(int key) const;

	// Precondizione: key non è già presente.
	void inserisci(int key, int value);
};


// Tabella hash arco -> frequenza. L'arco (u,v), con u < v, è codificato in un
// solo long long: key = u * 2^32 + v. Usata per edge_frequencies.

class EdgeTable {
public:
	struct Entry {
		long long key;
		int freq;
	};

private:
	int m;
	int n;
	std::vector<std::vector<Entry>> table;
	UniversalHash hasher;

	int hash(long long x) const { return static_cast<int>(hasher.hash(x) % m); }
	void rehash(int new_m);

public:
	explicit EdgeTable(int size = 1024);

	// Codifica / decodifica della coppia (u,v) con 0 <= u < v < 2^31.
	//encode(u, v): Combina due interi a 32 bit u e v in un unico long long a 64 bit
	static long long encode(int u, int v) {
		return (static_cast<long long>(u) << 32) | static_cast<long long>(v);
	}
	static int first(long long key) { return static_cast<int>(key >> 32); }
	static int second(long long key) { return static_cast<int>(key & 0xFFFFFFFFLL); }

	// Aggiunge 1 alla frequenza della chiave (la crea con frequenza 1 se assente).
	void incrementa(long long key);

	// Accesso in sola lettura ai bucket, per scorrere tutte le coppie.
	const std::vector<std::vector<Entry>>& buckets() const { return table; }
};

// Modulo AsGraph: costruzione del grafo AS pesato e non orientato a partire da
// file BGP, ed estrazione della componente connessa più grande (LCC).
class AsGraph {
private:
	// AsGraph1: frequenza di ogni arco non orientato.
	// Invariante: la chiave è encode(min(u,v), max(u,v)) con u != v, così (u,v) e
	// (v,u) sono la stessa chiave. Viene svuotata alla fine di construct_graph().
	EdgeTable edge_frequencies;

	// AsGraph2: mappatura tra ID originale dell'AS e indice interno 0..V-1.
	// Invariante: id_to_index.cerca(index_to_id[i]) == i per ogni i.
	IdIndexTable id_to_index;
	std::vector<int> index_to_id;

	// AsGraph2: lista di adiacenza; adj[u] = lista di (indice interno di v, peso).
	// Invariante: adj.size() == index_to_id.size(); ogni arco è presente in
	// entrambe le liste dei suoi estremi con lo stesso peso.
	std::vector<std::vector<std::pair<int, int>>> adj;

	// AsGraph1: registra un'occorrenza dell'arco {u,v}; ignora i self-loop.
	void add_occurrence(int u, int v);

	// AsGraph2: restituisce l'indice interno dell'AS; se l'AS è nuovo gli assegna
	// il prossimo indice libero e aggiunge una lista di adiacenza vuota.
	int get_or_create_index(int as_number);

public:
	// AsGraph1: legge il file e accumula in edge_frequencies la frequenza degli
	// archi. Chiamate successive sommano le frequenze.
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
	int get_index(int as_number) const { return id_to_index.cerca(as_number); }
};

#endif
