#include "AsGraph.hpp"
#include <algorithm>//Serve per usare std::find
#include <charconv> ////Serve per usare la funzione std::from_chars
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace {

// Legge un intero non negativo che occupa tutto l'intervallo [begin, end).
// Restituisce false se il campo è vuoto, non numerico o fuori dal range di int.
//Funzione aiutante che prende un pezzo di testo (da begin a end) e cerca di trasformarlo in un numero intero salvandolo nella variabile out.
// Restituisce true se ci riesce ed è un numero non negativo, false se il testo è vuoto, contiene lettere o è un numero sballato
bool parse_as(const char* begin, const char* end, int& out) {
    if (begin == end) return false;
    auto res = std::from_chars(begin, end, out);
    return res.ec == std::errc() && res.ptr == end && out >= 0;
}

}  // namespace

// ---------------------------------------------------------------------------
// AsGraph1: lettura del file e conteggio delle frequenze
// ---------------------------------------------------------------------------
////Questa funzione apre il file di testo contenente i dati di internet.
void AsGraph::count_frequencies(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("impossibile aprire il file " + filename);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        ////Inizializza un puntatore a caratteri begin che punta al primo elemento della stringa in memoria (line.data())
        const char* begin = line.data();
        const char* end = begin + line.size();
        
        //Verifica se l'ultimo carattere prima di end è il carattere di ritorno a capo Windows (\r).
        // Se presente, decrementa end di 1 per ignorarlo.
        if (*(end - 1) == '\r') --end;  // fine riga in stile Windows
        //Utilizza std::find per cercare la prima occorrenza del carattere spazio ' ' nell'intervallo [begin, end).
        const char* sp1 = std::find(begin, end, ' ');

        //Se nella riga non ci sono spazi
        if (sp1 == end) {
            // Formato a coppie: AS1|AS2|relazione (la relazione viene ignorata)
            //Cerca il primo separatore pipe '|' nell'intervallo [begin, end).
            const char* sep1 = std::find(begin, end, '|');
            if (sep1 == end) continue;
            const char* sep2 = std::find(sep1 + 1, end, '|');

            int u, v;

            //parse_as(begin, sep1, u) converte il primo ID (AS1)
            //parse_as(sep1 + 1, sep2, v) converte il secondo ID (AS2)
            if (!parse_as(begin, sep1, u) || !parse_as(sep1 + 1, sep2, v)) continue;

            //Se gli ID di u e v sono distinti, incrementa di 1 la frequenza 
            //dell'arco non orientato ordinato (min{u,v}, max{u,v}) nella tabella hash edge_frequencies
            if (u != v) edge_frequencies[{std::min(u, v), std::max(u, v)}]++;
        } else {
            // Formato a cammini: il percorso AS è il secondo campo separato da spazi
            const char* path_begin = sp1 + 1;
            const char* path_end = std::find(path_begin, end, ' ');

            //Inizializza la variabile prev a -1. 
            //Servirà a tenere traccia dell'AS precedente incontrato lungo il percorso
            int prev = -1;
            const char* field = path_begin;

            //Avvia un ciclo di parsing per scorrere tutti i token separati da pipe '|' all'interno del cammino
            while (true) {
                const char* sep = std::find(field, path_end, '|');
                int cur;
                if (parse_as(field, sep, cur)) {
                    // prev != cur scarta self-loop e duplicati consecutivi (prepending)
                    if (prev != -1 && prev != cur) edge_frequencies[{std::min(prev, cur), std::max(prev, cur)}]++;
                    prev = cur;
                } else {
                    prev = -1;  // token non valido: spezza il cammino, niente archi falsi
                }
                if (sep == path_end) break;
                field = sep + 1;
            }
        }
    }
}