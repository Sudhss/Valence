#pragma once
#include <vector>
#include <string>
#include <chrono>
#include "selection.h"

struct EditAction {
    enum Type { Insert, Delete };
    Type type;
    Position pos;
    std::string text;
    std::chrono::steady_clock::time_point timestamp;
};

class UndoManager {
public:
    UndoManager();

    void recordInsert(Position pos, const std::string& text);
    void recordDelete(Position pos, const std::string& text);
    void forceNewGroup();

    // Forces every edit recorded between the two calls into a single undo step,
    // regardless of shape or timing. Needed because the automatic grouping only
    // ever merges single-character edits, so a compound operation such as
    // indenting a block would otherwise take one Ctrl+Z per line.
    void beginCompound();
    void endCompound();

    struct UndoResult {
        bool valid = false;
        std::vector<EditAction> actions;
    };

    UndoResult undo();
    UndoResult redo();

    bool canUndo() const;
    bool canRedo() const;
    void clear();

private:
    using ActionGroup = std::vector<EditAction>;
    std::vector<ActionGroup> undoStack_;
    std::vector<ActionGroup> redoStack_;
    bool forceNext_ = false;
    int compoundDepth_ = 0;
    bool compoundStarted_ = false;

    bool shouldGroup(const EditAction& action) const;
    void push(const EditAction& action);
    static constexpr int GROUP_TIMEOUT_MS = 400;
    // Undo history is otherwise unbounded: a long editing session grows it
    // forever. Old groups are dropped from the bottom.
    static constexpr size_t MAX_GROUPS = 4000;
    void trim();
};
