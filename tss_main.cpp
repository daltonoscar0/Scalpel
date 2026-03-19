#include <iostream>
#include <string>
#include <vector>

#include "tokenizer.h"
#include "sentence_segmenter.h"
#include "token.h"

int main(){
    std::string input;
    char c;

    while (std::cin.get(c)){
        input.push_back(c);
    }

    if (input.size() == 0){
        input = "Dr. Meeden doesn't like state-of-the-art models. Does she?";
    }

    Tokenizer tokenizer;
    std::vector<Token> tokens = tokenizer.tokenize(input);

    SentenceSegmenter segmenter;
    std::vector<std::vector<Token>> sentences = segmenter.segment(tokens);

    for (int i = 0; i < sentences.size(); i++){
        std::cout << "Sentence " << i + 1 << ":\n";

        for (int j = 0; j < sentences[i].size(); j++){
            Token t = sentences[i][j];
            std::cout << " [" << t.text << "] ";

            if (t.type == TokenType::WORD) {
                std::cout << "WORD";
            } else if (t.type == TokenType::NUMBER) {
                std::cout << "NUMBER";
            } else if (t.type == TokenType::PUNCT) {
                std::cout << "PUNCT";
            } else if (t.type == TokenType::ABBREVIATION) {
                std::cout << "ABBREVIATION";
            } else if (t.type == TokenType::CONTRACTION) {
                std::cout << "CONTRACTION";
            } else if (t.type == TokenType::HYPHENATED) {
                std::cout << "HYPHENATED";
            } else if (t.type == TokenType::SENTENCE_END) {
                std::cout << "SENTENCE_END";
            }

            std::cout << "\n";
        }

        std::cout << "\n";
    }
    return 0;
}
