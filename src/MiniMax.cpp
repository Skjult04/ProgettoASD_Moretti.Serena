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