# ProgettoASD_Moretti.Serena

Progetto di Algoritmi e Strutture Dati.

Per iniziare, ho pensato di suddividere la struttura del codice nei seguenti quattro macro-moduli:

1. **AsGraph**: costruzione del grafo AS pesato e non orientato ed estrazione della componente connessa più grande.
2. **MiniMax**: struttura dati per rispondere alle query di cammino minimax.
3. **Count_Paths**: parte opzionale (conteggio dei cammini a costo minimax ottimo).
4. **Experimental_Analysis**: misure di tempo e statistiche sul grafo.

---

## 1. Modulo AsGraph

- **Compiti**: lettura e parsing dell'input BGP, conteggio delle frequenze degli archi, inserimento di nodi e archi, costruzione della lista di adiacenza ed estrazione della componente connessa più grande (LCC).
- **Obiettivi**: rappresentazione efficiente del grafo BGP (pesato e non orientato), con aggiornamento delle frequenze in tempo medio O(1) per arco letto e, al termine, un grafo connesso (senza nodi isolati o sconnessi).
- **Input**: percorso di un file di testo con i dati BGP. I file `.bz2` vanno decompressi prima (`bunzip2`), perché il codice legge solo testo. Sono riconosciuti due formati di riga:
  - cammini (`*.all-paths`): `<collector>|<n> AS1|AS2|...|ASk <prefisso> <i/?> <ip>`; il cammino è il secondo campo separato da spazi;
  - coppie (`*.as-rel.txt`): `AS1|AS2|<relazione>`; la relazione viene ignorata.
- **Output**: il grafo pesato e non orientato limitato alla LCC, cioè:
  - la lista di adiacenza `adj`;
  - la mappatura bidirezionale tra ID originali degli AS e indici interni compatti;
  - l'interfaccia di getter usata dagli altri moduli.

### Strutture dati

- **Tabelle hash (`std::unordered_map`)**
  - `edge_frequencies`: `unordered_map<pair<int,int>, int, EdgeHash>`, accumula la frequenza di ogni arco in tempo medio O(1). È temporanea: viene svuotata al termine di `construct_graph()`.
  - `id_to_index`: ID originale dell'AS → indice interno `0..V-1`.
- **Vettore dinamico (`std::vector`)**
  - `index_to_id`: indice interno → ID originale (mappatura inversa).
- **Lista di adiacenza** (`std::vector<std::vector<std::pair<int,int>>>`)
  - `adj`: struttura principale del grafo; `adj[u]` contiene le coppie `(indice_vicino, peso)`. Rispetto a una matrice di adiacenza occupa spazio O(V+E) invece di O(V²) e rende immediate le visite del grafo.

### Tipi e funtori ausiliari

- `EdgeHash`: funtore che calcola l'hash di una `std::pair<int,int>` (la libreria standard non ne fornisce uno) con la formula stile Boost e la costante `0x9e3779b9`. **Non riordina la coppia**: la chiave va inserita già ordinata (vedi invarianti).

### Invarianti

- Ogni chiave di `edge_frequencies` è `(min(u,v), max(u,v))` con `u != v`, quindi `(u,v)` e `(v,u)` sono lo stesso arco.
- `id_to_index[index_to_id[i]] == i` per ogni `i`.
- `adj.size() == index_to_id.size()` e ogni arco compare nelle liste di entrambi gli estremi con lo stesso peso.
- Dopo `extract_lcc()` gli indici sono compatti (`0..V'-1`) e il grafo è connesso.

### Complessità attesa

Con `L` lunghezza totale dei cammini letti, `V` nodi ed `E` archi distinti: parsing O(L) in media, costruzione O(E) in media, estrazione della LCC O(V+E). Spazio: O(E) temporaneo per `edge_frequencies`, O(V+E) per `adj`.

### Dipendenze

AsGraph non dipende dagli altri moduli. È usato da MiniMax (`get_adj()`, `get_index()`, `get_original_id()`) e da Experimental_Analysis (`count_nodes()`, `count_unique_edges()`, `get_adj()`).

---

### Sotto-modulo AsGraph1: lettura, parsing, conteggio frequenze

- **Obiettivo**: leggere il file riga per riga, isolare la sequenza di AS e contare quante volte compare ogni arco.
- **Input**: nome del file (formati descritti sopra).
- **Output**: `edge_frequencies` riempita; un'eccezione `std::runtime_error` se il file non si apre.
- **Strutture dati**: `edge_frequencies` (tabella hash con `EdgeHash`).
- **Specifiche funzionali**:
  - le righe vuote e quelle che iniziano con `#` vengono ignorate;
  - una riga senza spazi è nel formato a coppie e incrementa di 1 la frequenza dell'arco `AS1-AS2`;
  - in una riga con spazi, il cammino è il secondo campo; per ogni coppia di AS consecutivi `(u,v)` la frequenza dell'arco viene incrementata di 1;
  - grafo non orientato: la coppia viene ordinata (`min`, `max`) prima di essere salvata, così `(u,v)` e `(v,u)` non sono contati come archi separati;
  - self-loop: si controlla `u != v` prima di incrementare; ciò elimina anche i duplicati consecutivi dovuti al prepending (es. `701|701|3479`);
  - un token non valido (vuoto, non numerico, negativo o fuori dal range di `int`) interrompe il cammino in quel punto: non vengono creati archi tra gli AS ai due lati;
  - chiamate successive sommano le frequenze.
