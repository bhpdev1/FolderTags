#pragma once
#include "common.h"
#include <algorithm>
namespace Gdiplus {
using std::max;
using std::min;
}  // namespace Gdiplus
#include <gdiplus.h>
#include "tags.h"

namespace draw {

void EnsureGdiplus();

// Paints one macOS-style tag dot. On hover: halo ring + "+" (add) or "×" (remove).
// When applied (and not hovered): check mark; partially applied: dash.
void PaintDot(Gdiplus::Graphics& g, float cx, float cy, float r, int tag, tags::State st,
              bool hover, bool dark);

}  // namespace draw
