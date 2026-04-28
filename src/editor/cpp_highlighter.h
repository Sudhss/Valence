#pragma once
#include <vector>
#include <string>
#include <unordered_set>

enum class TokenType {
    Plain, Keyword, Type, String, Comment, Number,
    Preprocessor, Function, Punctuation
};

struct Token {
    TokenType type;
    int start;
    int length;
};

class CppHighlighter {
public:
    CppHighlighter();
    std::vector<Token> tokenize(const std::string& line, bool& inBlockComment) const;

private:
    std::unordered_set<std::string> keywords_;
    std::unordered_set<std::string> types_;

    static bool isIdentStart(char c);
    static bool isIdentChar(char c);
};
