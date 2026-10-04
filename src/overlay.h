#pragma once
#include "common.h"

// Icon overlay handler: one COM class per color (Windows allows a single icon per handler).
// {B6E4CD56-987A-4C8F-8729-7FD3D4D8EBE0} .. {...EBE6}
CLSID OverlayClsid(int tag);
std::wstring OverlayClsidString(int tag);
int OverlayTagFromClsid(REFCLSID clsid);  // -1 if not an overlay CLSID

IUnknown* CreateOverlay(int tag);
void OverlayInvalidateCache();
