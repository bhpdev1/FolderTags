#include "common.h"
#include "context_menu.h"
#include "index.h"
#include "overlay.h"
#include "tags.h"

HINSTANCE g_hInst = nullptr;
long g_dllRefs = 0;

namespace {

class ClassFactory final : public IClassFactory {
public:
    explicit ClassFactory(int overlayTag) : overlayTag_(overlayTag) {}

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        static const QITAB qit[] = {QITABENT(ClassFactory, IClassFactory), {nullptr, 0}};
        return QISearch(this, qit, riid, ppv);
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refs_); }
    IFACEMETHODIMP_(ULONG) Release() override {
        const long r = InterlockedDecrement(&refs_);
        if (r == 0) delete this;
        return r;
    }
    IFACEMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        *ppv = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION;
        IUnknown* obj = overlayTag_ >= 0
                            ? CreateOverlay(overlayTag_)
                            : static_cast<IShellExtInit*>(new (std::nothrow) ContextMenu());
        if (!obj) return E_OUTOFMEMORY;
        const HRESULT hr = obj->QueryInterface(riid, ppv);
        obj->Release();
        return hr;
    }
    IFACEMETHODIMP LockServer(BOOL lock) override {
        lock ? InterlockedIncrement(&g_dllRefs) : InterlockedDecrement(&g_dllRefs);
        return S_OK;
    }

private:
    long refs_ = 1;
    int overlayTag_;  // -1 = context menu
};

// Registered on Directory and * (files) rather than AllFilesystemObjects: Explorer places
// AllFilesystemObjects handlers much lower in the menu.
const wchar_t* const kHandlerKeys[] = {
    L"Software\\\\Classes\\\\Directory\\\\shellex\\\\ContextMenuHandlers\\\\ FolderTags",
    L"Software\\\\Classes\\\\*\\\\shellex\\\\ContextMenuHandlers\\\\ FolderTags",
};
// Used by an earlier build; removed on (un)registration.
const wchar_t kLegacyHandlerKey[] =
    L"Software\\\\Classes\\\\AllFilesystemObjects\\\\shellex\\\\ContextMenuHandlers\\\\ FolderTags";

HRESULT SetValue(const std::wstring& key, const wchar_t* name, const std::wstring& value) {
    return HRESULT_FROM_WIN32(RegSetKeyValueW(HKEY_CURRENT_USER, key.c_str(), name, REG_SZ,
                                              value.c_str(),
                                              (DWORD)((value.size() + 1) * sizeof(wchar_t))));
}

HRESULT RegisterComClass(const std::wstring& clsid, const wchar_t* name, const wchar_t* dll) {
    const std::wstring key = L"Software\\Classes\\CLSID\\" + clsid;
    HRESULT hr = SetValue(key, nullptr, name);
    if (SUCCEEDED(hr)) hr = SetValue(key + L"\\InprocServer32", nullptr, dll);
    if (SUCCEEDED(hr)) hr = SetValue(key + L"\\InprocServer32", L"ThreadingModel", L"Apartment");
    return hr;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hInst = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv) {
    *ppv = nullptr;
    int overlay = -1;
    if (!IsEqualCLSID(clsid, CLSID_FolderTagsMenu)) {
        overlay = OverlayTagFromClsid(clsid);
        if (overlay < 0) return CLASS_E_CLASSNOTAVAILABLE;
    }
    auto f = new (std::nothrow) ClassFactory(overlay);
    if (!f) return E_OUTOFMEMORY;
    const HRESULT hr = f->QueryInterface(riid, ppv);
    f->Release();
    return hr;
}

STDAPI DllCanUnloadNow() { return g_dllRefs == 0 ? S_OK : S_FALSE; }

// Per-user registration (HKCU): no administrator rights needed.
// Overlay identifiers must additionally be listed under HKLM (scripts/install-overlays.ps1).
STDAPI DllRegisterServer() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(g_hInst, path, MAX_PATH);

    HRESULT hr = RegisterComClass(kClsidStr, L"FolderTags Context Menu", path);
    RegDeleteTreeW(HKEY_CURRENT_USER, kLegacyHandlerKey);
    for (auto key : kHandlerKeys)
        if (SUCCEEDED(hr)) hr = SetValue(key, nullptr, kClsidStr);
    for (int i = 0; SUCCEEDED(hr) && i < tags::kCount; ++i) {
        const std::wstring name = std::wstring(L"FolderTags Overlay ") + tags::kTags[i].label;
        hr = RegisterComClass(OverlayClsidString(i), name.c_str(), path);
    }
    if (SUCCEEDED(hr)) hr = index::Setup();
    if (SUCCEEDED(hr)) SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return hr;
}

STDAPI DllUnregisterServer() {
    for (auto key : kHandlerKeys) RegDeleteTreeW(HKEY_CURRENT_USER, key);
    RegDeleteTreeW(HKEY_CURRENT_USER, kLegacyHandlerKey);
    RegDeleteTreeW(HKEY_CURRENT_USER,
                   (std::wstring(L"Software\\Classes\\CLSID\\") + kClsidStr).c_str());
    for (int i = 0; i < tags::kCount; ++i)
        RegDeleteTreeW(HKEY_CURRENT_USER,
                       (L"Software\\Classes\\CLSID\\" + OverlayClsidString(i)).c_str());
    index::Teardown();
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return S_OK;
}
