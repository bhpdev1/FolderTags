// "Tags…" window, launched out-of-process through rundll32 so Explorer never hosts our UI.
// rundll32 "FolderTags.dll",ShowTags <x> <y> <list-file>
#include "draw.h"
#include "i18n.h"
#include <dwmapi.h>
#include <windowsx.h>

using namespace Gdiplus;

namespace {

struct TagsWindow {
    tags::Selection sel;
    UINT dpi = 96;
    bool dark = false;
    int hover = -1;

    float k() const { return dpi / 96.0f; }
    int S(float v) const { return (int)(v * k() + 0.5f); }
    int Pad() const { return S(6); }
    int HeaderH() const { return S(30); }
    int RowH() const { return S(28); }
    int Width() const { return S(220); }
    int Height() const { return Pad() * 2 + HeaderH() + RowH() * tags::kCount; }

    int RowAt(int y) const {
        const int top = Pad() + HeaderH();
        if (y < top) return -1;
        const int r = (y - top) / RowH();
        return r < tags::kCount ? r : -1;
    }

    void Paint(HWND hwnd, HDC hdc) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        const int w = rc.right, h = rc.bottom;
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(mem, bmp);

        const Color bg = dark ? Color(255, 44, 44, 46) : Color(255, 246, 246, 246);
        const Color hl = dark ? Color(255, 66, 66, 70) : Color(255, 226, 226, 230);
        const COLORREF text = dark ? RGB(240, 240, 240) : RGB(28, 28, 30);
        const COLORREF dim = dark ? RGB(160, 160, 165) : RGB(110, 110, 115);

        Graphics g(mem);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        g.Clear(bg);

