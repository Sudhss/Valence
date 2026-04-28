#include "text_buffer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

TextBuffer::TextBuffer() {
    lines_.push_back("");
}

void TextBuffer::insertChar(int row, int col, char ch) {
    if (row < 0 || row >= (int)lines_.size()) return;
    col = std::clamp(col, 0, (int)lines_[row].size());
    lines_[row].insert(col, 1, ch);
}

void TextBuffer::deleteChar(int row, int col) {
    if (row < 0 || row >= (int)lines_.size()) return;
    if (col <= 0) {
        mergeLines(row);
        return;
    }
    if (col <= (int)lines_[row].size()) {
        lines_[row].erase(col - 1, 1);
    }
}

void TextBuffer::splitLine(int row, int col) {
    if (row < 0 || row >= (int)lines_.size()) return;
    col = std::clamp(col, 0, (int)lines_[row].size());
    std::string tail = lines_[row].substr(col);
    lines_[row] = lines_[row].substr(0, col);
    lines_.insert(lines_.begin() + row + 1, tail);
}

void TextBuffer::mergeLines(int row) {
    if (row <= 0 || row >= (int)lines_.size()) return;
    lines_[row - 1] += lines_[row];
    lines_.erase(lines_.begin() + row);
}

std::string TextBuffer::getText(Position start, Position end) const {
    if (start > end) std::swap(start, end);
    if (start.row == end.row) {
        int s = std::clamp(start.col, 0, (int)lines_[start.row].size());
        int e = std::clamp(end.col, 0, (int)lines_[start.row].size());
        return lines_[start.row].substr(s, e - s);
    }

    std::string result;
    // First line
    result += lines_[start.row].substr(start.col);
    result += '\n';
    // Middle lines
    for (int r = start.row + 1; r < end.row; r++) {
        result += lines_[r];
        result += '\n';
    }
    // Last line
    int e = std::clamp(end.col, 0, (int)lines_[end.row].size());
    result += lines_[end.row].substr(0, e);
    return result;
}

void TextBuffer::deleteRange(Position start, Position end) {
    if (start > end) std::swap(start, end);
    if (start.row == end.row) {
        int s = std::clamp(start.col, 0, (int)lines_[start.row].size());
        int e = std::clamp(end.col, 0, (int)lines_[start.row].size());
        lines_[start.row].erase(s, e - s);
        return;
    }

    // Keep prefix of first line + suffix of last line
    std::string prefix = lines_[start.row].substr(0, start.col);
    int ec = std::clamp(end.col, 0, (int)lines_[end.row].size());
    std::string suffix = lines_[end.row].substr(ec);

    // Erase lines from start.row to end.row (inclusive)
    lines_.erase(lines_.begin() + start.row, lines_.begin() + end.row + 1);
    lines_.insert(lines_.begin() + start.row, prefix + suffix);
}

Position TextBuffer::insertText(int row, int col, const std::string& text) {
    if (text.empty()) return {row, col};

    // Split the text into lines
    std::vector<std::string> newLines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') line.pop_back();
        newLines.push_back(line);
    }
    // If text ends with newline, add empty line
    if (!text.empty() && text.back() == '\n') {
        newLines.push_back("");
    }
    if (newLines.empty()) return {row, col};

    col = std::clamp(col, 0, (int)lines_[row].size());
    std::string prefix = lines_[row].substr(0, col);
    std::string suffix = lines_[row].substr(col);

    if (newLines.size() == 1) {
        lines_[row] = prefix + newLines[0] + suffix;
        return {row, (int)(prefix.size() + newLines[0].size())};
    }

    // Multi-line insert
    lines_[row] = prefix + newLines[0];
    for (int i = 1; i < (int)newLines.size() - 1; i++) {
        lines_.insert(lines_.begin() + row + i, newLines[i]);
    }
    int lastIdx = row + (int)newLines.size() - 1;
    lines_.insert(lines_.begin() + lastIdx, newLines.back() + suffix);
    return {lastIdx, (int)newLines.back().size()};
}

int TextBuffer::findWordBoundaryLeft(int row, int col) const {
    if (col <= 0) return 0;
    const std::string& l = lines_[row];
    int i = col - 1;
    // Skip whitespace
    while (i > 0 && std::isspace((unsigned char)l[i])) i--;
    // Skip word chars
    if (i >= 0 && std::isalnum((unsigned char)l[i])) {
        while (i > 0 && std::isalnum((unsigned char)l[i - 1])) i--;
    } else if (i >= 0) {
        // Skip punctuation
        while (i > 0 && !std::isalnum((unsigned char)l[i - 1]) && !std::isspace((unsigned char)l[i - 1])) i--;
    }
    return i;
}

int TextBuffer::findWordBoundaryRight(int row, int col) const {
    const std::string& l = lines_[row];
    int len = (int)l.size();
    if (col >= len) return len;
    int i = col;
    // Skip current word chars
    if (std::isalnum((unsigned char)l[i])) {
        while (i < len && std::isalnum((unsigned char)l[i])) i++;
    } else if (!std::isspace((unsigned char)l[i])) {
        while (i < len && !std::isalnum((unsigned char)l[i]) && !std::isspace((unsigned char)l[i])) i++;
    }
    // Skip whitespace
    while (i < len && std::isspace((unsigned char)l[i])) i++;
    return i;
}

std::string TextBuffer::getLeadingWhitespace(int row) const {
    if (row < 0 || row >= (int)lines_.size()) return "";
    const std::string& l = lines_[row];
    size_t i = 0;
    while (i < l.size() && (l[i] == ' ' || l[i] == '\t')) i++;
    return l.substr(0, i);
}

bool TextBuffer::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    lines_.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines_.push_back(line);
    }
    if (lines_.empty()) lines_.push_back("");
    return true;
}

bool TextBuffer::saveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    for (int i = 0; i < (int)lines_.size(); i++) {
        file << lines_[i];
        if (i < (int)lines_.size() - 1) file << '\n';
    }
    return true;
}

int TextBuffer::lineCount() const { return (int)lines_.size(); }
int TextBuffer::lineLength(int row) const {
    if (row < 0 || row >= (int)lines_.size()) return 0;
    return (int)lines_[row].size();
}
const std::string& TextBuffer::line(int row) const { return lines_[row]; }

void TextBuffer::clear() {
    lines_.clear();
    lines_.push_back("");
}
