#pragma once
#include "common.h"

namespace tags {

constexpr int kCount = 7;

struct TagInfo {
    const char* key;         // stored on disk (NTFS stream content)
    const wchar_t* id;       // same key, used for file / folder names
    const wchar_t* labelEn;  // shown in UI
    const wchar_t* labelFr;  // shown in UI on French Windows (also the folder names of v1.0.0)
    BYTE r, g, b;
};
extern const TagInfo kTags[kCount];

// Localized display name of a tag.
const wchar_t* Label(int tag);

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
