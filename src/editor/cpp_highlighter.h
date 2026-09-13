#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <set>

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
    // Transparent comparators, so a std::string_view can be looked up directly.
    // tokenize() runs for every visible line on every repaint; building a
    // std::string per identifier just to probe a set is pure waste.
    std::set<std::string, std::less<>> keywords_;
    std::set<std::string, std::less<>> types_;

    static bool isIdentStart(char c);
    static bool isIdentChar(char c);
};
