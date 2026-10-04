#include "context_menu.h"
#include "draw.h"

using namespace Gdiplus;

// Why no MFT_OWNERDRAW: a single owner-drawn item makes Windows 11 drop the modern
// (rounded, dark) menu renderer for the *whole* menu and fall back to the legacy light one.
// Instead the row is a regular blank item, and the dots are painted on top of it while the
// menu is open (thread timer + GetDC on the menu window).

namespace {

constexpr ULONG_PTR kRowMagic = 0x46544147;  // 'FTAG'
constexpr UINT kCmdRow = 0;
constexpr UINT kCmdEdit = 1;
#ifndef MN_GETHMENU
#define MN_GETHMENU 0x01E1
#endif

ContextMenu* g_timerOwner = nullptr;

// Row geometry. X positions are relative to the menu window's left edge so the dots line up
// with the text column of the Windows 11 menu (measured: ~37.6 logical px).
struct Geo {
    float k;
    float left, radius, step;
};

Geo GeoFor(UINT dpi) {
    const float k = dpi / 96.0f;
    return {k, 37.5f * k, 6.0f * k, 21.0f * k};
}

float DotCenterX(const Geo& g, int i) { return g.left + g.radius + g.step * i; }

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD) {
    if (g_timerOwner) g_timerOwner->OnTimer();
}

struct FindCtx {
    HWND wnd = nullptr;
    HMENU menu = nullptr;
    int pos = -1;
};

BOOL CALLBACK FindMenuWindow(HWND w, LPARAM lp) {
    wchar_t cls[16] = {};
    if (!IsWindowVisible(w) || !GetClassNameW(w, cls, ARRAYSIZE(cls)) || lstrcmpW(cls, L"#32768"))
        return TRUE;
    auto menu = reinterpret_cast<HMENU>(SendMessageW(w, MN_GETHMENU, 0, 0));
    if (!menu) return TRUE;
    const int n = GetMenuItemCount(menu);
    for (int i = 0; i < n; ++i) {
        MENUITEMINFOW mii{sizeof(mii)};
        mii.fMask = MIIM_DATA;
        if (GetMenuItemInfoW(menu, i, TRUE, &mii) && mii.dwItemData == kRowMagic) {
            auto ctx = reinterpret_cast<FindCtx*>(lp);
            ctx->wnd = w;
            ctx->menu = menu;
            ctx->pos = i;
            return FALSE;
        }
    }
    return TRUE;
}

}  // namespace

ContextMenu::ContextMenu() { InterlockedIncrement(&g_dllRefs); }

ContextMenu::~ContextMenu() {
    StopTimer();
    InterlockedDecrement(&g_dllRefs);
}

IFACEMETHODIMP ContextMenu::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(ContextMenu, IShellExtInit),
        QITABENT(ContextMenu, IContextMenu3),
        QITABENTMULTI(ContextMenu, IContextMenu2, IContextMenu3),
        QITABENTMULTI(ContextMenu, IContextMenu, IContextMenu3),
        {nullptr, 0},
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ContextMenu::AddRef() { return InterlockedIncrement(&refs_); }

IFACEMETHODIMP_(ULONG) ContextMenu::Release() {
    const long r = InterlockedDecrement(&refs_);
    if (r == 0) delete this;
    return r;
}

IFACEMETHODIMP ContextMenu::Initialize(PCIDLIST_ABSOLUTE, IDataObject* data, HKEY) {
    sel_.paths.clear();
    if (!data) return E_INVALIDARG;

    FORMATETC fe = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stg{};
    if (FAILED(data->GetData(&fe, &stg))) return E_FAIL;

    if (auto drop = static_cast<HDROP>(GlobalLock(stg.hGlobal))) {
        const UINT n = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < n; ++i) {
            wchar_t path[MAX_PATH * 4];
            if (!DragQueryFileW(drop, i, path, ARRAYSIZE(path))) continue;
            if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
                sel_.paths.push_back(tags::Normalize(path));
        }
        GlobalUnlock(stg.hGlobal);
    }
    ReleaseStgMedium(&stg);
    return sel_.paths.empty() ? E_FAIL : S_OK;
}

