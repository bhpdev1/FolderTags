#pragma once
#include "common.h"
#include "tags.h"

class ContextMenu final : public IShellExtInit, public IContextMenu3 {
public:
    ContextMenu();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IShellExtInit
    IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE folder, IDataObject* data, HKEY progId) override;

    // IContextMenu
    IFACEMETHODIMP QueryContextMenu(HMENU menu, UINT index, UINT first, UINT last,
                                    UINT flags) override;
    IFACEMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO* info) override;
    IFACEMETHODIMP GetCommandString(UINT_PTR cmd, UINT type, UINT* reserved, CHAR* name,
                                    UINT cch) override;
    // IContextMenu2 / 3
    IFACEMETHODIMP HandleMenuMsg(UINT msg, WPARAM wp, LPARAM lp) override;
    IFACEMETHODIMP HandleMenuMsg2(UINT msg, WPARAM wp, LPARAM lp, LRESULT* result) override;

    // Driven by a thread timer while the menu is open: tracks hover and paints the dots.
    void OnTimer();

private:
    ~ContextMenu();
    bool LocateRow(HWND* wnd, RECT* rc) const;
    int HitTest(HWND wnd, POINT screenPt, const RECT& itemRc) const;
    void PaintRow(HWND wnd, const RECT& itemRcScreen);
    void OpenTagsWindow();
    void StopTimer();

    long refs_ = 1;
    tags::Selection sel_;

    int hover_ = -1;
    HWND lastWnd_ = nullptr;
    RECT lastRc_{};
    COLORREF bg_ = CLR_INVALID;
    UINT_PTR timer_ = 0;
    ULONGLONG timerStart_ = 0;
    bool seen_ = false;
};
