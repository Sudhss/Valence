#pragma once
#include <utility>

struct Position {
    int row = 0;
    int col = 0;

    bool operator==(const Position& o) const { return row == o.row && col == o.col; }
    bool operator!=(const Position& o) const { return !(*this == o); }
    bool operator<(const Position& o) const {
        return row < o.row || (row == o.row && col < o.col);
    }
    bool operator>(const Position& o) const { return o < *this; }
    bool operator<=(const Position& o) const { return !(o < *this); }
    bool operator>=(const Position& o) const { return !(*this < o); }
};

struct Selection {
    Position anchor;
    bool active = false;

    void start(Position pos) { anchor = pos; active = true; }
    void clear() { active = false; }

    bool hasSelection(const Position& cursor) const {
        return active && anchor != cursor;
    }

    std::pair<Position, Position> normalized(const Position& cursor) const {
        if (anchor < cursor) return {anchor, cursor};
        return {cursor, anchor};
    }
};
