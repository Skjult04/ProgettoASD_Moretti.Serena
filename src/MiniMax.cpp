#include "Minimax.hpp"
#include <algorithm>

using namespace std;

// MiniMax1: UnionFind
UnionFind::UnionFind(int n) : parent(n), setSize(n, 1) {
    for (auto i = 0; i < n; i++) {
        parent[i] = i;
    }
}

int UnionFind::find(int x) const {
    while (parent[x] != x) {
        x = parent[x];
    }
    return x;
}

// Appartieni(u, v): confronta le radici dei due alberi
bool UnionFind::sameSet(int u, int v) const {
    return find(u) == find(v);
}

// Unisci(u, v): se S_u != S_v fonde i due insiemi 
// se S_u == S_v non fa nulla.
// Precondizione: sameSet(u, v) == false.
void UnionFind::unite(int u, int v) {
    auto a = find(u);
    auto b = find(v);
    if (a == b) return;

    if (setSize[a] < setSize[b]) swap(a, b); 
    parent[b] = a;
    setSize[a] += setSize[b];
}

MiniMax::MiniMax(const AdjList& adj)
    : n(static_cast<int>(adj.size())), maxLog(1) {
    if (n == 0) return;

    minimaxKruskal(adj);
    auto root = 0;
    buildBinaryLifting(rootTree(root));
}

// MiniMax2
void MiniMax::minimaxKruskal(const AdjList& adj) {
    // primo ciclo dello pseudocodice: raccolta degli archi (ogni arco una volta)
    vector<Edge> edges;
    for (auto u = 0; u < n; u++) {
        for (auto [v, w] : adj[u]) {
            if (u < v) edges.push_back({u, v, w});
        }
    }
    sort(edges.begin(), edges.end(),
         [](const Edge& a, const Edge& b) { return a.w < b.w; });

    UnionFind sets(n);   // set[u] = NuovoNodo(); Crea(set[u]);

    // secondo ciclo: scorro gli archi in ordine di peso crescente
    mst.clear();
    mst.reserve(n - 1);
    for (const auto& e : edges) {
        if (!sets.sameSet(e.u, e.v)) {       // !Appartieni
            sets.unite(e.u, e.v);            // Unisci
            mst.push_back(e);                // mst.InserisciFondo
            if (static_cast<int>(mst.size()) == n - 1) break;
        }
    }
}

// MiniMax3
auto MiniMax::rootTree(int root) -> vector<TreeEdge> {
    vector<vector<pair<int, int>>> mstAdj(n);
    for (auto [u, v, w] : mst) {
        mstAdj[u].push_back({v, w});
        mstAdj[v].push_back({u, w});
    }

    depth.assign(n, 0);
    vector<bool> visited(n, false);
    vector<int> order;
    order.reserve(n);
    vector<TreeEdge> edges;

    visited[root] = true;
    order.push_back(root);
    for (size_t head = 0; head < order.size(); head++) {
        auto u = order[head];
        for (auto [v, w] : mstAdj[u]) {
            if (visited[v]) continue;
            visited[v] = true;
            depth[v] = depth[u] + 1;
            edges.push_back({u, v, w});   // (padre, figlio, peso)
            order.push_back(v);
        }
    }
    return edges;
}
// MiniMax4
void MiniMax::buildBinaryLifting(const vector<TreeEdge>& edges) {
    maxLog = 1;
    while ((1 << maxLog) <= n) {
        maxLog++;
    }

    ancestor.assign(maxLog, vector<int>(n, NIL));
    maxWeight.assign(maxLog, vector<int>(n, 0));

    // caso base: ancestor[0][v] = padre di v
    for (auto [parent, child, w] : edges) {
        ancestor[0][child] = parent;
        maxWeight[0][child] = w;
    }

    // ancestor[k+1][v] = ancestor[k][ancestor[k][v]], se esiste
    for (auto k = 0; k + 1 < maxLog; k++) {
        for (auto v = 0; v < n; v++) {
            auto mid = ancestor[k][v];
            if (mid == NIL) {
                ancestor[k + 1][v] = NIL;
                maxWeight[k + 1][v] = maxWeight[k][v];
            } else {
                ancestor[k + 1][v] = ancestor[k][mid];
                maxWeight[k + 1][v] = max(maxWeight[k][v], maxWeight[k][mid]);
            }
        }
    }
}

// MiniMax5
int MiniMax::query(int u, int v) const {
    if (u < 0 || v < 0 || u >= n || v >= n) return NIL;

    auto cost = 0;
    if (depth[u] < depth[v]) swap(u, v);

    // 1) porto u alla profondità di v (come kthAncestor)
    auto diff = depth[u] - depth[v];
    for (auto k = 0; k < maxLog; k++) {
        if (diff & (1 << k)) {
            cost = max(cost, maxWeight[k][u]);
            u = ancestor[k][u];
        }
    }
    if (u == v) return cost;

    // 2) risalgo insieme finché gli antenati differiscono
    for (auto k = maxLog - 1; k >= 0; k--) {
        if (ancestor[k][u] != ancestor[k][v]) {
            cost = max({cost, maxWeight[k][u], maxWeight[k][v]});
            u = ancestor[k][u];
            v = ancestor[k][v];
        }
    }

    // ora u e v sono figli dello stesso nodo (il loro antenato comune)
    return max({cost, maxWeight[0][u], maxWeight[0][v]});
}