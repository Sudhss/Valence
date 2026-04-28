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

    bool shouldGroup(const EditAction& action) const;
    static constexpr int GROUP_TIMEOUT_MS = 400;
};
