#pragma once
// Irish initial-mutation layer for Scalpel: séimhiú (Len), urú (Ecl),
// h-prefix (HPref), t-prefix (TPref).
//
// This is Scalpel's string-rewriting (transducer) layer, the tokenizer's
// FSM is a recognizer, so this module adds the FST machinery rather than
// composing with pre-existing rules (see ARCHITECTURE.md, revised).
//
// Semantics are pinned by the Python reference implementation
// (src/mutations.py) and by the shared attested-form test file
// (tests/attested_mutations.tsv) mined from UD Irish-IDT; the C++ test
// runner must reproduce the Python pass count exactly.
//
// Design contracts (identical to the reference):
//   - pure application layer: no triggers, no lexicon, no heuristics
//   - vacuous application: mutating a non-target returns the word unchanged
//   - analysis over-generates candidates; the identity reading is the
//     caller's to add (see analyze())
//   - generation emits Caighdeán orthography; analysis additionally accepts
//     hyphen-before-capital (n-Ealaíon), hyphenated h (h-aoise), all-caps
//     (BHFUIL) and signage-style lowercase prefixes on caps (tAONTAS).
#include <string>
#include <utility>
#include <vector>

namespace ga {

// (tag, base); tag in {"Len","Ecl","HPref","TPref"}, or "none" from analyze()
using Analysis = std::pair<std::string, std::string>;

// Generation (caps-aware): tag in {"Len","Ecl","HPref","TPref"}.
std::string mutate(const std::string& tag, const std::string& word);

// Individual rules (Caighdeán, not caps-aware; exposed for tests).
std::string lenite(const std::string& w);
std::string eclipse(const std::string& w);
std::string h_prefix(const std::string& w);
std::string t_prefix(const std::string& w);

// Analysis: candidate (tag, base) pairs, excluding identity.
std::vector<Analysis> demutate(const std::string& w);
// As demutate, plus the ("none", w) reading in first position.
std::vector<Analysis> analyze(const std::string& w);

// Does mutate(tag, base) yield surface, modulo accepted orthographic
// variants? Shared pass criterion with the Python reference.
bool inverse_ok(const std::string& tag, const std::string& base,
                const std::string& surface);

// Generation via the arc-based transducer engine (parity path; must agree
// with mutate() on every input, enforced in mutation_tests.cpp).
std::string fst_mutate(const std::string& tag, const std::string& word);

}  // namespace ga
