# ProgettoASD_Moretti.Serena
Progetto di Algoritmi e Strutture Dati
Per iniziare, ho pensato di suddividere la struttura del codice nei seguenti tre macro-moduli:
### 1. Modulo AsGraph
    - __Compiti__: lettura e parsing dell'input,inserimento di nodi/archi, calcolo dei pesi, estrazione della componente connessa più grande.
    -__Obiettivi__: rappresentazione efficiente del grafo BGP, senza nodi isolati o sconnessi.
    -__Strutture Dati__: *tabella hash per mappare gli ID originali degli AS in indici interi consecutivi.
                         *lista di adiacenza per rappresentare il grafo pesato (più conveniente rispetto alla rappresentazione mediante matrice di adiacenza per via della maggiore efficienza nella gestione degli spazi e più comoda per implementare gli algoritmi di visita).
                         *array di visita per estrarre la componente connessa più grande.
### 2. Modulo MiniMax
    - __Compiti__: ordinamento degli archi per frequenza, esecuzione dell'algoritmo Kruksal tramite Union Find per costruire l'MST, pre-calcolo delle tabelle per il Binary Lifting.
    - __Obiettivi__: ricerca del cammino MiniMax ottimo e calcolo del costo tra qualsiasi coppia di nodi in tempo logaritmico.
    - __Strutture Dati__: *
                          *
### 3. Count_Paths
    - __Compiti__: da definire.
    - __Obiettivi__: eventuale implementazione della parte opzionale del codice.
    -__Strutture Dati__:
### 4. Experimental_Analysis
    - __Compiti__: da definire
    - __Obiettivi__: analisi del costo computazionale.
    - __Strutture Dati__:
    
