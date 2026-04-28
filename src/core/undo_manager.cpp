#include "undo_manager.h"

UndoManager::UndoManager() {}

bool UndoManager::shouldGroup(const EditAction& action) const {
    if (forceNext_) return false;
    if (undoStack_.empty() || undoStack_.back().empty()) return false;

    const auto& lastGroup = undoStack_.back();
    const auto& last = lastGroup.back();

    // Only group single-char inserts with single-char inserts (and same for deletes)
    if (last.type != action.type) return false;
    if (last.text.size() != 1 || action.text.size() != 1) return false;

    // Don't group across whitespace boundaries
    bool lastIsSpace = (last.text[0] == ' ' || last.text[0] == '\t');
    bool curIsSpace = (action.text[0] == ' ' || action.text[0] == '\t');
    if (lastIsSpace != curIsSpace) return false;

    // Check time
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        action.timestamp - last.timestamp).count();
    if (elapsed > GROUP_TIMEOUT_MS) return false;

    // Check adjacency
    if (action.type == EditAction::Insert) {
        // Cursor should be right after the last insert
        if (action.pos.row != last.pos.row) return false;
        if (action.pos.col != last.pos.col + (int)last.text.size()) return false;
    } else {
        // For deletes, cursor should be at or near the last delete
        if (action.pos.row != last.pos.row) return false;
    }

    return true;
}

void UndoManager::recordInsert(Position pos, const std::string& text) {
    EditAction action{EditAction::Insert, pos, text,
                      std::chrono::steady_clock::now()};

    redoStack_.clear();

    if (shouldGroup(action)) {
        undoStack_.back().push_back(action);
    } else {
        undoStack_.push_back({action});
    }
    forceNext_ = false;
}

void UndoManager::recordDelete(Position pos, const std::string& text) {
    EditAction action{EditAction::Delete, pos, text,
                      std::chrono::steady_clock::now()};

    redoStack_.clear();

    if (shouldGroup(action)) {
        undoStack_.back().push_back(action);
    } else {
        undoStack_.push_back({action});
    }
    forceNext_ = false;
}

void UndoManager::forceNewGroup() {
    forceNext_ = true;
}

UndoManager::UndoResult UndoManager::undo() {
    if (undoStack_.empty()) return {false, {}};

    auto group = undoStack_.back();
    undoStack_.pop_back();
    redoStack_.push_back(group);

    // Reverse the actions for undo — process in reverse order
    UndoResult result;
    result.valid = true;
    result.actions.assign(group.rbegin(), group.rend());
    return result;
}

UndoManager::UndoResult UndoManager::redo() {
    if (redoStack_.empty()) return {false, {}};

    auto group = redoStack_.back();
    redoStack_.pop_back();
    undoStack_.push_back(group);

    UndoResult result;
    result.valid = true;
    result.actions = group; // forward order
    return result;
}

bool UndoManager::canUndo() const { return !undoStack_.empty(); }
bool UndoManager::canRedo() const { return !redoStack_.empty(); }

void UndoManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
    forceNext_ = false;
}
