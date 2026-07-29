#include "mutation_fst.h"
#include "utf8.h"

#include <algorithm>
#include <cassert>
#include <functional>

namespace ga {

using CP = uint32_t;
using CPs = std::vector<CP>;

namespace {

const std::string VOWELS_S = "aeiou\u00e1\u00e9\u00ed\u00f3\u00fa";  // aeiouáéíóú

bool is_vowel(CP cp) {
    static const CPs V = utf8::to_codepoints(VOWELS_S);
    CP l = utf8::to_lower(cp);
    return std::find(V.begin(), V.end(), l) != V.end();
}

bool is_s_follower(CP cp) {  // s mutates only before vowel / l / n / r
    CP l = utf8::to_lower(cp);
    return is_vowel(l) || l == 'l' || l == 'n' || l == 'r';
}

bool is_lenitable(CP cp) {   // excluding conditional s
    CP l = utf8::to_lower(cp);
    return l == 'b' || l == 'c' || l == 'd' || l == 'f' || l == 'g' ||
           l == 'm' || l == 'p' || l == 't';
}

// eclipsis prefix for a consonant (empty = not eclipsable); base letter kept
std::string ecl_prefix(CP cp) {
    switch (utf8::to_lower(cp)) {
        case 'b': return "m";  case 'c': return "g";  case 'd': return "n";
        case 'f': return "bh"; case 'g': return "n";  case 'p': return "b";
        case 't': return "d";  default:  return "";
    }
}

bool word_is_upper(const CPs& w) {  // mirrors Python str.isupper()
    bool any_cased = false;
    for (CP c : w) {
        if (utf8::is_lower(c)) return false;
        if (utf8::is_upper(c)) any_cased = true;
    }
    return any_cased;
}

CPs lower_all(const CPs& w) {
    CPs out(w);
    for (CP& c : out) c = utf8::to_lower(c);
    return out;
}

CPs upper_all(const CPs& w) {
    CPs out(w);
    for (CP& c : out) c = utf8::to_upper(c);
    return out;
}

CPs cps(const std::string& s) { return utf8::to_codepoints(s); }
std::string str(const CPs& v) { return utf8::from_codepoints(v); }

CPs cat(const std::string& prefix, const CPs& rest) {
    CPs out = cps(prefix);
    out.insert(out.end(), rest.begin(), rest.end());
    return out;
}

CPs slice(const CPs& w, size_t from) { return CPs(w.begin() + from, w.end()); }

// ---- generation on codepoints ----------------------------------------------

CPs lenite_cp(const CPs& w) {
    if (w.empty()) return w;
    CP l0 = utf8::to_lower(w[0]);
    bool hits = is_lenitable(w[0]) ||
                (l0 == 's' && w.size() > 1 && is_s_follower(w[1]));
    if (!hits) return w;
    CPs out{w[0], 'h'};
    out.insert(out.end(), w.begin() + 1, w.end());
    return out;
}

CPs eclipse_cp(const CPs& w) {
    if (w.empty()) return w;
    std::string pre = ecl_prefix(w[0]);
    if (!pre.empty()) return cat(pre, w);
    if (is_vowel(w[0])) return cat(utf8::is_upper(w[0]) ? "n" : "n-", w);
    return w;
}

CPs h_prefix_cp(const CPs& w) {
    if (!w.empty() && is_vowel(w[0])) return cat("h", w);
    return w;
}

CPs t_prefix_cp(const CPs& w) {
    if (w.empty()) return w;
    if (is_vowel(w[0])) return cat(utf8::is_upper(w[0]) ? "t" : "t-", w);
    if (utf8::to_lower(w[0]) == 's' && w.size() > 1 && is_s_follower(w[1]))
        return cat("t", w);
    return w;
}

CPs plain_mutate(const std::string& tag, const CPs& w) {
    if (tag == "Len")   return lenite_cp(w);
    if (tag == "Ecl")   return eclipse_cp(w);
    if (tag == "HPref") return h_prefix_cp(w);
    if (tag == "TPref") return t_prefix_cp(w);
    assert(false && "unknown mutation tag");
    return w;
}

// caps-aware wrapper: ALL-CAPS input uppercases inserted material too
CPs mutate_cp(const std::string& tag, const CPs& w) {
    if (w.size() > 1 && word_is_upper(w)) return upper_all(plain_mutate(tag, w));
    return plain_mutate(tag, w);
}

// ---- analysis on codepoints ------------------------------------------------

void push_unique(std::vector<std::pair<std::string, CPs>>& out,
                 const std::string& tag, const CPs& base) {
    for (auto& c : out)
        if (c.first == tag && c.second == base) return;
    out.emplace_back(tag, base);
}

std::vector<std::pair<std::string, CPs>> demutate_core(const CPs& w) {
    std::vector<std::pair<std::string, CPs>> out;
    size_t n = w.size();
    if (n < 2) return out;
    CP c0 = w[0], l0 = utf8::to_lower(c0), c1 = w[1];
    // Len: X + inserted lowercase 'h'
    if (c1 == 'h') {
        if (is_lenitable(c0) && l0 != 's') {
            CPs base{c0};
            base.insert(base.end(), w.begin() + 2, w.end());
            push_unique(out, "Len", base);
        }
        if (l0 == 's' && n >= 3 && is_s_follower(w[2])) {
            CPs base{c0};
            base.insert(base.end(), w.begin() + 2, w.end());
            push_unique(out, "Len", base);
        }
    }
    // Ecl consonants (prefix always lowercase; base letter keeps its case)
    if (c0 == 'b' && c1 == 'h' && n >= 3 && utf8::to_lower(w[2]) == 'f') {
        push_unique(out, "Ecl", slice(w, 2));               // bh + fear
    } else {
        static const std::pair<char, char> pairs[] = {
            {'m', 'b'}, {'g', 'c'}, {'n', 'd'}, {'n', 'g'}, {'b', 'p'}, {'d', 't'}};
        for (auto [pre, base_c] : pairs)
            if (c0 == static_cast<CP>(pre) && utf8::to_lower(c1) == static_cast<CP>(base_c))
                push_unique(out, "Ecl", slice(w, 1));
    }
    // Ecl vowels
    if (c0 == 'n') {
        if (c1 == '-' && n >= 3 && is_vowel(w[2]) && utf8::is_lower(w[2]))
            push_unique(out, "Ecl", slice(w, 2));           // n-éan
        else if (is_vowel(c1) && utf8::is_upper(c1))
            push_unique(out, "Ecl", slice(w, 1));           // nÉan
    }
    // HPref
    if (c0 == 'h' && is_vowel(c1))
        push_unique(out, "HPref", slice(w, 1));
    // TPref
    if (c0 == 't') {
        if (c1 == '-' && n >= 3 && is_vowel(w[2]) && utf8::is_lower(w[2]))
            push_unique(out, "TPref", slice(w, 2));         // t-ainm
        else if (is_vowel(c1) && utf8::is_upper(c1))
            push_unique(out, "TPref", slice(w, 1));         // tAthair
        else if (utf8::to_lower(c1) == 's' && n >= 3 && is_s_follower(w[2]))
            push_unique(out, "TPref", slice(w, 1));         // tsúil
    }
    return out;
}

bool inverse_ok_cp(const std::string& tag, const CPs& base, const CPs& surface);

std::vector<std::pair<std::string, CPs>> demutate_cp(const CPs& w) {
    auto out = demutate_core(w);
    // ALL-CAPS track: match lowercased, recover base by uppercasing
    if (w.size() > 2 && word_is_upper(w))
        for (auto& [tag, base_l] : demutate_core(lower_all(w)))
            push_unique(out, tag, upper_all(base_l));
    // hyphen-before-capital / hyphenated-h variants
    if (w.size() > 2 && w[1] == '-' && is_vowel(w[2])) {
        static const std::pair<char, const char*> variants[] = {
            {'n', "Ecl"}, {'h', "HPref"}, {'t', "TPref"}};
        for (auto [pre, tag] : variants)
            if (w[0] == static_cast<CP>(pre) && (utf8::is_upper(w[2]) || pre == 'h'))
                push_unique(out, tag, slice(w, 2));
    }
    for (auto& [tag, base] : out) {
        (void)tag; (void)base;
        assert(inverse_ok_cp(tag, base, w));
    }
    return out;
}

bool ends_with(const CPs& s, const CPs& suffix) {
    return s.size() >= suffix.size() &&
           std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

bool inverse_ok_cp(const std::string& tag, const CPs& base, const CPs& surface) {
    CPs m = mutate_cp(tag, base);
    if (m == surface) return true;
    if (surface.size() > 2 && surface[1] == '-' &&
        (surface[0] == 'n' || surface[0] == 'h' || surface[0] == 't')) {
        CPs dehyph{surface[0]};
        dehyph.insert(dehyph.end(), surface.begin() + 2, surface.end());
        if (m == dehyph) return true;
    }
    // case-flexible mutation material (BHFUIL vs tAONTAS conventions);
    // base letters must be unchanged
    if (lower_all(m) == lower_all(surface)) {
        if (tag == "Len")
            return !base.empty() && surface.size() >= 2 &&
                   surface[0] == base[0] &&
                   CPs(surface.begin() + 2, surface.end()) ==
                       CPs(base.begin() + 1, base.end());
        return ends_with(surface, base);
    }
    return false;
}

// ---- arc-based generation transducer (parity engine) -----------------------
// Mirrors src/mutations.py:build_fst — priority-union semantics: no
// accepting path => identity. Exists so generation is demonstrably a
// transducer run, not just string surgery; parity is test-enforced.

struct Fst {
    struct Arc {
        std::function<bool(CP)> match;
        std::function<std::string(CP)> emit;
        int next;  // -1 = COPY
    };
    std::vector<std::vector<Arc>> arcs{2};  // states 0,1 (+COPY sentinel)

    std::string apply(const CPs& w) const {  // "" => no path (identity)
        std::function<std::string(int, size_t)> step =
            [&](int state, size_t i) -> std::string {
            if (state == -1) {  // COPY
                std::string rest;
                for (size_t k = i; k < w.size(); ++k) utf8::append(rest, w[k]);
                return std::string(1, '\1') + rest;   // \1 marks acceptance
            }
            if (i >= w.size()) return "";
            for (const auto& a : arcs[state])
                if (a.match(w[i])) {
                    std::string r = step(a.next, i + 1);
                    if (!r.empty()) return r.insert(1, a.emit(w[i]));
                }
            return "";
        };
        return step(0, 0);
    }
};

Fst build_fst(const std::string& tag) {
    Fst f;
    auto copy1 = [](CP c) { std::string s; utf8::append(s, c); return s; };
    if (tag == "Len") {
        f.arcs[0].push_back({[](CP c) { return is_lenitable(c) &&
                                               utf8::to_lower(c) != 's'; },
                             [](CP c) { std::string s; utf8::append(s, c);
                                        return s + "h"; }, -1});
        f.arcs[0].push_back({[](CP c) { return utf8::to_lower(c) == 's'; },
                             [](CP c) { std::string s; utf8::append(s, c);
                                        return s + "h"; }, 1});
        f.arcs[1].push_back({is_s_follower, copy1, -1});
    } else if (tag == "Ecl") {
        f.arcs[0].push_back({[](CP c) { return !ecl_prefix(c).empty(); },
                             [](CP c) { std::string s = ecl_prefix(c);
                                        utf8::append(s, c); return s; }, -1});
        f.arcs[0].push_back({is_vowel,
                             [](CP c) { std::string s = utf8::is_upper(c) ? "n" : "n-";
                                        utf8::append(s, c); return s; }, -1});
    } else if (tag == "HPref") {
        f.arcs[0].push_back({is_vowel,
                             [](CP c) { std::string s = "h";
                                        utf8::append(s, c); return s; }, -1});
    } else if (tag == "TPref") {
        f.arcs[0].push_back({is_vowel,
                             [](CP c) { std::string s = utf8::is_upper(c) ? "t" : "t-";
                                        utf8::append(s, c); return s; }, -1});
        f.arcs[0].push_back({[](CP c) { return utf8::to_lower(c) == 's'; },
                             [](CP c) { std::string s = "t";
                                        utf8::append(s, c); return s; }, 1});
        f.arcs[1].push_back({is_s_follower, copy1, -1});
    }
    return f;
}

}  // namespace

// ---- public API -------------------------------------------------------------

std::string lenite(const std::string& w)   { return str(lenite_cp(cps(w))); }
std::string eclipse(const std::string& w)  { return str(eclipse_cp(cps(w))); }
std::string h_prefix(const std::string& w) { return str(h_prefix_cp(cps(w))); }
std::string t_prefix(const std::string& w) { return str(t_prefix_cp(cps(w))); }

std::string mutate(const std::string& tag, const std::string& word) {
    return str(mutate_cp(tag, cps(word)));
}

std::vector<Analysis> demutate(const std::string& w) {
    std::vector<Analysis> out;
    for (auto& [tag, base] : demutate_cp(cps(w)))
        out.emplace_back(tag, str(base));
    return out;
}

std::vector<Analysis> analyze(const std::string& w) {
    std::vector<Analysis> out{{"none", w}};
    for (auto& a : demutate(w)) out.push_back(a);
    return out;
}

bool inverse_ok(const std::string& tag, const std::string& base,
                const std::string& surface) {
    return inverse_ok_cp(tag, cps(base), cps(surface));
}

std::string fst_mutate(const std::string& tag, const std::string& word) {
    CPs w = cps(word);
    bool caps = w.size() > 1 && word_is_upper(w);
    std::string r = build_fst(tag).apply(w);
    if (r.empty()) return word;                    // no path -> identity
    r.erase(0, 1);                                 // strip acceptance marker
    if (caps) return str(upper_all(cps(r)));
    return r;
}

}  // namespace ga
