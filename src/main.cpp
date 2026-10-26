#include <charconv>
#include <cstring>
#include <exception>
#include <iostream>
#include <system_error>

#include "ExperimentalAnalysis.hpp"

namespace {

// Legge un intero >= 0 che occupa tutta la stringa s.
bool parse_nonneg(const char* s, int& out) {
    const char* end = s + std::strlen(s);
    if (s == end) return false;
    auto res = std::from_chars(s, end, out);
    return res.ec == std::errc() && res.ptr == end && out >= 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 5) {
        std::cerr << "uso: " << argv[0] << " <file_bgp> [Q] [S] [seed]\n";
        return 1;
    }

    int Q = 100000, S = 50, seed = 12345;
    if (argc >= 3 && !parse_nonneg(argv[2], Q)) {
        std::cerr << "Q non valido: " << argv[2] << "\n";
        return 1;
    }
    if (argc >= 4 && !parse_nonneg(argv[3], S)) {
        std::cerr << "S non valido: " << argv[3] << "\n";
        return 1;
    }
    if (argc >= 5 && !parse_nonneg(argv[4], seed)) {
        std::cerr << "seed non valido: " << argv[4] << "\n";
        return 1;
    }

    try {
        run_experiments(argv[1], Q, S, static_cast<unsigned int>(seed));
    } catch (const std::exception& e) {
        std::cerr << "errore: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
