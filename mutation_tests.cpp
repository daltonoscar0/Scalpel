// Tests for the mutation FST layer, in Scalpel's house test style.
// Includes the attested-form parity run: reading the identical
// tests/attested_mutations.tsv used by the Python reference; pass counts
// must match the reference exactly (17200/17201 as of session 2).
//
// Build:
//   g++ -std=c++17 -o mutation_tests mutation_tests.cpp mutation_fst.cpp \
//       tokenizer.cpp char_classes.cpp -I.  (one line)
// Run:
//   ./mutation_tests path/to/attested_mutations.tsv
#include "mutation_fst.h"
#include "tokenizer.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static int run = 0, failed = 0;

static void check(bool ok, const std::string& name) {
    ++run;
    if (!ok) { ++failed; std::cout << "[FAIL]" << name << "\n"; }
    else     {           std::cout << "[PASS]" << name << "\n"; }
}

static bool has(const std::vector<ga::Analysis>& v,
                const std::string& tag, const std::string& base) {
    for (auto& a : v) if (a.first == tag && a.second == base) return true;
    return false;
}

struct Pair { const char* base; const char* surf; };

static void generation_tests() {
    const Pair len[] = {{"bean","bhean"},{"cóta","chóta"},{"doras","dhoras"},
        {"fear","fhear"},{"máthair","mháthair"},{"póca","phóca"},
        {"teach","theach"},{"súil","shúil"},{"sráid","shráid"},
        {"slat","shlat"},{"sneachta","shneachta"},{"Conradh","Chonradh"},
        {"Seán","Sheán"}};
    bool ok = true;
    for (auto& p : len) ok = ok && ga::lenite(p.base) == p.surf;
    for (const char* b : {"scoil","smaoineamh","spéir","staidéar","lá",
                          "nead","rud","hata","ábhar","uisce"})
        ok = ok && ga::lenite(b) == b;
    check(ok, "lenition incl. s-blocking and vacuous application");

    const Pair ecl[] = {{"bó","mbó"},{"ceist","gceist"},{"doras","ndoras"},
        {"fear","bhfear"},{"gort","ngort"},{"páirc","bpáirc"},{"tír","dtír"},
        {"éan","n-éan"},{"uisce","n-uisce"},{"Baile","mBaile"},
        {"Frainc","bhFrainc"},{"Tír","dTír"},{"Éire","nÉire"},
        {"Gaillimh","nGaillimh"}};
    ok = true;
    for (auto& p : ecl) ok = ok && ga::eclipse(p.base) == p.surf;
    for (const char* b : {"lá","máthair","nead","rud","sráid","hata"})
        ok = ok && ga::eclipse(b) == b;
    check(ok, "eclipsis incl. bhf, vowels, capitals, vacuous");

    const Pair hp[] = {{"aon","haon"},{"óige","hóige"},{"Éirinn","hÉirinn"},
        {"é","hé"},{"uimhreacha","huimhreacha"}};
    ok = true;
    for (auto& p : hp) ok = ok && ga::h_prefix(p.base) == p.surf;
    ok = ok && ga::h_prefix("teach") == "teach";
    check(ok, "h-prefix");

    const Pair tp[] = {{"uisce","t-uisce"},{"ainm","t-ainm"},
        {"Athair","tAthair"},{"Uachtarán","tUachtarán"},{"súil","tsúil"},
        {"sráid","tsráid"},{"Sín","tSín"},{"samhlaíocht","tsamhlaíocht"}};
    ok = true;
    for (auto& p : tp) ok = ok && ga::t_prefix(p.base) == p.surf;
    for (const char* b : {"scoil","spéir","teach","doras"})
        ok = ok && ga::t_prefix(b) == b;
    check(ok, "t-prefix incl. ts and blocked clusters");

    check(ga::mutate("Len","BLIAIN") == "BHLIAIN" &&
          ga::mutate("Ecl","FUIL") == "BHFUIL" &&
          ga::mutate("Ecl","BRIATHAR") == "MBRIATHAR" &&
          ga::mutate("Ecl","Ealaíon") == "nEalaíon" &&
          ga::mutate("HPref","aoise") == "haoise",
          "all-caps generation; canonical stays Caighdeán");
}