- **Complessità**: O(L) in media.

### Sotto-modulo AsGraph2: mappatura degli ID e costruzione del grafo

- **Obiettivo**: mappare gli ID in indici consecutivi, così da usare un `vector` come lista di adiacenza.
- **Input**: `edge_frequencies` (precondizione: `count_frequencies()` già chiamata).
- **Output**: `id_to_index`, `index_to_id` e `adj` popolati; `edge_frequencies` svuotata.
- **Strutture dati**: `id_to_index` (`std::unordered_map<int,int>`), `index_to_id` (`std::vector<int>`), `adj` (`std::vector<std::vector<std::pair<int,int>>>`, con gli **indici interni** dei vicini e le frequenze come pesi).
- **Specifiche funzionali**:
  - le strutture vengono azzerate all'inizio, per evitare archi duplicati se la funzione è chiamata due volte;
  - per ogni arco `(u,v)` con frequenza `f` si ottengono i due indici con `get_or_create_index()` e si aggiunge `(v,f)` in `adj[u]` e `(u,f)` in `adj[v]`;
  - inserimento dei nodi: `get_or_create_index()` restituisce l'indice se l'ID è già noto; altrimenti assegna il prossimo indice libero, aggiorna `id_to_index` e `index_to_id` e aggiunge una lista vuota a `adj`;
  - l'assegnazione degli indici dipende dall'ordine di iterazione della tabella hash, quindi non è riproducibile tra implementazioni diverse; i risultati del progetto non dipendono dagli indici;
  - al termine `edge_frequencies` viene svuotata e la memoria dei suoi bucket rilasciata.
- **Complessità**: O(E) in media.

### Sotto-modulo AsGraph3: estrazione della componente connessa più grande (LCC)

- **Obiettivo**: mantenere solo la componente connessa con più nodi.
- **Input**: `adj`, `id_to_index`, `index_to_id` (precondizione: `construct_graph()` già chiamata).
- **Output**: le stesse tre strutture, ricostruite sulla sola LCC con indici compatti `0..V'-1`. È il grafo che viene passato al modulo MiniMax.
- **Strutture dati**: vettore `visited` (uno per nodo), vettore usato come coda per la BFS, vettore `old_to_new` (vecchio indice → nuovo indice, `-1` se il nodo non fa parte della LCC).
- **Specifiche funzionali**:
  - si itera su tutti i nodi; per ogni nodo non visitato si lancia una BFS che raccoglie la sua componente;
  - si sceglie la BFS iterativa per evitare di esaurire lo stack su grafi molto grandi;
  - viene tenuta la componente con più nodi; a parità, la prima incontrata;
  - se il grafo è vuoto, la funzione non fa nulla;
  - la ricostruzione conserva i pesi e l'ordine dei vicini, e aggiorna `id_to_index` e `index_to_id` in modo coerente con i nuovi indici;
  - dopo la chiamata, `get_index()` restituisce `-1` per gli AS esclusi dalla LCC.
- **Complessità**: O(V+E) tempo, O(V+E) spazio aggiuntivo.

---

## 2. Modulo MiniMax

- **Compiti**: ordinamento degli archi per frequenza, esecuzione dell'algoritmo di Kruskal tramite Union-Find per costruire l'MST, pre-calcolo delle tabelle per il Binary Lifting.
- **Obiettivi**: ricerca del cammino minimax ottimo e calcolo del costo tra qualsiasi coppia di nodi in tempo logaritmico.
- **Input**: la lista di adiacenza della LCC fornita da `AsGraph::get_adj()`.
- **Strutture dati**:
  - lista di archi;
  - Union-Find;
  - tabelle per il Binary Lifting.

## 3. Modulo Count_Paths

- **Compiti**: da definire.
- **Obiettivi**: eventuale implementazione della parte opzionale del codice.
- **Strutture dati**: da definire.

## 4. Modulo Experimental_Analysis

- **Compiti**: tracciamento dei tempi di esecuzione delle singole fasi e raccolta delle statistiche richieste (nodi, archi, distribuzione delle frequenze).
- **Obiettivi**: analisi della complessità computazionale.
- **Strutture dati**: `vector` per l'istogramma delle frequenze.

---

## Interazione fra moduli

L'interazione fra i moduli avviene in maniera sequenziale.

- L'input, rielaborato da AsGraph, viene passato a MiniMax tramite `get_adj()`. MiniMax lo usa per la costruzione dell'MST e il pre-calcolo delle tabelle per il Binary Lifting. Le query arrivano con gli ID originali degli AS e vengono tradotte con `get_index()`; i risultati si riportano agli ID originali con `get_original_id()`.
- Da definire l'interazione con il terzo modulo.
- Il quarto modulo interroga AsGraph per ottenere il numero di nodi e di archi (`count_nodes()`, `count_unique_edges()`) e misura i tempi di esecuzione di tutti i moduli. L'istogramma delle frequenze si ricava da `get_adj()` contando ogni arco una sola volta (ad esempio solo quando `v > u`), perché `edge_frequencies` viene svuotata dopo la costruzione del grafo.
    
