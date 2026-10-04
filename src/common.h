#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <cmath>
#include <string>
#include <vector>
#include <strsafe.h>

extern HINSTANCE g_hInst;
extern long g_dllRefs;

// {B6E4CD56-987A-4C8F-8729-7FD3D4D8EBD9}
inline constexpr CLSID CLSID_FolderTagsMenu = {
    0xb6e4cd56, 0x987a, 0x4c8f, {0x87, 0x29, 0x7f, 0xd3, 0xd4, 0xd8, 0xeb, 0xd9}};
inline constexpr wchar_t kClsidStr[] = L"{B6E4CD56-987A-4C8F-8729-7FD3D4D8EBD9}";
