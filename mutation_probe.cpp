// Diagnostic probe for the mutation layer. Reads one command per line on
// stdin and prints one result line per command, so the Python reference and
// this implementation can be compared cell by cell over a case matrix or
// over a corpus.
//
// Commands (tab separated):
//   A <word>                  analysis: every (tag, base) and its inverse_ok
//   C <word>                  candidates before verification, same format
//   G <tag> <base>            generation: mutate(tag, base)
//   I <tag> <base> <surface>  inverse_ok(tag, base, surface)
//
// Output:
//   A <word> <tab> tag:base:0|1 ...      (1 = inverse_ok holds)
//   G <tag> <base> <tab> <surface>
//   I <tag> <base> <surface> <tab> 0|1
//
// Build two ways. Without NDEBUG the self-check inside demutate() still
// aborts, which is how the matrix records which cells abort. With
// -DNDEBUG the assert is compiled out and analysis returns its readings
// regardless, which is how the matrix records what those readings were:
//
//   g++ -std=c++17 -O2 -o mutation_probe mutation_probe.cpp mutation_fst.cpp \
//       char_classes.cpp -I.
//   g++ -std=c++17 -O2 -DNDEBUG -o mutation_probe_nd mutation_probe.cpp \
//       mutation_fst.cpp char_classes.cpp -I.
#include "mutation_fst.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> out;
    std::string field;
    std::stringstream ss(line);
    while (std::getline(ss, field, '\t')) out.push_back(field);
    return out;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        auto f = split_tabs(line);
        if (f.empty()) { std::cout << "\n"; continue; }

        if (f[0] == "A" && f.size() >= 2) {
            std::cout << "A\t" << f[1] << "\t";
            bool first = true;
            for (const auto& [tag, base] : ga::demutate(f[1])) {
                if (!first) std::cout << " ";
                first = false;
                std::cout << tag << ":" << base << ":"
                          << (ga::inverse_ok(tag, base, f[1]) ? 1 : 0);
            }
            std::cout << "\n";
        } else if (f[0] == "C" && f.size() >= 2) {
            std::cout << "C\t" << f[1] << "\t";
            bool first = true;
            for (const auto& [tag, base] : ga::demutate_candidates(f[1])) {
                if (!first) std::cout << " ";
                first = false;
                std::cout << tag << ":" << base << ":"
                          << (ga::inverse_ok(tag, base, f[1]) ? 1 : 0);
            }
            std::cout << "\n";
        } else if (f[0] == "G" && f.size() >= 3) {
            std::cout << "G\t" << f[1] << "\t" << f[2] << "\t"
                      << ga::mutate(f[1], f[2]) << "\n";
        } else if (f[0] == "I" && f.size() >= 4) {
            std::cout << "I\t" << f[1] << "\t" << f[2] << "\t" << f[3] << "\t"
                      << (ga::inverse_ok(f[1], f[2], f[3]) ? 1 : 0) << "\n";
        } else {
            std::cout << "?\t" << line << "\n";
        }
        std::cout.flush();
    }
    return 0;
}
