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

Le tabelle hash non usano `std::unordered_map`: sono implementate seguendo il dizionario visto a lezione (hash universale + liste di trabocco), estese in modo da associare un valore alla chiave.

- **`UniversalHash`**: `h(x) = ((a·x + b) mod p) mod m`, con `p = 998244353` primo, `a ∈ [1, p-1]` e `b ∈ [0, p-1]` casuali. La chiave viene ridotta modulo `p` prima del prodotto, così `a·(x mod p) < 2^60` e non c'è overflow in `long long`.
- **`IdIndexTable`** (tabella hash `int → int`): usata per `id_to_index`, ID originale dell'AS → indice interno `0..V-1`. Ogni bucket è un `vector` di coppie `(chiave, valore)`. Operazioni: `cerca(chiave)` (restituisce `-1` se assente) e `inserisci(chiave, valore)`.
- **`EdgeTable`** (tabella hash `long long → int`): usata per `edge_frequencies`, arco → frequenza. L'arco `(u,v)` con `u < v` è codificato in una sola chiave `u · 2^32 + v`, così l'hash universale della lezione si applica a un intero senza bisogno di hash per coppie. Operazioni: `incrementa(chiave)` (crea l'arco con frequenza 1 oppure aggiunge 1) e accesso ai bucket in sola lettura, per scorrere tutti gli archi. È temporanea: viene svuotata al termine di `construct_graph()`.
- **Rehash**: entrambe le tabelle tengono il numero `n` di elementi; quando `n > m` raddoppiano i bucket (`rehash(2m)`) estraendo nuovi `a` e `b`, come nel `rehash(newM)` della lezione. Il fattore di carico resta ≤ 1, quindi non serve conoscere in anticipo il numero di archi.
- **Vettore dinamico (`std::vector`)**
  - `index_to_id`: indice interno → ID originale (mappatura inversa).
- **Lista di adiacenza** (`std::vector<std::vector<std::pair<int,int>>>`)
  - `adj`: struttura principale del grafo; `adj[u]` contiene le coppie `(indice_vicino, peso)`. Rispetto a una matrice di adiacenza occupa spazio O(V+E) invece di O(V²) e rende immediate le visite del grafo.

### Invarianti

- Ogni chiave di `edge_frequencies` è `encode(min(u,v), max(u,v))` con `u != v`, quindi `(u,v)` e `(v,u)` sono lo stesso arco.
- Ogni chiave compare una sola volta nella tabella in cui è memorizzata.
- `id_to_index.cerca(index_to_id[i]) == i` per ogni `i`.
- `adj.size() == index_to_id.size()` e ogni arco compare nelle liste di entrambi gli estremi con lo stesso peso.
- Dopo `extract_lcc()` gli indici sono compatti (`0..V'-1`) e il grafo è connesso.

### Complessità attesa

Con `L` lunghezza totale dei cammini letti, `V` nodi ed `E` archi distinti: parsing O(L) in media (ogni `incrementa` costa O(1) in media, ammortizzato grazie al rehash), costruzione O(E) in media, estrazione della LCC O(V+E). Spazio: O(E) temporaneo per `edge_frequencies`, O(V+E) per `adj`.

### Dipendenze

AsGraph non dipende dagli altri moduli. È usato da MiniMax (`get_adj()`, `get_index()`, `get_original_id()`) e da Experimental_Analysis (`count_nodes()`, `count_unique_edges()`, `get_adj()`).

---

### Sotto-modulo AsGraph1: lettura, parsing, conteggio frequenze

- **Obiettivo**: leggere il file riga per riga, isolare la sequenza di AS e contare quante volte compare ogni arco.
- **Input**: nome del file (formati descritti sopra).
- **Output**: `edge_frequencies` riempita; un'eccezione `std::runtime_error` se il file non si apre.
- **Strutture dati**: `edge_frequencies` (`EdgeTable`).
- **Specifiche funzionali**:
  - le righe vuote e quelle che iniziano con `#` vengono ignorate;
  - una riga senza spazi è nel formato a coppie e registra un'occorrenza dell'arco `AS1-AS2`;
  - in una riga con spazi, il cammino è il secondo campo; per ogni coppia di AS consecutivi `(u,v)` si registra un'occorrenza dell'arco;
  - la registrazione è affidata a `add_occurrence(u, v)`, che:
    - scarta i self-loop (`u == v`), eliminando anche i duplicati consecutivi dovuti al prepending (es. `701|701|3479`);
    - ordina la coppia (`min`, `max`) prima di codificarla, così `(u,v)` e `(v,u)` non sono contati come archi separati;
    - chiama `edge_frequencies.incrementa()`;
  - un token non valido (vuoto, non numerico, negativo o fuori dal range di `int`) interrompe il cammino in quel punto: non vengono creati archi tra gli AS ai due lati;
  - chiamate successive sommano le frequenze.
- **Complessità**: O(L) in media.

### Sotto-modulo AsGraph2: mappatura degli ID e costruzione del grafo

- **Obiettivo**: mappare gli ID in indici consecutivi, così da usare un `vector` come lista di adiacenza.
- **Input**: `edge_frequencies` (precondizione: `count_frequencies()` già chiamata).
- **Output**: `id_to_index`, `index_to_id` e `adj` popolati; `edge_frequencies` svuotata.
- **Strutture dati**: `id_to_index` (`IdIndexTable`), `index_to_id` (`std::vector<int>`), `adj` (`std::vector<std::vector<std::pair<int,int>>>`, con gli indici interni dei vicini e le frequenze come pesi).
- **Specifiche funzionali**:
  - le strutture vengono azzerate all'inizio, per evitare archi duplicati se la funzione è chiamata due volte;
  - per ogni arco `(u,v)` con frequenza `f`, scorrendo i bucket di `edge_frequencies`, si decodificano gli ID e si ottengono i due indici con `get_or_create_index()`; poi si aggiunge `(v,f)` in `adj[u]` e `(u,f)` in `adj[v]`;
  - inserimento dei nodi: `get_or_create_index()` restituisce l'indice se l'ID è già noto; altrimenti assegna il prossimo indice libero, lo inserisce in `id_to_index`, lo aggiunge a `index_to_id` e aggiunge una lista vuota a `adj`;
  - l'assegnazione degli indici dipende dall'ordine in cui si scorrono i bucket, che cambia con i parametri casuali dell'hash; i risultati del progetto non dipendono dagli indici;
  - al termine `edge_frequencies` viene sostituita da una tabella vuota e la memoria rilasciata.
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
  - la ricostruzione conserva i pesi e l'ordine dei vicini, e ricostruisce `id_to_index` e `index_to_id` in modo coerente con i nuovi indici;
  - dopo la chiamata, `get_index()` restituisce `-1` per gli AS esclusi dalla LCC.
- **Complessità**: O(V+E) tempo in media (la ricostruzione di `id_to_index` costa O(V) in media), O(V+E) spazio aggiuntivo.

---

## 2. Modulo MiniMax

- **Compiti**: costruzione dell'MST della LCC con l'algoritmo di Kruskal (ordinamento degli archi per frequenza e insiemi disgiunti tramite Union-Find), orientamento dell'MST da una radice e pre-calcolo delle tabelle per il Binary Lifting, risposta alle query di costo minimax.

- **Obiettivi**: calcolare il costo del cammino minimax ottimo tra qualsiasi coppia di nodi in tempo logaritmico, dopo un pre-calcolo di costo O(m log m).

- **Idea di fondo**: dati due nodi u e v, il costo minimax ottimo è il massimo peso degli archi sull'unico cammino da u a v nell'MST. Di conseguenza non serve Dijkstra: basta costruire l'MST una volta e interrogarlo. Se più archi hanno lo stesso peso, l'MST non è unico, ma il costo minimax calcolato è sempre lo stesso.

- **Input**: la lista di adiacenza della LCC fornita da `AsGraph::get_adj()` (precondizione: grafo connesso e `adj` simmetrica, con lo stesso peso nelle liste dei due estremi).

- **Output**: un oggetto `MiniMax` che espone `query(u, v)`, dove `u` e `v` sono indici interni, e `countMstEdges()`.

### Strutture dati

- `mst`: `std::vector<Edge>`, con `Edge = {u, v, w}`, gli n-1 archi dell'MST.
- `UnionFind`: due vettori, `parent` (padre nell'albero dell'insieme) e `setSize` (dimensione dell'insieme, valida solo per le radici).
- `depth`: `std::vector<int>`, profondità di ogni nodo nell'MST radicato.
- `ancestor`: `std::vector<std::vector<int>>`, dove `ancestor[k][v]` è l'antenato di `v` a distanza 2^k (`NIL = -1` se non esiste).
- `maxWeight`: `std::vector<std::vector<int>>`, dove `maxWeight[k][v]` è il massimo peso tra gli archi attraversati risalendo da `v` di 2^k livelli.

### Invarianti

- `mst` contiene esattamente n-1 archi e forma un albero che copre tutti i nodi (perché la LCC è connessa).
- Ogni nodo appartiene a esattamente un insieme dell'Union-Find, rappresentato dalla radice del suo albero di puntatori.
- `ancestor[0][v]` è il padre di `v` e `ancestor[0][radice] == NIL`.
- `maxWeight[0][v]` è il peso dell'arco `(ancestor[0][v], v)`.
- `maxWeight[k+1][v] == max(maxWeight[k][v], maxWeight[k][ancestor[k][v]])` quando `ancestor[k][v] != NIL`.

### Complessità attesa

Costruzione: O(m log m) per Kruskal (dominato dall'ordinamento), O(n) per l'orientamento dell'MST, O(n log n) per le tabelle del Binary Lifting. Query: O(log n). Spazio: O(m) temporaneo per la lista degli archi, O(n log n) per `ancestor` e `maxWeight`.

### Dipendenze

MiniMax dipende da AsGraph solo tramite `get_adj()`. La funzione `query()` lavora sugli indici interni: la traduzione da e verso gli As originali e gli indici interi usa `get_index()` e `get_original_id()` di AsGraph.

### Sotto-modulo MiniMax1: UnionFind

- **Obiettivo**: gestire gli insiemi disgiunti per Kruskal, realizzando le operazioni `Appartieni` e `Unisci` dello pseudocodice visto a lezione.

- **Input**: il numero di nodi n (alla creazione); coppie di nodi per le operazioni.

- **Output**: `sameSet(u, v)` dice se i due nodi sono nello stesso insieme; `unite(u, v)` fonde i due insiemi.

- **Strutture dati**: `parent`, `setSize` (vettori di int).

### Sotto-modulo MiniMax2: minimaxKruskal

- **Obiettivo**: costruire l'MST, cioè la lista `mst` dello pseudocodice di Kruskal.

- **Input**: `adj` della LCC.

- **Output**: `mst` con n-1 archi `(u, v, peso)`.

- **Strutture dati**: lista temporanea degli archi, `UnionFind`, `mst`.

### Sotto-modulo MiniMax3: rootTree

- **Obiettivo**: orientare l'MST a partire da una radice e calcolare la profondità dei nodi.

- **Input**: `mst` e la radice (indice 0).

- **Output**: il vettore degli archi orientati `(padre, figlio, peso)`, nel formato usato a lezione per costruire il Binary Lifting, e il vettore `depth`.

- **Strutture dati**: lista di adiacenza temporanea dell'MST, vettore `visited`, vettore `order` usato come coda.

### Sotto-modulo MiniMax4: buildBinaryLifting

- **Obiettivo**: pre-calcolare le tabelle `ancestor` e `maxWeight` per risalire nell'albero di 2^k livelli e ricordare il massimo peso incontrato.

- **Input**:Gli archi orientati prodotti da MiniMax3

- **Output**: `ancestor`, `Maxweight`, `maxLog`

### Sotto-modulo MiniMax5: query

- **Obiettivo**: restituire il costo minimax ottimo tra due nodi.

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
