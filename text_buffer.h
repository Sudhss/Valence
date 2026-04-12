#pragma once
#include <vector>
#include <string>

class TextBuffer {
public:
    TextBuffer();

    void insertChar(int row, int col, char ch);
    void deleteChar(int row, int col);
    void splitLine(int row, int col);
    void mergeLines(int row);

    int getLineCount() const;
    int getLineLength(int row) const;
    const std::string& getLine(int row) const;

private:
    std::vector<std::string> lines;
};
