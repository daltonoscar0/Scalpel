# Scalpel

[![tests](https://github.com/daltonoscar0/Scalpel/actions/workflows/tests.yml/badge.svg)](https://github.com/daltonoscar0/Scalpel/actions/workflows/tests.yml)

Rule-based C++ tokenizer and sentence segmenter.
Feed it raw text; get back typed tokens grouped into sentences, handling contractions, abbreviations, hyphenation, ellipses, and numbers without any model weights or dependencies.

An optional [Irish initial-mutation layer](#irish-initial-mutations) adds séimhiú, urú, h-prefix and t-prefix generation and analysis on top of UTF-8 aware tokenization.

```
Input  →  "Dr. Jones can't visit on Jan. 31st. Amazing!"

Sentence 1:  [Dr.]  ABBREVIATION  [Jones]  WORD  [can't]  CONTRACTION
             [visit]  WORD  [on]  WORD  [Jan.]  ABBREVIATION
             [31]  NUMBER  [st.]  ABBREVIATION  [.]  PUNCT

Sentence 2:  [Amazing]  WORD  [!]  PUNCT
```

---

## How it works

```
┌───────────────────────────────┐
│  Raw text  (std::string)      │
└───────────────┬───────────────┘
                │  char-by-char
                ▼
┌───────────────────────────────────────────────────────────┐
│  Tokenizer  (FSM)                                         │
│                                                           │
│   START ──letter──▶ IN_WORD ──apostrophe──▶ IN_CONTRACTION│
│     │                  │                                  │
│     │              hyphen                                 │
│     │                  ▼                                  │
│     │           IN_HYPHENATED                             │
│     │                                                     │
│     │           IN_WORD ──period (≤3 chars)──▶ IN_ABBREV  │
│     │                                                     │
│     ├──digit──▶ IN_NUMBER                                 │
│     │                                                     │
│     └──punct/period──▶ IN_PUNCT  (merges '...' ellipsis)  │
└───────────────────────────────┬───────────────────────────┘
                                │  std::vector<Token>
                                ▼
┌───────────────────────────────────────────────────────────┐
│  SentenceSegmenter                                        │
│                                                           │
│  Walk tokens; push to current sentence;                   │
│  on PUNCT('.','!','?') or SENTENCE_END → flush sentence   │
│  Abbreviations (ABBREVIATION type) never split sentences  │
└───────────────────────────────┬───────────────────────────┘
                                │  std::vector<std::vector<Token>>
                                ▼
                        Labelled sentences
```

### Why a finite state machine?

Character-by-character FSM processing means a single linear pass over the input with O(1) lookahead. There is no backtracking, no regex engine, and no heap allocation per character, just a state variable, a string buffer, and a switch. This makes boundary cases (e.g. `U.S.` vs `dogs.`) explicit and testable rather than buried in regex alternations.

### Why separate tokenizer and segmenter?

The tokenizer assigns types (`ABBREVIATION`, `CONTRACTION`, etc.) that the segmenter uses to decide whether a period ends a sentence. Keeping the two stages separate means each can be tested independently, and the segmenter can be swapped out without touching tokenization logic.

---

## Token types

| Type | Examples | Notes |
|---|---|---|
| `WORD` | `Hello`, `fox`, `31st` | Default alphabetic token |
| `NUMBER` | `3`, `42` | Pure digit sequences |
| `PUNCT` | `.`, `!`, `?`, `...` | Punctuation; `.`/`!`/`?` trigger sentence boundaries |
| `ABBREVIATION` | `Dr.`, `Jan.`, `U.S.` | Word ≤ 3 chars followed by a period; never splits sentences |
| `CONTRACTION` | `don't`, `I'll`, `can't` | Word + apostrophe + continuation |
| `HYPHENATED` | `state-of-the-art` | Word segments joined by hyphens |
| `SENTENCE_END` |, | Explicit marker; can be injected directly into the token stream |

---

## Prerequisites

| Tool | Version |
|---|---|
| C++ compiler | C++11 or later (g++ 4.8+, clang++ 3.3+) |

No external libraries. No build system required.

---

## Building

### Main program

```bash
g++ -std=c++11 -o tss \
    tss_main.cpp \
    tokenizer.cpp \
    char_classes.cpp \
    sentence_segmenter.cpp
```

### Tokenizer test suite

```bash
g++ -std=c++11 -o tokenizer_tests \
    tokenizer_tests.cpp \
    tokenizer.cpp \
    char_classes.cpp \
    -I.
```

### Sentence segmenter test suite

```bash
g++ -std=c++11 -o sentence_segmenter_tests \
    sentence_segmenter_tests.cpp \
    sentence_segmenter.cpp \
    -I.
```

### Integration test suite

```bash
g++ -std=c++11 -o tss_tests \
    tss_tests.cpp \
    tokenizer.cpp \
    char_classes.cpp \
    -I.
```

### Irish mutation test suite

`mutation_fst.cpp` and `mutation_tests.cpp` use structured bindings, so this
suite needs C++17 (the rest of the codebase stays C++11):

```bash
g++ -std=c++17 -o mutation_tests \
    mutation_tests.cpp \
    mutation_fst.cpp \
    tokenizer.cpp \
    char_classes.cpp \
    -I.
```

---

## Usage

Run the program, type (or pipe) input, then press **Ctrl-D** to signal end of input:

```bash
./tss
The quick brown fox jumps over 3 lazy dogs. Amazing!
^D
```

```
Sentence 1:
 [The] WORD
 [quick] WORD
 [brown] WORD
 [fox] WORD
 [jumps] WORD
 [over] WORD
 [3] NUMBER
 [lazy] WORD
 [dogs] WORD
 [.] PUNCT

Sentence 2:
 [Amazing] WORD
 [!] PUNCT
```

Pipe text directly:

```bash
echo "Dr. Meeden doesn't like state-of-the-art models. Does she?" | ./tss
```

---

## Irish initial mutations

`mutation_fst.h/.cpp` add an optional layer for the four Irish initial
mutations. The tokenizer's FSM is a *recognizer*; this layer is Scalpel's
string-rewriting (transducer) side, so it supplies its own FST machinery rather
than composing with the existing tokenizer rules.

| Tag | Mutation | Example |
|---|---|---|
| `Len` | séimhiú (lenition) | `fear` → `fhear` |
| `Ecl` | urú (eclipsis) | `fear` → `bhfear` |
| `HPref` | h-prefix | `aois` → `haois` |
| `TPref` | t-prefix | `Aontas` → `tAontas`, `aontas` → `t-aontas` |

Both directions are available:

```cpp
#include "mutation_fst.h"

ga::mutate("Ecl", "fear");     // generation → "bhfear"
ga::demutate("bhfear");        // analysis   → {("Len","bfear"), ("Ecl","fear")}
ga::analyze("bhfear");         // as above, plus ("none","bhfear") first
```

Note the second analysis candidate: `demutate` is deliberately
over-generating, so `bhfear` also yields the (spurious) lenition reading
`Len:bfear`. Filtering candidates against a lexicon is the caller's job.

`lenite`, `eclipse`, `h_prefix` and `t_prefix` expose the individual rules.
`fst_mutate` is an arc-based transducer path that must agree with `mutate` on
every input; the test suite enforces that parity. `inverse_ok` is the shared
pass criterion used against the reference implementation.

Design contracts, quoted from the header:

- **Pure application layer** — no triggers, no lexicon, no heuristics. Deciding
  *when* a mutation applies is the caller's job; this layer only applies it.
- **Vacuous application** — mutating a non-target returns the word unchanged.
- **Over-generating analysis** — `demutate` returns candidates and excludes the
  identity reading; adding it back is the caller's choice (or use `analyze`).
- **Caighdeán on output** — generation emits standard orthography. Analysis is
  more permissive and additionally accepts hyphen-before-capital (`n-Ealaíon`),
  hyphenated h (`h-aoise`), all-caps (`BHFUIL`), and signage-style lowercase
  prefixes on capitals (`tAONTAS`).

Tokenization became UTF-8 codepoint-aware (`utf8.h`) in the same change, so
mutated forms survive tokenization intact: the suite checks that `bhfear`
tokenizes as one token and still analyzes to `Ecl:fear`.

### Status and provenance

The header states that the semantics are pinned by a Python reference
implementation (`src/mutations.py`) and a shared attested-form fixture
(`tests/attested_mutations.tsv`) mined from UD Irish-IDT, with the C++ runner
required to reproduce the Python pass count exactly. `mutation_tests.cpp`
records that contract as 17200/17201 attested forms.

**Neither the Python reference nor the TSV fixture is in this repository.**
The attested-form check therefore reports
`[SKIP] attested TSV not found: ../../tests/attested_mutations.tsv`
and the 17200/17201 figure is not reproduced by the tests here. The suite's
other 11 checks run and pass. Pass a path to supply the fixture yourself:

```bash
./mutation_tests /path/to/attested_mutations.tsv
```

The Python reference implementation (`src/mutations.py`) and the attested-form fixture (`tests/attested_mutations.tsv`) live in [Morpheme-Aware-Language-Modeling-for-Irish](https://github.com/daltonoscar0/Morpheme-Aware-Language-Modeling-for-Irish). Clone it alongside Scalpel, or pass the fixture path to `mutation_tests` directly.

---

## Testing

```bash
./tokenizer_tests
```

```
[PASS]simple sentence
[PASS]contractions
[PASS]hyphenation
[PASS]abbreviations
[PASS]ellipsis

Tests run:5
Tests failed: 0
```

```bash
./sentence_segmenter_tests
```

```
Running Sentence Segmenter Tests
===============================

[PASS] basic sentence period: sentence count
[PASS] basic sentence period: content
... (21 tests total)

Tests run: 21
Tests failed: 0
```

---

## Project structure

```
scalpel/
├── tokenizer.h/.cpp            # FSM tokenizer — core of the pipeline
├── char_classes.h/.cpp         # Character classification (LETTER, DIGIT, PERIOD, etc.)
├── token.h                     # Token struct and TokenType enum
├── sentence_segmenter.h/.cpp   # Sentence boundary detection
├── utils.h                     # String utility declarations
├── utf8.h                      # UTF-8 codepoint helpers
├── mutation_fst.h/.cpp         # Irish initial mutations (séimhiú, urú, h-/t-prefix)
├── tss_main.cpp                # CLI entry point (stdin → labelled sentences)
├── tokenizer_tests.cpp         # Tokenizer unit tests
├── sentence_segmenter_tests.cpp# Sentence segmenter unit tests
├── tss_tests.cpp               # Integration tests (tokenizer end-to-end)
├── mutation_tests.cpp          # Irish mutation tests (C++17)
└── test_input.txt              # Sample input file
```

---

## FSM state reference

| State | Entered when | Exits when |
|---|---|---|
| `START` | Initial / after any token is emitted | Any non-whitespace character |
| `IN_WORD` | Letter seen in `START` | Non-letter (emits `WORD`), or apostrophe / hyphen / short-word period |
| `IN_NUMBER` | Digit seen in `START` | Non-digit (emits `NUMBER`) |
| `IN_PUNCT` | Punct or period seen in `START` | Non-punct, non-period (emits `PUNCT`; `...` merges naturally) |
| `IN_CONTRACTION` | Apostrophe seen in `IN_WORD` | Non-letter (emits `CONTRACTION`) |
| `IN_HYPHENATED` | Hyphen seen in `IN_WORD` | Non-letter (emits `HYPHENATED`) |
| `IN_ABBREVIATION` | Period seen in `IN_WORD` with buffer ≤ 3 chars | Non-letter, non-period (emits `ABBREVIATION`) |
| `POSSIBLE_SENTENCE_END` | Reserved for future lookahead disambiguation |, |

---

## Customisation

- **Abbreviation threshold:** the `buffer_.length() <= 3` check in `tokenizer.cpp` controls what counts as an abbreviation candidate. Raise it to catch longer abbreviations like `Prof.` (4 chars before the period).
- **Known abbreviations list:** `sentence_segmenter.h` has an `abbreviations` vector stub ready to hold an explicit list for finer-grained sentence boundary control.
- **Additional token types:** add a new `TokenType` in `token.h`, a matching `State` in `tokenizer.h`, and handle the transition in `process_char`.

---

## License

MIT. See [LICENSE](LICENSE).
