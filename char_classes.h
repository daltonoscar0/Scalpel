#pragma once
#include <cstdint>

enum class CharClass{
    LETTER,
    DIGIT,
    WHITESPACE,
    PERIOD,
    APOSTROPHE,
    HYPHEN, 
    PUNCT,
    OTHER
};

class CharClassifier{
    public:
    CharClass classify_char(char c);
    // Codepoint-aware classification (UTF-8 pipeline). Covers Latin letters
    // incl. Irish accented vowels; curly apostrophe U+2019 classifies as
    // APOSTROPHE, other non-ASCII punctuation as PUNCT/OTHER.
    CharClass classify_codepoint(uint32_t cp);
};