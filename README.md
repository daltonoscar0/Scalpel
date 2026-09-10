# scalpel

Rule-based C++ tokenizer and sentence segmenter.
Feed it raw text; get back typed tokens grouped into sentences, handling contractions, abbreviations, hyphenation, ellipses, and numbers without any model weights or dependencies.

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
├── tss_main.cpp                # CLI entry point (stdin → labelled sentences)
├── tokenizer_tests.cpp         # Tokenizer unit tests
├── sentence_segmenter_tests.cpp# Sentence segmenter unit tests
├── tss_tests.cpp               # Integration tests (tokenizer end-to-end)
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
