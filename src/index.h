#pragma once
#include "common.h"

// "Tags" entry in the Explorer navigation pane, like the Tags section of the Finder sidebar.
// Backed by a real folder (%LOCALAPPDATA%\FolderTags\Tags) holding one sub-folder per color
// with shortcuts to every tagged folder.
namespace index {

std::wstring DataDir();  // %LOCALAPPDATA%\FolderTags
std::wstring IconPath(const wchar_t* name);
std::wstring OverlayIconPath(int tag);
// Hidden folder pre-tagged with `tag`; querying its icon makes a process load that overlay image.
std::wstring WarmupDir(int tag);

void Update(const std::wstring& path, unsigned oldMask, unsigned newMask);

// Creates icons, the Tags folder tree and registers the navigation-pane entry (HKCU).
HRESULT Setup();
void Teardown();

}  // namespace index
