#include <iostream>
#include <vector>
#include <string>

#include "tokenizer.h"
#include "token.h"

int tests_run = 0;
int tests_failed = 0;

void assert_equal(std::string got, std::string expected, std::string test_name){
    tests_run ++;
    if (got != expected){
        tests_failed++;
        std::cout << "[FAIL]" << test_name << "\n";
        std::cout << "Expected: " << expected << "\n";
        std::cout << "Got: " << got << "\n\n";
    } else {
        std::cout << "[PASS]" << test_name << "\n";
    }
}

std::string join_tokens(std::vector<Token> tokens){
    std::string result;

    for (int i = 0; i < tokens.size(); i++){
        result += tokens[i].text;
        if (i + 1 < tokens.size()){
            result += "|";
        }
    }
    return result;
} 

void test_simple_sentence() {
    Tokenizer tokenizer;
    std::string input = "Hello world.";
    std::vector<Token> tokens = tokenizer.tokenize(input);

    std::string joined = join_tokens(tokens);
    assert_equal(joined, "Hello|world|.", "simple sentence");
}

void test_contractions() {
    Tokenizer tokenizer;
    std::string input = "I don't know.";
    std::vector<Token> tokens = tokenizer.tokenize(input);

    std::string joined = join_tokens(tokens);
    assert_equal(joined, "I|don't|know|.", "contractions");
}

void test_hyphenation() {
    Tokenizer tokenizer;
    std::string input = "state-of-the-art model";
    std::vector<Token> tokens = tokenizer.tokenize(input);

    std::string joined = join_tokens(tokens);
    assert_equal(joined, "state-of-the-art|model", "hyphenation");
}

void test_abbreviations() {
    Tokenizer tokenizer;
    std::string input = "Dr. Smith lives in the U.S.";
    std::vector<Token> tokens = tokenizer.tokenize(input);

    std::string joined = join_tokens(tokens);
    assert_equal(joined, "Dr.|Smith|lives|in|the|U.S.", "abbreviations");
}

void test_ellipsis() {
    Tokenizer tokenizer;
    std::string input = "Wait... what?";
    std::vector<Token> tokens = tokenizer.tokenize(input);

    std::string joined = join_tokens(tokens);
    assert_equal(joined, "Wait|...|what|?", "ellipsis");
}

int main(){
    test_simple_sentence();
    test_contractions();
    test_hyphenation();
    test_abbreviations();
    test_ellipsis();

    std::cout << "\nTests run:" << tests_run << "\n";
    std::cout << "Tests failed: " << tests_failed << "\n";

    if (tests_failed > 0){
        return 1;
    }
    return 0;
}