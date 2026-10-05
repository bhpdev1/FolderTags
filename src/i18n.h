#pragma once
#include "common.h"

// UI language: French when the Windows display language is French, English everywhere else.
// Only user-visible strings go through here; anything persisted (NTFS stream content,
// folder/icon file names, registry keys) uses language-neutral identifiers.
namespace i18n {

inline bool IsFrench() {
    static const bool fr = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_FRENCH;
    return fr;
}

inline const wchar_t* T(const wchar_t* en, const wchar_t* fr) { return IsFrench() ? fr : en; }

}  // namespace i18n
