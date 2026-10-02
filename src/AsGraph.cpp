#include "AsGraph.hpp"

#include <algorithm>  // std::find, std::min, std::max
#include <charconv>   // std::from_chars
#include <fstream>
#include <stdexcept>
#include <system_error>

//Le funzioni definite all'interno di namesapace sono visibili solo all'interno di questo file .cpp
namespace {

// Legge un intero non negativo che occupa tutto l'intervallo [begin, end).
// Restituisce false se il campo è vuoto, non numerico o fuori dal range di int.
bool parse_as(const char* begin, const char* end, int& out) {
    if (begin == end) return false;
    auto res = std::from_chars(begin, end, out);
    return res.ec == std::errc() && res.ptr == end && out >= 0;
}

}  // namespace


// Tabelle hash (dizionario con hash universale e liste di trabocco)


IdIndexTable::IdIndexTable(int size) : m(size), n(0), table(size) {}

int IdIndexTable::cerca(int key) const {
    for (const Entry& e : table[hash(key)])
        if (e.key == key) return e.value;
    return -1;
}

void IdIndexTable::inserisci(int key, int value) {
    //table[hash(key)]: Calcola l'indice del bucket tramite la funzione di hash 
    //e ne seleziona la lista di trabocco.
    table[hash(key)].push_back({key, value});
    ++n;
    if (n > m) rehash(2 * m);  // fattore di carico <= 1
}

void IdIndexTable::rehash(int new_m) {

    //Genera nuovi parametri casuali per la funzione di hash universale
    hasher.rehash();
    m = new_m;
    std::vector<std::vector<Entry>> new_table(m);
    for (const auto& bucket : table)
        for (const Entry& e : bucket) new_table[hash(e.key)].push_back(e);
    table = std::move(new_table);
}

EdgeTable::EdgeTable(int size) : m(size), n(0), table(size) {}

void EdgeTable::incrementa(long long key) {
    for (Entry& e : table[hash(key)])

    
    //Se l'arco key è già presente, incrementa il suo conteggio delle frequenze ++e.freq ed esce.
        if (e.key == key) {
            ++e.freq;
            return;
        }

    // altr. aggiunge una nuova Entry con frequenza iniziale 1 e incrementa n
    table[hash(key)].push_back({key, 1});
    ++n;
    if (n > m) rehash(2 * m);
}

void EdgeTable::rehash(int new_m) {
    hasher.rehash();
    m = new_m;
    std::vector<std::vector<Entry>> new_table(m);
    for (const auto& bucket : table)
        for (const Entry& e : bucket) new_table[hash(e.key)].push_back(e);
    table = std::move(new_table);
}


