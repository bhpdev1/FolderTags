#include "tags.h"
#include "index.h"
#include "overlay.h"

namespace tags {

const TagInfo kTags[kCount] = {
    {"red", L"Rouge", 240, 82, 79},
    {"orange", L"Orange", 246, 155, 48},
    {"yellow", L"Jaune", 247, 203, 63},
    {"green", L"Vert", 98, 194, 85},
    {"blue", L"Bleu", 56, 138, 240},
    {"purple", L"Violet", 176, 99, 214},
    {"gray", L"Gris", 152, 152, 157},
};

static const wchar_t kStream[] = L":FolderTags.Tags";

std::wstring Normalize(std::wstring p) {
    while (p.size() > 3 && (p.back() == L'\\' || p.back() == L'/')) p.pop_back();
    return p;
}

unsigned Read(const std::wstring& path) {
    HANDLE h = CreateFileW((Normalize(path) + kStream).c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return 0;
    char buf[512];
    DWORD n = 0;
    ReadFile(h, buf, sizeof(buf), &n, nullptr);
    CloseHandle(h);

    std::string s(buf, n);
    unsigned mask = 0;
    size_t start = 0;
    while (start < s.size()) {
        size_t end = s.find_first_of(",\r\n", start);
        if (end == std::string::npos) end = s.size();
        std::string tok = s.substr(start, end - start);
        for (int i = 0; i < kCount; ++i)
            if (tok == kTags[i].key) mask |= 1u << i;
        start = end + 1;
    }
    return mask;
}

bool Write(const std::wstring& rawPath, unsigned mask) {
    const std::wstring path = Normalize(rawPath);

    // Keep the folder's timestamps untouched (tagging is metadata, like on macOS).
    HANDLE dir = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                             OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    FILETIME created{}, accessed{}, written{};
    const bool haveTimes =
        dir != INVALID_HANDLE_VALUE && GetFileTime(dir, &created, &accessed, &written);

    // Folders with a custom icon (desktop.ini) carry the ReadOnly attribute, which makes
    // NTFS refuse writes to their streams. Lift it for the duration of the write.
    const DWORD attrs = GetFileAttributesW(path.c_str());
    const bool readOnly = attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY);
    if (readOnly) SetFileAttributesW(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);

    const std::wstring stream = path + kStream;
    bool ok = false;
    if (mask == 0) {
        ok = DeleteFileW(stream.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND;
    } else {
        std::string data;
        for (int i = 0; i < kCount; ++i) {
            if (!(mask & (1u << i))) continue;
            if (!data.empty()) data += ',';
            data += kTags[i].key;
        }
        HANDLE h = CreateFileW(stream.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            DWORD w = 0;
            ok = WriteFile(h, data.data(), (DWORD)data.size(), &w, nullptr) && w == data.size();
            CloseHandle(h);
        }
    }

    if (readOnly) SetFileAttributesW(path.c_str(), attrs);
    if (haveTimes) SetFileTime(dir, nullptr, &accessed, &written);
    if (dir != INVALID_HANDLE_VALUE) CloseHandle(dir);
    return ok;
}

void NotifyChanged(const std::wstring& path) {
    OverlayInvalidateCache();
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, path.c_str(), nullptr);
}

bool IsDarkMode() {
    DWORD v = 1, size = sizeof(v);
    RegGetValueW(HKEY_CURRENT_USER,
                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &v, &size);
    return v == 0;
}

void Selection::Load() {
    masks.clear();
    for (auto& p : paths) masks.push_back(Read(p));
}

State Selection::StateOf(int tag) const {
    size_t count = 0;
    for (unsigned m : masks)
        if (m & (1u << tag)) ++count;
    if (count == 0) return State::None;
    return count == masks.size() ? State::All : State::Partial;
}

void Selection::Toggle(int tag) {
    const bool remove = StateOf(tag) == State::All;
    const unsigned bit = 1u << tag;
    for (size_t i = 0; i < paths.size(); ++i) {
        const unsigned next = remove ? (masks[i] & ~bit) : (masks[i] | bit);
        if (next != masks[i] && Write(paths[i], next)) {
            index::Update(paths[i], masks[i], next);
            masks[i] = next;
            NotifyChanged(paths[i]);
        }
    }
}

}  // namespace tags
