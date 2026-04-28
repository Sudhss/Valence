#pragma once
#include <vector>
#include <string>
#include "selection.h"

class TextBuffer {
public:
    TextBuffer();

    // Single-char operations
    void insertChar(int row, int col, char ch);
    void deleteChar(int row, int col);
    void splitLine(int row, int col);
    void mergeLines(int row);

    // Range operations
    std::string getText(Position start, Position end) const;
    void deleteRange(Position start, Position end);
    Position insertText(int row, int col, const std::string& text);

    // Word operations
    int findWordBoundaryLeft(int row, int col) const;
    int findWordBoundaryRight(int row, int col) const;

    // Auto-indent
    std::string getLeadingWhitespace(int row) const;

    // File I/O
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;

    // Accessors
    int lineCount() const;
    int lineLength(int row) const;
    const std::string& line(int row) const;

    void clear();

private:
    std::vector<std::string> lines_;
};