        HFONT font = CreateFontW(-MulDiv(9, dpi, 72), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                                 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HFONT bold = CreateFontW(-MulDiv(9, dpi, 72), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
                                 DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        SetBkMode(mem, TRANSPARENT);

        // Header
        HGDIOBJ oldFont = SelectObject(mem, bold);
        SetTextColor(mem, dim);
        RECT hr = {Pad() + S(10), Pad(), w - Pad(), Pad() + HeaderH()};
        DrawTextW(mem, sel.paths.size() > 1 ? i18n::T(L"Tags (selection)", L"Tags (s\u00e9lection)") : L"Tags",
                  -1, &hr, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

        SelectObject(mem, font);
        for (int i = 0; i < tags::kCount; ++i) {
            const int top = Pad() + HeaderH() + RowH() * i;
            if (i == hover) {
                GraphicsPath path;
                const float x = (float)Pad(), y = (float)top, rw = (float)(w - 2 * Pad()),
                            rh = (float)RowH(), r = (float)S(5);
                path.AddArc(x, y, r * 2, r * 2, 180, 90);
                path.AddArc(x + rw - r * 2, y, r * 2, r * 2, 270, 90);
                path.AddArc(x + rw - r * 2, y + rh - r * 2, r * 2, r * 2, 0, 90);
                path.AddArc(x, y + rh - r * 2, r * 2, r * 2, 90, 90);
                path.CloseFigure();
                SolidBrush b(hl);
                g.FillPath(&b, &path);
            }
            const tags::State st = sel.StateOf(i);
            draw::PaintDot(g, (float)(Pad() + S(18)), top + RowH() / 2.0f, (float)S(6), i,
                           tags::State::None, false, dark);

            SetTextColor(mem, text);
            RECT tr = {Pad() + S(34), top, w - Pad() - S(30), top + RowH()};
            DrawTextW(mem, tags::Label(i), -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

            if (st != tags::State::None) {
                Pen pen(dark ? Color(255, 240, 240, 240) : Color(255, 28, 28, 30),
                        1.6f * k());
                pen.SetStartCap(LineCapRound);
                pen.SetEndCap(LineCapRound);
                pen.SetLineJoin(LineJoinRound);
                const float cx = (float)(w - Pad() - S(18)), cy = top + RowH() / 2.0f,
                            s = 4.5f * k();
                if (st == tags::State::All) {
                    PointF pts[3] = {{cx - s, cy}, {cx - s * 0.3f, cy + s * 0.75f},
                                     {cx + s, cy - s * 0.7f}};
                    g.DrawLines(&pen, pts, 3);
                } else {
                    g.DrawLine(&pen, cx - s * 0.8f, cy, cx + s * 0.8f, cy);
                }
            }
        }

        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        SelectObject(mem, oldFont);
        DeleteObject(font);
        DeleteObject(bold);
        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);
    }
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto self = reinterpret_cast<TagsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_NCCREATE:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          (LONG_PTR) reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        self->Paint(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEMOVE: {
        const int row = self->RowAt(GET_Y_LPARAM(lp));
        if (row != self->hover) {
            self->hover = row;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        self->hover = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP: {
        const int row = self->RowAt(GET_Y_LPARAM(lp));
        if (row >= 0) {
            self->sel.Toggle(row);
            SetTimer(hwnd, 1, 400, nullptr);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_TIMER:
        KillTimer(hwnd, 1);
        for (auto& p : self->sel.paths) tags::NotifyChanged(p);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE || wp == VK_RETURN) DestroyWindow(hwnd);
        if (wp >= '1' && wp < '1' + tags::kCount) {
            self->sel.Toggle((int)(wp - '1'));
            SetTimer(hwnd, 1, 400, nullptr);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wp) == WA_INACTIVE) DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        for (auto& p : self->sel.paths) tags::NotifyChanged(p);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

std::vector<std::wstring> ReadList(const wchar_t* file) {
    std::vector<std::wstring> out;
    HANDLE h = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return out;
    LARGE_INTEGER size{};
    GetFileSizeEx(h, &size);
    std::wstring data((size_t)size.QuadPart / sizeof(wchar_t), L'\0');
    DWORD n = 0;
    ReadFile(h, data.data(), (DWORD)size.QuadPart, &n, nullptr);
    CloseHandle(h);
    DeleteFileW(file);

    size_t start = 0;
    while (start < data.size()) {
        size_t end = data.find(L'\n', start);
        if (end == std::wstring::npos) end = data.size();
        if (end > start) out.push_back(data.substr(start, end - start));
        start = end + 1;
    }
    return out;
}

}  // namespace

extern "C" void CALLBACK ShowTagsW(HWND, HINSTANCE, LPWSTR cmdLine, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    int x = 0, y = 0;
    wchar_t file[MAX_PATH] = {};
    if (swscanf_s(cmdLine, L"%d %d %259[^\n]", &x, &y, file, (unsigned)ARRAYSIZE(file)) != 3)
        return;

    TagsWindow tw;
    tw.sel.paths = ReadList(file);
    if (tw.sel.paths.empty()) return;
    tw.sel.Load();
    tw.dark = tags::IsDarkMode();

    POINT pt = {x, y};
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    UINT dx = 96, dy = 96;
    using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
    if (HMODULE shcore = LoadLibraryW(L"shcore.dll")) {
        if (auto fn = (GetDpiForMonitorFn)GetProcAddress(shcore, "GetDpiForMonitor"))
            fn(mon, 0, &dx, &dy);
    }
    tw.dpi = dx;

    draw::EnsureGdiplus();

    WNDCLASSEXW wc{sizeof(wc)};
    wc.style = CS_DROPSHADOW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"FolderTags.TagsWindow";
    RegisterClassExW(&wc);

    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(mon, &mi);
    const int w = tw.Width(), h = tw.Height();
    x = std::min<int>(std::max<int>(x, mi.rcWork.left), mi.rcWork.right - w);
    y = std::min<int>(std::max<int>(y, mi.rcWork.top), mi.rcWork.bottom - h);

    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, wc.lpszClassName, L"Tags",
                                WS_POPUP, x, y, w, h, nullptr, nullptr, g_hInst, &tw);
    if (!hwnd) return;

    const DWORD round = 2;  // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &round, sizeof(round));
    const BOOL darkAttr = tw.dark;
    DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &darkAttr, sizeof(darkAttr));

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
}
