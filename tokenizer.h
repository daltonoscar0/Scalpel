#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "token.h"
#include "char_classes.h"

class Tokenizer{
    public:
    Tokenizer();

    //Core entry point
    std::vector<Token> tokenize(std::string input);
    void reset();

    private:
    enum class State{
        START,
        IN_WORD,
        IN_NUMBER,
        IN_ABBREVIATION,
        IN_CONTRACTION,
        IN_HYPHENATED,
        IN_PUNCT,
        POSSIBLE_SENTENCE_END,
    };

    State state_;
    std::string buffer_;
    int token_start_;

    //FSM processing functions
    void process_char(uint32_t cp, int index, std::vector<Token>& output);
    void emit_token(std::vector<Token>& output, int end_Index, TokenType type);
    void transition_to(State next);

    //Linguistic heuristics (all pass by value)
    bool is_abbreviation_candidate(std::string s);
    bool is_contraction_boundary(char prev, char curr);
    bool is_hyphen_internal(char prev, char next);
};