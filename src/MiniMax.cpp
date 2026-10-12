#include "Minimax.hpp"
#include <algorithm>

using namespace std;

// ---------- MiniMax1: UnionFind ----------
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