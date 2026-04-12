#include "text_buffer.h"

TextBuffer::TextBuffer() {
    lines.push_back("");
}

void TextBuffer::insertChar(int row, int col, char ch) {
    lines[row].insert(col, 1, ch);
}

void TextBuffer::deleteChar(int row, int col) {
    if (col == 0) {
        mergeLines(row);
        return;
    }
    lines[row].erase(col - 1, 1);
}

void TextBuffer::splitLine(int row, int col) {
    std::string tail = lines[row].substr(col);
    lines[row] = lines[row].substr(0, col);
    lines.insert(lines.begin() + row + 1, tail);
}

void TextBuffer::mergeLines(int row) {
    if (row == 0) return;
    lines[row - 1] += lines[row];
    lines.erase(lines.begin() + row);
}

int TextBuffer::getLineCount() const {
    return lines.size();
}

int TextBuffer::getLineLength(int row) const {
    return lines[row].size();
}

const std::string& TextBuffer::getLine(int row) const {
    return lines[row];
}
