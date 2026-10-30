# ProgettoASD_Moretti.Serena

Progetto di Algoritmi e Strutture Dati.

Per iniziare, ho pensato di suddividere la struttura del codice nei seguenti tre macro-moduli:

1. **AsGraph**: costruzione del grafo AS pesato e non orientato ed estrazione della componente connessa più grande.
2. **MiniMax**: struttura dati per rispondere alle query di cammino minimax.
4. **ExperimentalAnalysis**: misure di tempo e statistiche sul grafo.

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

Le tabelle hash sono implementate seguendo il dizionario visto a lezione (hash universale + liste di trabocco), estese in modo da associare un valore alla chiave.

- **`UniversalHash`**: `h(x) = ((a·x + b) mod p) mod m`, con `p = 998244353` primo, dove m è il numero di bucket della tabella. I parametri sono `a = random_below(p-1) + 1`, quindi `a ∈ [1, p-1]`, e `b = random_below(p)`, quindi `b ∈ [0, p-1]`. `random_below(k)` è una funzione di AsGraph.hpp che restituisce un intero in `[0, k)` come `(hi · (RAND_MAX+1) + lo) mod k`, con hi e lo due chiamate a rand() in istruzioni separate. Una sola chiamata con RAND_MAX = 32767 darebbe solo 32768 valori, molti meno dei circa 10⁹ di a e b; due chiamate danno almeno 2³⁰ > p valori, e l’ordine fisso rende riproducibile l’esecuzione per un dato seed. La chiave viene ridotta modulo `p` prima del prodotto, così `a·(x mod p) < 2^60` e non c'è overflow in `long long`.
- **`IdIndexTable`** (tabella hash `int → int`): usata per `id_to_index`, ID originale dell'AS → indice interno `0..V-1`. Ogni bucket è un `vector` di coppie `(chiave, valore)`. Operazioni: `cerca(chiave)` (restituisce `-1` se assente) e `inserisci(chiave, valore)`.
- **`EdgeTable`** (tabella hash `long long → int`): usata per `edge_frequencies`, arco → frequenza.L’arco (u,v) con `u < v` è codificato in una sola chiave `u · 2^32 + v` (codifica iniettiva), così la stessa tabella hash vale per interi e non servono hash per coppie. Operazioni: `incrementa(chiave)` (crea l'arco con frequenza 1 oppure aggiunge 1) e accesso ai bucket in sola lettura, per scorrere tutti gli archi. È temporanea: viene svuotata al termine di `construct_graph()`.
- **Rehash**: entrambe le tabelle tengono il numero `n` di elementi; quando `n > m` raddoppiano i bucket (`rehash(2m)`) estraendo nuovi `a` e `b`, come nel `rehash(newM)` della lezione. Il fattore di carico resta ≤ 1.
- **Limite dell’universalità**: la famiglia `h(x) = ((a·x + b) mod p) mod m` è universale per chiavi `x ∈ [0, p-1]`: se `x ≠ y` allora `a(x-y) ≢ 0 (mod p)`, quindi la collisione può avvenire solo nella riduzione mod m, con probabilità `≤ 1/m` su `(a,b)`. Il codice però riduce la chiave con `x % p `prima del prodotto, e questa riduzione non dipende da a e b: due chiavi distinte congruenti modulo p collidono sempre. Questo riguarda le chiavi degli archi (fino a `~2^63`) e, in teoria, gli ID degli AS a 32 bit maggiori di p. Di conseguenza la garanzia teorica di costo atteso O(1) vale pienamente solo per chiavi minori di p; per le altre è un’ipotesi sulla distribuzione dei dati (poche chiavi congruenti modulo p, plausibile sui dump reali). La correttezza non è in discussione: ogni tabella confronta sempre la chiave intera `(e.key == key)`, quindi una collisione costa solo tempo, mai un risultato sbagliato.
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

Con `L` lunghezza totale dei cammini letti, `V` nodi ed `E` archi distinti: parsing O(L) in media (ogni `incrementa` costa O(1) in media, ammortizzato grazie al rehash), costruzione O(E) in media, estrazione della LCC O(V+E).

### Dipendenze

AsGraph non dipende dagli altri moduli. È usato da ExperimentalAnalysis (`count_nodes()`, `count_unique_edges()`, `get_adj()`, `get_original_id()`, `get_index()` ), che passa `get_adj()` a MiniMax.

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
  - le strutture vengono azzerate all'inizio e una seconda chiamata senza `count_frequencies()` produce un grafo vuoto, perché `edge_frequencies` è già stata svuotata;
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
- **Complessità**: O(V+E) tempo in media (la ricostruzione di `id_to_index` costa O(V) in media).

---

## 2. Modulo MiniMax

- **Compiti**: costruzione dell'MST della LCC con l'algoritmo di Kruskal (ordinamento degli archi per frequenza e insiemi disgiunti tramite Union-Find), orientamento dell'MST da una radice e pre-calcolo delle tabelle per il Binary Lifting, risposta alle query di costo minimax.

- **Obiettivi**: calcolare il costo del cammino minimax ottimo tra qualsiasi coppia di nodi in tempo logaritmico, dopo un pre-calcolo di costo O(m log m).

- **Idea di fondo**: dati due nodi u e v, il costo minimax ottimo è il massimo peso degli archi sull'unico cammino da u a v nell'MST. Di conseguenza non serve Dijkstra: basta costruire l'MST una volta e interrogarlo. Se più archi hanno lo stesso peso, l'MST non è unico, ma il costo minimax calcolato è sempre lo stesso.

- **Input**: la lista di adiacenza della LCC di tipo `MiniMax::AdjList` passata al costruttore da ExperimentalAnalysis, che la ottiene da AsGraph(`get_adj()`) (precondizione: grafo connesso e `adj` simmetrica, con lo stesso peso nelle liste dei due estremi).

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

Costruzione: O(m log n) per Kruskal, O(n) per l'orientamento dell'MST, O(n log n) per le tabelle del Binary Lifting. Query: O(log n). Spazio: O(m) temporaneo per la lista degli archi, O(n log n) per `ancestor` e `maxWeight`.

### Dipendenze

MiniMax non include AsGraph: il costruttore riceve la lista di adiacenza della LCC,
di tipo `MiniMax::AdjList`, che il chiamante ottiene con `AsGraph::get_adj()`. Le query
lavorano su indici interni: la traduzione da e verso gli ID originali degli AS usa
`get_index()` e `get_original_id()` di AsGraph, quindi avviene fuori da MiniMax.

---

### Sotto-modulo MiniMax1: UnionFind

- **Obiettivo**: gestire gli insiemi disgiunti per Kruskal, realizzando le operazioni `Appartieni` e `Unisci` dello pseudocodice visto a lezione.

- **Input**: il numero di nodi n (alla creazione); coppie di nodi per le operazioni.

- **Output**: `sameSet(u, v)` dice se i due nodi sono nello stesso insieme; `unite(u, v)` fonde i due insiemi.

- **Strutture dati**: `parent`, `setSize` (vettori di int).

- **Specifiche funzionali**:
  - all'inizio ogni nodo è un insieme a sé (`parent[i]=i`, `setSize[i]=1`)
  - `Unisci(u,v)` rispetta la definizione data a lezione: se `S_u != S_v`, i due insiemi vengono eliminati e sostituiti da `S_u U S_v`; se `S_u==S_v` non si fa nulla(dal punto di vista teorico, il codice evita questo caso). Nel codice ciò significa attaccare la radice dell'albero più piccolo a quella dell'albero più grande (union by size).
  - `Appartieni(u,v)` confronta le radici dei due alberi; la ricerca della radice risale i puntatori ai padri.
  - precondizione di `unite`: `sameSet(u,v) == false` (come in Kruskal,dove `Unisci` è chiamata solo se `!Appartieni` ).

- **Complessità**: O(logn) per operazione, perché con union by size ogni albero ha altezza O(logn). Il costo totale per Kruskal è O(mlogn).

### Sotto-modulo MiniMax2: minimaxKruskal

- **Obiettivo**: costruire l'MST, cioè la lista `mst` dello pseudocodice di Kruskal.

- **Input**: `adj` della LCC.

- **Output**: `mst` con n-1 archi `(u, v, peso)`.

- **Strutture dati**: lista temporanea degli archi, `UnionFind`, `mst`.

- **Specifiche funzionali**:
  - Nell'algoritmo di Kruskal ho utilizzato un vettore(`std::vector<Edge>`) di archi, che poi viene ordinato, al posto della coda di priorità. Scorrere il vettore ordinato per pesi crescenti equivale, infatti, ad estrarre il minimo ad ogni passo nella coda di priorità, con lo stesso costo nel caso peggiore (O(mlogm)).Il vantaggio di utilizzare un vettore consiste nella struttura più semplice e negli accessi sequenziali in memoria. Inoltre, ogni arco compare in adj due volte, quindi lo si inserisce nel vettore solo quando `u < v`.
  - Nel secondo ciclo dell'algoritmo di Kruskal, si scorrono gli archi in ordine crescente di peso; se `!Appartieni(u,v)` si esegue Unisci(u,v) e si aggiunge l'arco a `mst` (`InserisciFondo`).
  - il ciclo si interrompe appena mst contiene n-1 archi.

- **Complessità**: O(mlogm) per l'ordinamento, più O(mlogn)per le operazioni di Union Find. Totale: O(mlogm)

### Sotto-modulo MiniMax3: rootTree

- **Obiettivo**: orientare l'MST a partire da una radice e calcolare la profondità dei nodi.

- **Input**: `mst` e la radice (indice 0).

- **Output**: il vettore degli archi orientati `(padre, figlio, peso)`, nel formato usato a lezione per costruire il Binary Lifting, e il vettore `depth`.

- **Strutture dati**: lista di adiacenza temporanea dell'MST, vettore `visited`, vettore `order` usato come coda.

- **Specifiche funzionali**:
  - si costruisce la lista di adiacenza dell'MST;
  - BFS iterativa dalla radice: ogni arco che scopre un nuovo nodo viene registrato come `(padre, figlio, peso)` e `depth[figlio]=depth[padre]+1`;
  - la BFS iterativa evita di esaurire lo stack, come in AsGraph3

- **Complessità**: O(n) tempo.


### Sotto-modulo MiniMax4: buildBinaryLifting

- **Obiettivo**: pre-calcolare le tabelle `ancestor` e `maxWeight` per risalire nell'albero di 2^k livelli e ricordare il massimo peso incontrato.

- **Input**:Gli archi orientati prodotti da MiniMax3

- **Output**: `ancestor`, `maxWeight`, `maxLog`

- **Strutture dati**: `ancestor`, `maxWeight` (tabelle `maxLog`x `n`)

- **Specifiche funzionali**:
  - `maxLog` è calcolato come segue: si parte da 1 e si incrementa finché `1 << maxLog<=n`;
  - caso base: per ogni arco `(padre, figlio, peso)` si pone `ancestor[0][figlio]=padre` e `maxWeight[0][figlio]=peso`; per la radice `ancestor[0][radice]=NIL`;
  - caso induttivo: `ancestor[k+1][v]=ancestor[k][ancestor[k][v]]` se `ancestor[k][v] != NIL`, altrimenti Nil; `maxWeight[k+1][v]=max(maxWeight[k][v], maxWeight[k][ancestor[k][v]])`. Nel caso `ancestor[k][v]==NIL`, `maxWeight[k+1][v]=maxWeight[k][v]`

- **Complessità**: O(nlogn) tempo.

### Sotto-modulo MiniMax5: query

- **Obiettivo**: restituire il costo minimax ottimo tra due nodi.

- **Input**: due indici interni u e v.

- **Output**: il costo minimax ottimo(0 se `u==v`); `NIL` se un indice non è valido (ad esempio -1, restituito da `get_index()` per un AS fuori dalla LCC).

- **Strutture dati**: `depth`, `ancestor`, `maxWeight`.

- **Specifiche funzionali**:
  - si scambiano i nodi in modo che u sia il più profondo
  - si porta u alla profondità di v risalendo di depth[u]-depth[v] livelli e aggiornando il massimo con`maxWeight[k][u]` a ogni salto;
  - se a questo punto u==v, il costo è il massimo accumulato;
  - per `k` da `maxLog-1` a 0, se `ancestor[k][u] != ancestor[k][v]` si salta (si risale nell'albero di 2^k livelli) con entrambi i nodi, aggiornando il massimo con `maxWeight[k][u]` e `maxWeight[k][v]`;  
  - al termine, u e v sono figli dello stesso nodo (il loro antenato comune più basso), quindi il costo finale è il massimo tra quello accumulato, `maxWeight[0][u]` e `maxWeight[0][v]`

- **Complessità**: O(logn) per query.


## 4. Modulo ExperimentalAnalysis

File: `ExperimentalAnalysis.hpp`, `ExperimentalAnalysis.cpp`.

**Compiti**:

- conteggio del numero di nodi e archi del grafo completo e della LCC, distribuzione delle frequenze sugli archi (ossia il numero di archi del grafo di frequenza esattamente f);
- misura dei tempi di esecuzione delle singole fasi (lettura e conteggio delle frequenze, costruzione del grafo, estrazione della LCC, costruzione della struttura MiniMax) e delle query;
- esecuzione di un insieme di query minimax su coppie di nodi scelte in modo arbitrario, con raccolta dei costi ottimi ottenuti;
- verifica sperimentale della correttezza delle risposte di MiniMax, con un metodo indipendente dall'MST;
- stampa dei risultati.

- **Notazione**: `V` nodi ed `E` archi (come in AsGraph; corrispondono a `n` ed `m` di MiniMax); `fmax` frequenza massima (è l'ultimo elemento di `distinctFreq`); `F` numero di frequenze distinte (`F ≤ E`); `Q` numero di query eseguite; `S` numero di query verificate.

- **Obiettivi**: documentare con dati reali il comportamento dei moduli AsGraph e MiniMax e confrontare i tempi misurati con la complessità attesa: lineare per la lettura e la costruzione del grafo, O(m log m) per l'MST, O(log n) per query.
  - Gli esperimenti significativi si fanno con i file `*.all-paths`. Con i file `*.as-rel.txt` ogni arco compare una sola volta e ha quindi frequenza 1: l'istogramma ha una sola frequenza e ogni query ha costo 1, perciò quel formato serve solo a controllare la struttura del grafo e i tempi di lettura e costruzione.

- **Input**:
  - percorso del file BGP (formati di AsGraph);
  - numero Q di query casuali;
  - numero S di query da verificare (Se `S>Q` se ne verificano Q);
  - seme `seed` per `srand()`.

- **Output**:
  - report su standard output;
  - file `istogramma.csv` con la distribuzione delle frequenze.

- **Strumenti usati**:
  - `<chrono>` per misurare i tempi;
  - `rand()` per generare numeri casuali tramite `random_below()`;
  - `std::sort` per costruire l'istogramma;
  - visita BFS e ricerca booleana su funzioni monotone per la verifica.

- **Strutture dati**:
  - `struct GraphStats { int nodes; int edges; }`;
  - `struct Timings { double read, build, lcc, minimax; }`, in millisecondi;
  - `std::vector<std::pair<int,int>> histogram` (coppie `(frequenza, numero di archi)`), `std::vector<int> distinctFreq`;
  - `std::vector<std::pair<int,int>> queries`, `std::vector<int> costs`.

- **Flusso (`run_experiments(filename, Q, S, seed)`)**. Tutti gli oggetti sono variabili locali della funzione, quindi `MiniMax` si costruisce dopo la LCC senza puntatori:
  1. `srand(seed)`, prima di creare `AsGraph` (che usa `rand()` per i parametri dell'hash universale), così le esecuzioni sono riproducibili;
  2. `count_frequencies(filename)`, tempo in `read`;
  3. `construct_graph()`, tempo in `build`;
  4. `compute_stats(g)`: statistiche del grafo completo;
  5. `extract_lcc()`, tempo in `lcc`;
  6. `compute_stats(g)`: statistiche della LCC; se `nodes == 0` si stampa il report (statistiche e tempi, con `minimax = 0` perché la struttura non viene costruita) e il flusso termina;
  7. `build_histogram(g, histogram, distinctFreq)`;
  8. `MiniMax mm(g.get_adj())`, tempo in `minimax`;
  9. `run_queries(...)`;
  10. `verify_queries(...)`;
  11. `write_histogram_csv("istogramma.csv", histogram)`;
  12. `print_report(...)`, con `Q` pari al numero di query realmente eseguite e `S` pari a `min(S, Q)`;
  13. stampa di al più 5 query d'esempio (le prime della lista) con gli ID originali degli AS (`get_original_id()` e `query_by_id()`), per mostrare esempi concreti di costo ottimo minimax.

- **Casi limite**: file senza archi validi (LCC vuota): vedi passo 6; grafo con meno di 2 nodi o `Q == 0`: nessuna query, e la sezione sulle query non viene stampata; `S > Q`: si verificano solo `Q` query; `S == 0`: la verifica non viene eseguita; file non apribile: `count_frequencies()` lancia `std::runtime_error`, gestita dal `main`. I parametri negativi sono rifiutati dal `main`.

- **Complessità attesa**: il costo è dominato dai moduli chiamati. Il solo modulo aggiunge O(V + E log E) per statistiche e istogramma, O(Q) per la generazione delle query più Q chiamate a `query` (ognuna O(log n)), e O(S · log F · (V + E)) per la verifica, con F numero di frequenze distinte.

- **Invarianti**: dopo `run_experiments`, AsGraph e MiniMax non sono stati modificati se non dalle loro stesse fasi di costruzione; tutte le funzioni di analisi ricevono `AsGraph` e `MiniMax` per riferimento `const`.

- **Dipendenze**: AsGraph (`count_nodes()`, `count_unique_edges()`, `get_adj()`, `get_index()`, `get_original_id()` e le tre fasi di costruzione), MiniMax (costruttore e `query()`, costante `NIL`).

### Sotto-modulo ExperimentalAnalysis1: statistiche del grafo

- **Compiti**: rilevare numero di nodi e di archi del grafo prima e dopo l'estrazione della LCC.
- **Obiettivi**: riportare il conteggio di nodi e archi e mostrare quale percentuale dei nodi e degli archi del grafo originario si trova nella LCC.
- **Funzione**: `GraphStats compute_stats(const AsGraph& g)`.
- **Input**: un grafo AsGraph in un qualunque stato successivo a `construct_graph()`.
- **Output**: `{g.count_nodes(), g.count_unique_edges()}`.
- **Strutture dati**: `GraphStats`.
- **Specifiche funzionali**: non modifica nulla. Viene chiamata due volte, prima e dopo `extract_lcc()`, perché dopo l'estrazione i valori del grafo completo non sono più recuperabili.
- **Complessità**: O(V), perché `count_unique_edges()` somma le dimensioni delle liste di adiacenza.
- **Invarianti**: `edges ≥ nodes - 1` nella LCC (è connessa); `nodes` e `edges` della LCC non superano quelli del grafo completo.
- **Dipendenze**: AsGraph (`count_nodes()`, `count_unique_edges()`).

### Sotto-modulo ExperimentalAnalysis2: distribuzione delle frequenze

- **Compiti**: costruire l'istogramma delle frequenze degli archi della LCC (solo per le frequenze che compaiono davvero) e ricavarne l'elenco delle frequenze distinte, usato dalla verifica.
- **Obiettivi**: riportare la distribuzione delle frequenze usando uno spazio proporzionale al numero di archi e non alla frequenza massima. Sui dump `all-paths` `fmax` può arrivare a milioni di occorrenze, mentre le frequenze distinte sono poche: un vettore indicizzato da `f` di taglia `fmax + 1` occuperebbe centinaia di MB quasi tutti a zero.
- **Scelta di progetto**: si copiano i pesi degli archi in un vettore, lo si ordina e si contano le sequenze di valori uguali (run). Costa O(E log E) in tempo ma solo O(E) in spazio, e produce direttamente sia i conteggi sia `distinctFreq`. Si scarta il vettore denso `histogram[f]`, che avrebbe tempo O(V + E + fmax) ma spazio O(fmax).
- **Funzione**: `void build_histogram(const AsGraph& g, std::vector<std::pair<int,int>>& histogram, std::vector<int>& distinctFreq)`.
- **Input**: il grafo della LCC.
- **Output**:
  - `histogram`: coppie `(f, c)` in ordine crescente di `f`, dove `c > 0` è il numero di archi di frequenza `f`;
  - `distinctFreq`: le frequenze `f` presenti in `histogram`, nello stesso ordine.
- **Strutture dati**:
  - `std::vector<int> weights`, vettore temporaneo con i pesi di tutti gli archi (liberato al termine);
  - `std::vector<std::pair<int,int>> histogram`;
  - `std::vector<int> distinctFreq`.
- **Specifiche funzionali**:
  - ogni arco compare in `adj` due volte, quindi viene contato solo quando `v > u`;
  - un solo passaggio su `get_adj()` riempie `weights` (con `reserve(E)`);
  - `weights` viene ordinato con `std::sort`;
  - una scansione di `weights` conta le run di valori uguali: per ogni run `(f, c)` si aggiunge la coppia a `histogram` e `f` a `distinctFreq`;
  - `histogram` e `distinctFreq` vengono azzerati all'inizio, così la funzione si può richiamare;
  - se il grafo non ha archi, entrambi sono vuoti.
- **Complessità**: O(V + E log E) tempo; O(E) spazio temporaneo per `weights` e O(F) per il risultato, con `F ≤ E`.
- **Invarianti**: ogni frequenza in `histogram` è `≥ 1` e ogni conteggio è `> 0`; le frequenze sono strettamente crescenti; la somma dei conteggi è `E`; `distinctFreq[i] == histogram[i].first` per ogni `i`.
- **Dipendenze**: AsGraph (`get_adj()`, `count_nodes()`, `count_unique_edges()`).

### Sotto-modulo ExperimentalAnalysis3: misura dei tempi

- **Compiti**: cronometrare con `<chrono>` le fasi `count_frequencies()`, `construct_graph()`, `extract_lcc()` e la costruzione dell'oggetto `MiniMax`.
- **Obiettivi**: ottenere i tempi di esecuzione richiesti dalla traccia, fase per fase, da confrontare con le complessità attese.
- **Funzione**: `double ms_since(std::chrono::steady_clock::time_point start)`.
- **Input**: l'istante `start`, preso con `std::chrono::steady_clock::now()` prima della fase.
- **Output**: millisecondi trascorsi, come `double`, calcolati con `std::chrono::duration<double, std::milli>`.
- **Strutture dati**: `Timings`.
- **Specifiche funzionali**:
  - ogni fase è racchiusa fra due letture del clock: `t0 = now()`, chiamata, `x = ms_since(t0)`;
  - le fasi misurate sono `count_frequencies()` (`read`), `construct_graph()` (`build`, comprende il rilascio di `edge_frequencies`), `extract_lcc()` (`lcc`) e il costruttore di `MiniMax` (`minimax`);
  - il costruttore di `MiniMax` è misurato nel suo insieme (Kruskal, orientamento, Binary Lifting), perché le sue sotto-fasi sono private;
  - si usa `steady_clock`, che non è influenzato da cambi dell'orologio di sistema;
  - le fasi sono eseguite una volta sola, perché durano abbastanza da essere misurabili; per le query vedi ExperimentalAnalysis4.
- **Complessità**: O(1) per misura.
- **Invarianti**: ogni tempo è ≥ 0 e le misure delle fasi non si sovrappongono.
- **Dipendenze**: libreria standard (`<chrono>`) per `ms_since()`; AsGraph e MiniMax, di cui vengono cronometrate le chiamate pubbliche (`count_frequencies()`, `construct_graph()`, `extract_lcc()`, costruttore di `MiniMax`) all'interno di `run_experiments()`.

### Sotto-modulo ExperimentalAnalysis4: esperimenti sulle query

- **Compiti**:
  - generare con `rand()` Q coppie di indici interni, eseguire le query con `MiniMax::query()` e cronometrare l'intero blocco;
  - raccogliere i costi ottimi ottenuti;
  - permettere anche una query su una coppia di AS scelta dall'utente, indicata con gli ID originali.
- **Obiettivi**: riportare il costo ottimo minimax e il tempo medio per query (tempo del blocco diviso Q, perché la singola query è troppo breve per essere misurata da sola). Una query costa circa `log n` passi, cioè frazioni di microsecondo o pochi microsecondi: con poche query il blocco durerebbe meno di un millisecondo e la misura sarebbe dominata dal rumore. Per questo il valore predefinito di `Q` è alto (100000, vedi `main`).
- **Funzioni**:
  - `int random_index(int n)`;
  - `void run_queries(const AsGraph& g, const MiniMax& mm, int Q, std::vector<std::pair<int,int>>& queries, std::vector<int>& costs, double& totalMs)`;
  - `int query_by_id(const AsGraph& g, const MiniMax& mm, int as_u, int as_v)`.
- **Input**: grafo della LCC, oggetto `MiniMax` già costruito, `Q`; per `query_by_id`, due ID originali di AS. Precondizione di `random_index`: `n > 0`.
- **Output**: `queries` (Q coppie di indici interni con `u != v`, non necessariamente distinte fra loro), `costs` (costo restituito per ogni coppia), `totalMs` (tempo dell'intero blocco di query); `query_by_id` restituisce il costo minimax o `MiniMax::NIL` se uno dei due AS non è nella LCC.
- **Strutture dati**:
  - `std::vector<std::pair<int,int>> queries` con le coppie estratte;
  - `std::vector<int> costs` con i costi restituiti.
- **Specifiche funzionali**:
  - `random_index(n)` restituisce un intero in `[0, n)` chiamando `random_below(n)` di AsGraph;
  - `run_queries` con `n < 2` o `Q <= 0` lascia i vettori vuoti e `totalMs = 0`;
  - le coppie sono generate tutte prima di cronometrare, così il tempo non include la generazione; una coppia con `u == v` viene estratta di nuovo;
  - il cronometro (`steady_clock`) racchiude solo il ciclo `costs[i] = mm.query(u_i, v_i)`;
  - `query_by_id` calcola `mm.query(g.get_index(as_u), g.get_index(as_v))`: `get_index()` restituisce `-1` per un AS fuori dalla LCC, e `query()` risponde `NIL` agli indici non validi;
  - `run_experiments()` usa `query_by_id()` per le query d'esempio finali, partendo dagli ID originali ottenuti con `get_original_id()`.
- **Invarianti**: `queries.size() == costs.size()`, uguale a `Q` se `n ≥ 2` e `Q > 0`, altrimenti 0; ogni coppia ha `0 ≤ u, v < n` e `u != v`; ogni costo è ≥ 1 e coincide con la frequenza di un arco, quindi compare in `distinctFreq`.
- **Complessità**: O(Q) per la generazione (in media), Q · O(log n) per le query; `query_by_id` O(log n) in media.
- **Dipendenze**: AsGraph (`count_nodes()`, `get_index()`, `get_original_id()`, `random_below()`), MiniMax (`query()`), ExperimentalAnalysis3 (`ms_since()`).

### Sotto-modulo ExperimentalAnalysis5: verifica di correttezza

- **Compiti**: per un campione ridotto delle coppie di ExperimentalAnalysis4, ricalcolare il costo minimax con ricerca binaria sulle frequenze distinte e BFS, e confrontarlo con la risposta di `MiniMax::query()`.
- **Obiettivi**: controllare MiniMax con un metodo indipendente dall'MST. Il campione è piccolo perché ogni verifica costa più di una query: richiede O(log F) BFS sull'intero grafo, con F numero di frequenze distinte.
- **Funzioni**:
  - `bool connected_upto(const AsGraph& g, int u, int v, int t, std::vector<char>& visited, std::vector<int>& queue)`;
  - `int minimax_by_search(const AsGraph& g, const std::vector<int>& distinctFreq, int u, int v, std::vector<char>& visited, std::vector<int>& queue)`;
  - `int verify_queries(const AsGraph& g, const std::vector<int>& distinctFreq, const std::vector<std::pair<int,int>>& queries, const std::vector<int>& costs, int S)`.
- **Input**: grafo della LCC, `distinctFreq`, le coppie e i costi di ExperimentalAnalysis4, il numero `S` di query da verificare.
- **Output**: `connected_upto` dice se `v` è raggiungibile da `u` usando solo archi di frequenza `≤ t`; `minimax_by_search` restituisce il costo minimax calcolato senza MST (0 se `u == v`); `verify_queries` restituisce il numero di discordanze tra questo costo e quello di `MiniMax::query()` sulle prime `min(S, Q)` coppie (atteso: 0).
- **Strutture dati**:
  - `distinctFreq` (da ExperimentalAnalysis2);
  - `std::vector<char> visited` (un byte per nodo, come in AsGraph3);
  - un `std::vector<int>` usato come coda con indice di testa, come nella BFS di AsGraph3.
- **Specifiche funzionali**:
  - `connected_upto`: se `u == v` restituisce `true`; altrimenti azzera `visited` e fa una BFS iterativa da `u` che percorre un arco `(x, y, w)` solo se `w ≤ t`; termina appena raggiunge `v`;
  - proprietà usata: "u e v sono connessi con archi `≤ t`" è una funzione booleana monotona in `t`, e il costo minimax è il più piccolo `t` per cui è vera; poiché è la frequenza di un arco, si cerca tra `distinctFreq`;
  - `minimax_by_search`: se `u == v` restituisce subito 0, con un controllo esplicito prima della ricerca (senza, la ricerca binaria restituirebbe `distinctFreq[0]`); altrimenti ricerca binaria con `lo = 0`, `hi = distinctFreq.size() - 1`; a ogni passo `mid = (lo + hi) / 2`; se `connected_upto(distinctFreq[mid])` allora `hi = mid`, altrimenti `lo = mid + 1`; si ferma con `lo == hi` e restituisce `distinctFreq[lo]`;
  - precondizioni: `distinctFreq` non vuoto, `u` e `v` indici validi della LCC; la risposta è vera per `distinctFreq.back()` (la frequenza massima), perché la LCC è connessa;
  - `verify_queries` restituisce 0 se `distinctFreq` o `queries` sono vuoti o se `S <= 0`; altrimenti esegue `minimax_by_search` per ciascuna delle prime `min(S, Q)` coppie e conta le discordanze rispetto a `costs[i]`; `visited` e `queue` sono allocati una sola volta e riutilizzati;
  - il costo minimax non coincide in generale con la frequenza dell'arco diretto: nell'esempio della traccia, `3 → 4` ha costo 2 (cammino `3 → 5 → 4`), anche se l'arco `(3,4)` ha frequenza 3;
  - test di sanità sull'esempio della traccia: un file in formato `all-paths` con le sei righe
    ```
    rv|1 1|2|5 10.0.0.0/24 i 1.1.1.1
    rv|1 1|2|5 10.0.1.0/24 i 1.1.1.1
    rv|1 1|3|4|5 10.0.2.0/24 i 1.1.1.1
    rv|1 1|3|5 10.0.3.0/24 i 1.1.1.1
    rv|1 2|3|4 10.0.4.0/24 i 1.1.1.1
    rv|1 3|4|5 10.0.5.0/24 i 1.1.1.1
    ```
    deve produrre 5 nodi e 7 archi, l'istogramma `1,2` / `2,4` / `3,1`, costo 2 tra gli AS 1 e 5, costo 1 tra gli AS 2 e 5 e costo 2 tra gli AS 3 e 4, sia con `MiniMax::query()` sia con `minimax_by_search`. Le righe devono avere il formato `all-paths` (con gli spazi): una riga di solo cammino come `1|2|5`, senza spazi, viene letta da AsGraph1 nel formato a coppie e registra solo l'arco `1-2`.
- **Complessità**: `connected_upto` O(V + E) (il riempimento di `visited` costa O(V)); `minimax_by_search` O(log F · (V + E)); `verify_queries` O(S · log F · (V + E)). Per questo `S` è molto più piccolo di `Q`.
- **Invarianti**: invariante della ricerca binaria: la risposta si trova in `distinctFreq[lo..hi]` e `connected_upto(distinctFreq[hi])` è vera; `visited` è azzerato a ogni chiamata di `connected_upto`.
- **Dipendenze**: AsGraph (`get_adj()`, `count_nodes()`), ExperimentalAnalysis2 (`distinctFreq`) e ExperimentalAnalysis4 (`queries`, `costs`).

### Sotto-modulo ExperimentalAnalysis6: report dei risultati

- **Compiti**: stampare in forma leggibile statistiche, istogramma, tempi, esito delle query e della verifica.
- **Obiettivi**: presentare in un unico punto tutti i dati richiesti.
- **Funzioni**:
  - `void write_histogram_csv(const std::string& filename, const std::vector<std::pair<int,int>>& histogram)`;
  - `void print_report(const GraphStats& full, const GraphStats& lcc, const std::vector<std::pair<int,int>>& histogram, const Timings& t, int Q, double queryMs, const std::vector<int>& costs, int S, int mismatches)`.
- **Input**: i risultati degli altri sotto-moduli; in `print_report`, `Q` è il numero di query eseguite e `S` il numero di query verificate (cioè `min(S, Q)`).
- **Output**: il file `istogramma.csv` e il report su standard output.
- **Strutture dati**: nessuna propria, bensì legge quelle degli altri sotto-moduli.
- **Specifiche funzionali**:
  - `write_histogram_csv` scrive una riga `frequenza,numero_archi`, senza intestazione, per ogni coppia di `histogram`; lancia `std::runtime_error` se il file non si può scrivere;
  - l'istogramma completo va su file e non a schermo, perché può avere molte frequenze distinte;
  - il report contiene: nodi e archi del grafo completo e della LCC, con la percentuale di nodi e di archi contenuta nella LCC (se il grafo completo ha nodi e archi); frequenza massima (`histogram.back().first`), numero e percentuale di archi con frequenza 1 (il conteggio della prima coppia se la sua frequenza è 1, altrimenti 0) e numero di frequenze distinte (`histogram.size()`), se la LCC ha archi; i quattro tempi in millisecondi; per le query, il numero `Q`, il costo minimo, massimo e medio, il tempo medio per query in microsecondi (`queryMs * 1000 / Q`) e l'esito della verifica (`mismatches` su `S`, oppure "non eseguita" se `S == 0`);
  - se `Q == 0` la sezione sulle query non viene stampata;
  - le query d'esempio con gli ID originali non fanno parte di `print_report`: le stampa `run_experiments()` subito dopo.
- **Complessità**: O(Q) (calcolo di minimo, massimo e media dei costi); la scrittura del CSV è O(F).
- **Invarianti**: il report non modifica alcuna struttura; ogni riga del CSV ha frequenza ≥ 1 e numero di archi > 0.
- **Dipendenze**: dipende da tutti i sotto-moduli di ExperimentalAnalysis precedenti.

---

## Programma principale (main)

File: `main.cpp`.

- **Compiti**: punto d'ingresso del programma; legge i parametri da riga di comando, li controlla e avvia `run_experiments()`; segnala gli errori.
- **Input**: `<file_bgp> [Q] [S] [seed]`, dove `file_bgp` è un file di testo già decompresso (formati di AsGraph1; per esperimenti significativi si usa un file `all-paths`), `Q` il numero di query casuali (default 100000), `S ≤ Q` il numero di query verificate con BFS (default 50), `seed` il seme di `rand()` (default 12345). `Q`, `S` e `seed` sono interi `≥ 0`.
- **Output**: nessun dato proprio; il report è prodotto da ExperimentalAnalysis. Codice di uscita 0 se l'esecuzione termina correttamente, 1 per parametri non validi o errori (ad esempio file non apribile).
- **Specifiche funzionali**:
  - il `main` non contiene logica del progetto: l'ordine delle fasi (lettura, costruzione del grafo, LCC, MiniMax, query, verifica) è descritto in `run_experiments()`;
  - i parametri numerici sono letti con `std::from_chars`, che rifiuta valori vuoti, non numerici o fuori range, come in AsGraph; i valori negativi sono rifiutati con un controllo esplicito;
  - il numero di argomenti deve essere tra 1 e 4, altrimenti si stampa l'uso e si esce con codice 1;
  - le eccezioni sono intercettate e stampate su `std::cerr`.
- **Dipendenze**: solo ExperimentalAnalysis (`run_experiments()`); gli altri moduli sono usati da `run_experiments()`.

## Interazione fra moduli

AsGraph e MiniMax sono indipendenti: nessuno dei due include l'altro, né ExperimentalAnalysis. Il solo punto di contatto tra loro è il tipo della lista di adiacenza: `AsGraph::get_adj()` restituisce un `std::vector<std::vector<std::pair<int,int>>>`, che è esattamente il tipo `MiniMax::AdjList` accettato dal costruttore di MiniMax.

Il collegamento è realizzato da ExperimentalAnalysis, che dipende da entrambi e orchestra il flusso (`run_experiments()`):

1. chiama in sequenza `count_frequencies()`, `construct_graph()` ed `extract_lcc()` di AsGraph, misurando il tempo di ciascuna fase;
2. passa la lista di adiacenza della LCC a MiniMax con `MiniMax mm(g.get_adj())`, misurando il tempo del costruttore (Kruskal, orientamento dell'MST, Binary Lifting);
3. legge da AsGraph `count_nodes()`, `count_unique_edges()` e `get_adj()` per statistiche e istogramma. L'istogramma si ricava da `get_adj()` contando ogni arco una sola volta (solo quando `v > u`), perché `edge_frequencies` viene svuotata dopo la costruzione del grafo;
4. esegue le query con `MiniMax::query()`, che lavora solo su indici interni.

La traduzione tra ID originali degli AS e indici interni avviene fuori da MiniMax, in ExperimentalAnalysis, tramite `get_index()` e `get_original_id()` di AsGraph (ad esempio in `query_by_id()`). Se un AS non è nella LCC, `get_index()` restituisce -1 e `query()` risponde `NIL`.
