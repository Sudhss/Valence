#include "cpp_highlighter.h"
#include <cctype>

CppHighlighter::CppHighlighter() {
    keywords_ = {
        "if", "else", "for", "while", "do", "switch", "case", "break",
        "continue", "return", "class", "struct", "enum", "namespace",
        "template", "typename", "public", "private", "protected",
        "virtual", "override", "final", "const", "constexpr", "static",
        "inline", "extern", "volatile", "mutable", "explicit",
        "void", "new", "delete", "try", "catch", "throw", "noexcept",
        "using", "typedef", "auto", "nullptr", "true", "false",
        "this", "operator", "sizeof", "alignof", "decltype",
        "static_cast", "dynamic_cast", "reinterpret_cast", "const_cast",
        "include", "define", "pragma", "ifdef", "ifndef", "endif", "elif",
        "default", "goto", "register", "friend", "concept", "requires",
        "co_await", "co_return", "co_yield", "consteval", "constinit"
    };

    types_ = {
        "int", "char", "float", "double", "bool", "long", "short",
        "unsigned", "signed", "wchar_t", "char8_t", "char16_t", "char32_t",
        "string", "vector", "map", "unordered_map", "set", "unordered_set",
        "array", "list", "deque", "stack", "queue", "pair", "tuple",
        "shared_ptr", "unique_ptr", "weak_ptr", "optional", "variant",
        "size_t", "ptrdiff_t", "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "QString", "QWidget", "QMainWindow", "QApplication",
        "QColor", "QFont", "QPainter", "QTimer", "QEvent",
        "std", "cout", "cin", "endl", "cerr",
        "ifstream", "ofstream", "fstream", "stringstream", "istringstream", "ostringstream"
    };
}

bool CppHighlighter::isIdentStart(char c) {
    return std::isalpha((unsigned char)c) || c == '_';
}

bool CppHighlighter::isIdentChar(char c) {
    return std::isalnum((unsigned char)c) || c == '_';
}

std::vector<Token> CppHighlighter::tokenize(const std::string& line, bool& inBlockComment) const {
    std::vector<Token> tokens;
    int i = 0;
    int len = (int)line.size();

    while (i < len) {
        // Inside block comment continuation
        if (inBlockComment) {
            int start = i;
            while (i < len) {
                if (i + 1 < len && line[i] == '*' && line[i + 1] == '/') {
                    i += 2;
                    inBlockComment = false;
                    break;
                }
                i++;
            }
            tokens.push_back({TokenType::Comment, start, i - start});
            continue;
        }

        char c = line[i];

        // Skip whitespace (emit as plain)
        if (std::isspace((unsigned char)c)) {
            int start = i;
            while (i < len && std::isspace((unsigned char)line[i])) i++;
            tokens.push_back({TokenType::Plain, start, i - start});
            continue;
        }

        // Line comment
        if (c == '/' && i + 1 < len && line[i + 1] == '/') {
            tokens.push_back({TokenType::Comment, i, len - i});
            i = len;
            continue;
        }

        // Block comment start
        if (c == '/' && i + 1 < len && line[i + 1] == '*') {
            int start = i;
            i += 2;
            inBlockComment = true;
            while (i < len) {
                if (i + 1 < len && line[i] == '*' && line[i + 1] == '/') {
                    i += 2;
                    inBlockComment = false;
                    break;
                }
                i++;
            }
            tokens.push_back({TokenType::Comment, start, i - start});
            continue;
        }

        // Preprocessor
        if (c == '#') {
            tokens.push_back({TokenType::Preprocessor, i, len - i});
            i = len;
            continue;
        }

        // String literal
        if (c == '"' || c == '\'') {
            char quote = c;
            int start = i;
            i++;
            while (i < len && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < len) i++; // skip escape
                i++;
            }
            if (i < len) i++; // closing quote
            tokens.push_back({TokenType::String, start, i - start});
            continue;
        }

        // Number
        if (std::isdigit((unsigned char)c) ||
            (c == '.' && i + 1 < len && std::isdigit((unsigned char)line[i + 1]))) {
            int start = i;
            if (c == '0' && i + 1 < len && (line[i + 1] == 'x' || line[i + 1] == 'X')) {
                i += 2;
                while (i < len && std::isxdigit((unsigned char)line[i])) i++;
            } else {
                while (i < len && (std::isdigit((unsigned char)line[i]) || line[i] == '.')) i++;
                if (i < len && (line[i] == 'e' || line[i] == 'E')) {
                    i++;
                    if (i < len && (line[i] == '+' || line[i] == '-')) i++;
                    while (i < len && std::isdigit((unsigned char)line[i])) i++;
                }
            }
            // Suffixes like f, L, u, ll
            while (i < len && (line[i] == 'f' || line[i] == 'F' || line[i] == 'l' ||
                               line[i] == 'L' || line[i] == 'u' || line[i] == 'U')) i++;
            tokens.push_back({TokenType::Number, start, i - start});
            continue;
        }

        // Identifier / keyword / type / function
        if (isIdentStart(c)) {
            int start = i;
            while (i < len && isIdentChar(line[i])) i++;
            std::string word = line.substr(start, i - start);

            // Check if followed by '(' → function
            int peek = i;
            while (peek < len && std::isspace((unsigned char)line[peek])) peek++;
            bool isFunc = (peek < len && line[peek] == '(');

            if (keywords_.count(word)) {
                tokens.push_back({TokenType::Keyword, start, i - start});
            } else if (types_.count(word)) {
                tokens.push_back({TokenType::Type, start, i - start});
            } else if (isFunc) {
                tokens.push_back({TokenType::Function, start, i - start});
            } else {
                tokens.push_back({TokenType::Plain, start, i - start});
            }
            continue;
        }

        // Punctuation (operators, braces, etc.)
        {
            int start = i;
            i++;
            tokens.push_back({TokenType::Punctuation, start, 1});
        }
    }

    return tokens;
}