static void analysis_tests() {
    check(has(ga::demutate("bhfear"), "Ecl", "fear") &&
          has(ga::demutate("shráid"), "Len", "sráid") &&
          has(ga::demutate("n-éan"),  "Ecl", "éan") &&
          has(ga::demutate("tsúil"),  "TPref", "súil") &&
          has(ga::demutate("hÉireann"), "HPref", "Éireann") &&
          has(ga::demutate("Chonradh"), "Len", "Conradh"),
          "demutation core round trips");
    check(has(ga::analyze("hata"), "none", "hata") &&
          has(ga::analyze("hata"), "HPref", "ata"),
          "genuine h-word keeps identity reading");
    check(has(ga::demutate("BHFUIL"), "Ecl", "FUIL") &&
          has(ga::demutate("BHLIAIN"), "Len", "BLIAIN") &&
          has(ga::demutate("n-Ealaíon"), "Ecl", "Ealaíon") &&
          has(ga::demutate("h-aoise"), "HPref", "aoise") &&
          has(ga::demutate("tAONTAS"), "TPref", "AONTAS"),
          "all-caps and orthographic-variant tracks");
    bool none_tpref = true;
    for (auto& a : ga::demutate("tscoil"))
        if (a.first == "TPref") none_tpref = false;
    check(none_tpref, "no TPref before blocked cluster (tsc)");
}

static void parity_tests() {
    const char* words[] = {"bean","cóta","fear","súil","sráid","scoil",
        "smaoineamh","lá","rud","hata","éan","uisce","Éire","Baile","Frainc",
        "Tír","tír","gort","páirc","Athair","samhlaíocht","BLIAIN","FUIL",
        "Ealaíon","aoise","é","ábhar","Gaillimh","teach","doras"};
    bool ok = true;
    for (const char* tag : {"Len","Ecl","HPref","TPref"})
        for (const char* w : words)
            if (ga::fst_mutate(tag, w) != ga::mutate(tag, w)) {
                ok = false;
                std::cout << "  parity mismatch " << tag << "(" << w << "): "
                          << ga::fst_mutate(tag, w) << " vs "
                          << ga::mutate(tag, w) << "\n";
            }
    check(ok, "transducer engine parity with direct rules");
}

static void integration_test() {
    Tokenizer tk;
    auto tokens = tk.tokenize(
        "Bhí an tsúil ghorm ag an bhfear óg i mBaile Átha Cliath.");
    bool found_ecl = false, whole = false;
    for (auto& t : tokens) {
        if (t.text == "bhfear") {
            whole = true;
            found_ecl = has(ga::analyze(t.text), "Ecl", "fear");
        }
    }
    check(whole && found_ecl,
          "tokenize Irish -> analyze: bhfear survives and yields Ecl:fear");
}

static void attested_parity(const std::string& tsv) {
    std::ifstream in(tsv);
    if (!in) {
        std::cout << "[SKIP] attested TSV not found: " << tsv << "\n";
        return;
    }
    long total = 0, pass = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string surface, lemma, upos, tag, split;
        std::getline(ss, surface, '\t'); std::getline(ss, lemma, '\t');
        std::getline(ss, upos, '\t');    std::getline(ss, tag, '\t');
        std::getline(ss, split, '\t');
        ++total;
        for (auto& [t, base] : ga::demutate(surface))
            if (t == tag && ga::inverse_ok(t, base, surface)) { ++pass; break; }
    }
    std::cout << "attested: " << pass << "/" << total << " pass\n";
    // Parity contract with the Python reference (session 2): 17200/17201.
    check(total == 17201 && pass == 17200,
          "attested-form parity with Python reference (17200/17201)");
}

int main(int argc, char** argv) {
    generation_tests();
    analysis_tests();
    parity_tests();
    integration_test();
    attested_parity(argc > 1 ? argv[1] : "../../tests/attested_mutations.tsv");
    std::cout << "\nTests run:" << run << "\nTests failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
