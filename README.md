# ProgettoASD_Moretti.Serena
Progetto di Algoritmi e Strutture Dati.
Per iniziare, ho pensato di suddividere la struttura del codice nei seguenti quatttro macro-moduli:
### 1. Modulo AsGraph
    - __Compiti__: lettura e parsing dell'input,inserimento di nodi/archi, calcolo dei pesi, estrazione della componente connessa più grande.
    -__Obiettivi__: rappresentazione efficiente del grafo BGP, senza nodi isolati o sconnessi.
    -__Strutture Dati__: *tabella hash per mappare gli ID originali degli AS in indici interi consecutivi.
                         *lista di adiacenza per rappresentare il grafo pesato (più conveniente rispetto alla rappresentazione mediante matrice di adiacenza per via della maggiore efficienza nella gestione degli spazi e più comoda per implementare gli algoritmi di visita).
                         *array di visita per estrarre la componente connessa più grande.
    - __Input__: Nome/Percorso del file BGP reale contenente le tracce dei cammini (file di testo o .bz2).
    -__Output__:
        *Grafo pesato e non orientato limitato alla sola componente connessa più grande (LCC).
        *La lista di adiacenza adj.
        *Il sistema bidirezionale di mappatura tra gli ID AS originali e gli indici compatti.
        *L'interfaccia gettere, utile per gli altri moduli.
    
    - **Sotto-modulo AsGraph1**:lettura, parsing, conteggio frequenze**:
        *Obiettivo: leggere il file riga per riga, isolare la sequenza AS e contare quante volte compare ogni arco
        *Gestione archi non orientati:poiché il grafo è non orientato, un arco da AS1  a As2 equivale ad un arco da AS2  a As1. Conviene, dunque, ordinare la coppia prima di salvarla, per evitare di contare i due archi sopra come archi separati.
        *Gestione self-loop: controllare che u!=v prima di incrementare la frequenza.
    
    -**Sotto-modulo AsGraph2**: mappatura degli ID e costruzione del grafo:
        *Obiettivo: mappare gli ID in indici consecutivi in modo da utilizzare un vector come lista di adiacenza.
        *Mappatura: Creiamo una std::unordered_map<int, int> id_to_index (per passare dall'ID originale all'indice interno) e un std::vector<int> index_to_id (per fare il percorso inverso)
        *Lista di adiacenza: creiamo il std::vector<std::vector<std::pair<int, int>>> contenente gli ID mappati.
        *Inserimento nodi: se ne occupa la funzione helper() dopo aver trovato un ID "nuovo".
    
    -**Sotto-modulo AsGraph3**: estrazione componente connessa più grande (LCC):
        *Obiettivo: estrazione della componente connessa più grande
        *Esplorazione (BFS/DFS):usa un array booleano visited. Itera su tutti i nodi, se un nodo non è visitato lancia un BFS/DFS da lì.
        *Ricostruzione: si costruisce una lista di adiacenza contenente solo i nodi e gli archi appartenenti alla LCC. Questo produce l'output che verrà passato al modulo MiniMax.

### 2. Modulo MiniMax
    - __Compiti__: ordinamento degli archi per frequenza, esecuzione dell'algoritmo Kruskal tramite Union Find per costruire l'MST, pre-calcolo delle tabelle per il Binary Lifting.
    - __Obiettivi__: ricerca del cammino MiniMax ottimo e calcolo del costo tra qualsiasi coppia di nodi in tempo logaritmico.
    - __Strutture Dati__: *lista di archi
                          *Union-Find
                          *Tabelle per Binary Lifting
### 3. Modulo Count_Paths
    - __Compiti__: da definire.
    - __Obiettivi__: eventuale implementazione della parte opzionale del codice.
    -__Strutture Dati__:
### 4. Modulo Experimental_Analysis
    - __Compiti__: tracciamento dei tempi di esecuzione delle singole fasi.
    - __Obiettivi__: analisi della complessità computazionale computazionale.
    - __Strutture Dati__: * vector per l'istogramma delle frequenze.
## Interazione fra Moduli: 
L'interazione fra i Moduli avviene in maniera sequenziale. L'input, rielaborato da AsGraph, viene passato a MiniMax, che lo utilizza per la costruzione dell'MST e il pre-calcolo delle tabelle per il Binary Lifting. Da definire interazione con il terzo Modulo. Il quarto modulo interroga AsGraph per ottenere il numero totale di nodi e misura i tempi di esecuzione di tutti i Moduli del codice.

    
