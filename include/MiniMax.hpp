#pragma once // Questo file header viene incluso solo una volta durante la compilazione
#include <vector>
#include <utility>

// ---------- MiniMax1: Union-Find ----------
class UnionFind {
public:
    explicit UnionFind(int n);
    bool sameSet(int u, int v) const;   // Appartieni
    void unite(int u, int v);           // Unisci (precondizione: sameSet(u, v) == false)
private:
    int find(int x) const;              // radice dell'albero che contiene x
    std::vector<int> parent;
    std::vector<int> setSize;
};

// ---------- MiniMax2-5 ----------
class MiniMax {
public:
    using AdjList = std::vector<std::vector<std::pair<int, int>>>;
    static constexpr int NIL = -1;

    // adj: lista di adiacenza della LCC (AsGraph::get_adj()), grafo connesso
    explicit MiniMax(const AdjList& adj);

    // costo minimax tra indici interni; NIL se gli indici non sono validi
    int query(int u, int v) const;

    int countMstEdges() const { return static_cast<int>(mst.size()); }

private:
    //Struttura per un arco generico del grafo, con estremità u, v e peso w
    struct Edge { int u, v, w; };

    //Struttura per gli archi orientati
    struct TreeEdge { int parent, child, w; };

    int n;//numero di nodi nel grafo
    int maxLog; //massima potenza di 2 necessaria per il Binary Lifting (log2(n))
    std::vector<Edge> mst;
    std::vector<int> depth;
    std::vector<std::vector<int>> ancestor;    // ancestor[k][v]
    std::vector<std::vector<int>> maxWeight;   // maxWeight[k][v]

    void minimaxKruskal(const AdjList& adj);                       // MiniMax2
    auto rootTree(int root) -> std::vector<TreeEdge>;              // MiniMax3
    void buildBinaryLifting(const std::vector<TreeEdge>& edges);   // MiniMax4
};