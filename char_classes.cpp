#include "char_classes.h"
#include <cctype>
#include "utf8.h"

// Classifies a character by checking its type and returning the appropriate CharClass enum value
CharClass CharClassifier::classify_char(char c){
    if (std::isalpha(c)){
        return CharClass::LETTER;
    } else if (std::isdigit(c)){
        return CharClass::DIGIT;
    } else if (std::isspace(c)){
        return CharClass::WHITESPACE;
    } else if (c == '.'){
        return CharClass::PERIOD;
    } else if (c == '\''){
        return CharClass::APOSTROPHE;
    } else if (c == '-'){
        return CharClass::HYPHEN;
    } else if (std::ispunct(c)){
        return CharClass::PUNCT;
    } else {
        return CharClass::OTHER;
    }
}


CharClass CharClassifier::classify_codepoint(uint32_t cp){
    if (cp < 0x80) return classify_char(static_cast<char>(cp));
    if (utf8::is_letter(cp)) return CharClass::LETTER;
    if (cp == 0x2019) return CharClass::APOSTROPHE;   // ' curly apostrophe
    if (cp == 0x2013 || cp == 0x2014 ||               // -, dashes
        cp == 0x2018 || cp == 0x201C || cp == 0x201D ||
        cp == 0x2026)                                  // … ellipsis
        return CharClass::PUNCT;
    return CharClass::OTHER;
}