IFACEMETHODIMP ContextMenu::QueryContextMenu(HMENU menu, UINT index, UINT first, UINT,
                                             UINT flags) {
    if (flags & CMF_DEFAULTONLY) return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

    sel_.Load();
    hover_ = -1;
    seen_ = false;

    InsertMenuW(menu, index++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

    // Blank, regular item reserving the space for the dots (em spaces give it a width).
    MENUITEMINFOW row{sizeof(row)};
    row.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA | MIIM_STRING;
    row.fType = MFT_STRING;
    row.wID = first + kCmdRow;
    row.dwItemData = kRowMagic;
    wchar_t blank[] = L"\u2003\u2003\u2003\u2003\u2003\u2003\u2003\u2003\u2003\u2003";
    row.dwTypeData = blank;
    InsertMenuItemW(menu, index++, TRUE, &row);

    InsertMenuW(menu, index++, MF_BYPOSITION | MF_STRING, first + kCmdEdit, L"Tags\u2026");
    InsertMenuW(menu, index++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

    // Paint/hover loop; runs inside the menu's modal loop (thread timers are dispatched).
    if (g_timerOwner && g_timerOwner != this) g_timerOwner->StopTimer();
    if (!timer_) {
        g_timerOwner = this;
        AddRef();  // released in StopTimer
        timer_ = SetTimer(nullptr, 0, 15, TimerProc);
        timerStart_ = GetTickCount64();
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, kCmdEdit + 1);
}

IFACEMETHODIMP ContextMenu::GetCommandString(UINT_PTR cmd, UINT type, UINT*, CHAR* name,
                                             UINT cch) {
    const wchar_t* verb = cmd == kCmdRow ? L"foldertags.colors"
                          : cmd == kCmdEdit ? L"foldertags.edit"
                                            : nullptr;
    if (!verb) return E_INVALIDARG;
    switch (type) {
    case GCS_VERBW: return StringCchCopyW(reinterpret_cast<LPWSTR>(name), cch, verb);
    case GCS_HELPTEXTW:
        return StringCchCopyW(reinterpret_cast<LPWSTR>(name), cch,
                              cmd == kCmdRow ? L"Ajouter ou retirer un tag de couleur"
                                             : L"Modifier les tags");
    case GCS_VALIDATEW: return S_OK;
    default: return E_NOTIMPL;
    }
}

IFACEMETHODIMP ContextMenu::InvokeCommand(CMINVOKECOMMANDINFO* info) {
    UINT cmd;
    if (IS_INTRESOURCE(info->lpVerb)) {
        cmd = LOWORD(info->lpVerb);
    } else if (!lstrcmpiA(info->lpVerb, "foldertags.colors")) {
        cmd = kCmdRow;
    } else if (!lstrcmpiA(info->lpVerb, "foldertags.edit")) {
        cmd = kCmdEdit;
    } else {
        return E_INVALIDARG;
    }

    if (cmd == kCmdRow) {
        int dot = hover_;
        if (dot < 0 && lastWnd_) {
            POINT pt;
            GetCursorPos(&pt);
            dot = HitTest(lastWnd_, pt, lastRc_);
        }
        StopTimer();
        if (dot >= 0)
            sel_.Toggle(dot);
        else
            OpenTagsWindow();  // Keyboard activation: fall back to the tag editor.
        return S_OK;
    }
    if (cmd == kCmdEdit) {
        StopTimer();
        OpenTagsWindow();
        return S_OK;
    }
    return E_INVALIDARG;
}

IFACEMETHODIMP ContextMenu::HandleMenuMsg(UINT, WPARAM, LPARAM) { return S_OK; }

IFACEMETHODIMP ContextMenu::HandleMenuMsg2(UINT, WPARAM, LPARAM, LRESULT* result) {
    if (result) *result = FALSE;
    return S_OK;
}

bool ContextMenu::LocateRow(HWND* wnd, RECT* rc) const {
    FindCtx ctx;
    EnumThreadWindows(GetCurrentThreadId(), FindMenuWindow, reinterpret_cast<LPARAM>(&ctx));
    if (!ctx.wnd) return false;
    if (!GetMenuItemRect(ctx.wnd, ctx.menu, ctx.pos, rc)) return false;
    *wnd = ctx.wnd;
    return true;
}

int ContextMenu::HitTest(HWND wnd, POINT pt, const RECT& rc) const {
    if (!PtInRect(&rc, pt)) return -1;
    RECT wr{};
    if (!GetWindowRect(wnd, &wr)) return -1;
    const Geo g = GeoFor(GetDpiForWindow(wnd));
    const float x = (float)(pt.x - wr.left);
    for (int i = 0; i < tags::kCount; ++i)
        if (std::abs(x - DotCenterX(g, i)) <= g.step / 2.0f) return i;
    return -1;
}

void ContextMenu::OnTimer() {
    HWND wnd;
    RECT rc;
    if (!LocateRow(&wnd, &rc)) {
        // Not shown yet (give it time) or menu closed.
        if (seen_ || GetTickCount64() - timerStart_ > 5000) StopTimer();
        return;
    }
    seen_ = true;
    lastWnd_ = wnd;
    lastRc_ = rc;
    POINT pt;
    GetCursorPos(&pt);
    hover_ = HitTest(wnd, pt, rc);
    PaintRow(wnd, rc);
}

void ContextMenu::PaintRow(HWND wnd, const RECT& rcScreen) {
    RECT rc = rcScreen;
    MapWindowPoints(nullptr, wnd, reinterpret_cast<POINT*>(&rc), 2);
    const int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    HDC hdc = GetDC(wnd);
    if (!hdc) return;

    // Sample the menu background next to the row (outside the inset hover highlight).
    bg_ = GetPixel(hdc, 1, rc.top + h / 2);
    if (bg_ == CLR_INVALID) bg_ = tags::IsDarkMode() ? RGB(44, 44, 44) : RGB(249, 249, 249);
    const bool dark = (GetRValue(bg_) + GetGValue(bg_) + GetBValue(bg_)) < 384;

    RECT wr{};
    GetWindowRect(wnd, &wr);
    POINT origin = {0, 0};
    ClientToScreen(wnd, &origin);
    const float clientOffset = (float)(origin.x - wr.left);  // border width

    draw::EnsureGdiplus();
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ old = SelectObject(mem, bmp);
    {
        Graphics g(mem);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        g.Clear(Color(255, GetRValue(bg_), GetGValue(bg_), GetBValue(bg_)));

        const Geo geo = GeoFor(GetDpiForWindow(wnd));
        const float dx = -(float)rc.left - clientOffset;  // window-relative -> bitmap
        for (int i = 0; i < tags::kCount; ++i)
            draw::PaintDot(g, DotCenterX(geo, i) + dx, h / 2.0f, geo.radius, i, sel_.StateOf(i),
                           i == hover_, dark);
    }
    BitBlt(hdc, rc.left, rc.top, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(wnd, hdc);
}

void ContextMenu::StopTimer() {
    if (!timer_) return;
    KillTimer(nullptr, timer_);
    timer_ = 0;
    if (g_timerOwner == this) g_timerOwner = nullptr;
    Release();
}

void ContextMenu::OpenTagsWindow() {
    wchar_t tmpDir[MAX_PATH], tmpFile[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpDir);
    if (!GetTempFileNameW(tmpDir, L"ftg", 0, tmpFile)) return;

    std::wstring list;
    for (auto& p : sel_.paths) list += p + L"\n";
    HANDLE h = CreateFileW(tmpFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD w;
    WriteFile(h, list.data(), (DWORD)(list.size() * sizeof(wchar_t)), &w, nullptr);
    CloseHandle(h);

    wchar_t dll[MAX_PATH], sys[MAX_PATH];
    GetModuleFileNameW(g_hInst, dll, MAX_PATH);
    GetSystemDirectoryW(sys, MAX_PATH);
    POINT pt;
    GetCursorPos(&pt);

    std::wstring exe = std::wstring(sys) + L"\\rundll32.exe";
    std::wstring cmd = L"\"" + exe + L"\" \"" + dll + L"\",ShowTags " + std::to_wstring(pt.x) +
                       L" " + std::to_wstring(pt.y) + L" " + tmpFile;

    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(exe.c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                       &si, &pi)) {
        AllowSetForegroundWindow(pi.dwProcessId);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}
