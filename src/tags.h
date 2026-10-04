#pragma once
#include "common.h"

namespace tags {

constexpr int kCount = 7;

struct TagInfo {
    const char* key;       // stored on disk
    const wchar_t* label;  // shown in UI
    BYTE r, g, b;
};
extern const TagInfo kTags[kCount];

enum class State { None, Partial, All };

// Tags are stored in an NTFS alternate data stream on the folder itself
// ("<folder>:FolderTags.Tags"), like macOS stores them in an xattr: they follow
// the folder when it is renamed or moved on the same volume.
unsigned Read(const std::wstring& path);
bool Write(const std::wstring& path, unsigned mask);
std::wstring Normalize(std::wstring path);
void NotifyChanged(const std::wstring& path);
bool IsDarkMode();

struct Selection {
    std::vector<std::wstring> paths;
    std::vector<unsigned> masks;

    void Load();
    State StateOf(int tag) const;
    // macOS semantics: if every item has the tag, remove it; otherwise add it to all.
    void Toggle(int tag);
};

}  // namespace tags
