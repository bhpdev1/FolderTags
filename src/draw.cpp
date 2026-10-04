#include "draw.h"

using namespace Gdiplus;

namespace draw {

void EnsureGdiplus() {
    static ULONG_PTR token = 0;
    if (!token) {
        GdiplusStartupInput in;
        GdiplusStartup(&token, &in, nullptr);
    }
}

static BYTE Darken(BYTE c, float f) { return (BYTE)(c * f); }

void PaintDot(Graphics& g, float cx, float cy, float r, int tag, tags::State st, bool hover,
              bool dark) {
    const auto& t = tags::kTags[tag];

    if (hover) {
        const float ringW = std::max(1.0f, r * 0.2f);
        const float rr = r * 1.45f;
        Pen ring(dark ? Color(170, 255, 255, 255) : Color(120, 0, 0, 0), ringW);
        g.DrawEllipse(&ring, cx - rr, cy - rr, rr * 2, rr * 2);
    }

    SolidBrush fill(Color(255, t.r, t.g, t.b));
    g.FillEllipse(&fill, cx - r, cy - r, r * 2, r * 2);

    const float bw = std::max(1.0f, r * 0.12f);
    Pen border(Color(255, Darken(t.r, 0.82f), Darken(t.g, 0.82f), Darken(t.b, 0.82f)), bw);
    const float br = r - bw / 2;
    g.DrawEllipse(&border, cx - br, cy - br, br * 2, br * 2);

    const bool lightFill = (0.299f * t.r + 0.587f * t.g + 0.114f * t.b) > 186.0f;
    Pen sym(lightFill ? Color(210, 70, 55, 0) : Color(255, 255, 255, 255),
            std::max(1.2f, r * 0.26f));
    sym.SetStartCap(LineCapRound);
    sym.SetEndCap(LineCapRound);
    sym.SetLineJoin(LineJoinRound);
    const float s = r * 0.5f;

    if (hover) {
        if (st == tags::State::All) {
            const float d = s * 0.8f;
            g.DrawLine(&sym, cx - d, cy - d, cx + d, cy + d);
            g.DrawLine(&sym, cx - d, cy + d, cx + d, cy - d);
        } else {
            g.DrawLine(&sym, cx - s, cy, cx + s, cy);
            g.DrawLine(&sym, cx, cy - s, cx, cy + s);
        }
    } else if (st == tags::State::All) {
        PointF pts[3] = {{cx - s * 0.95f, cy + s * 0.05f},
                         {cx - s * 0.25f, cy + s * 0.7f},
                         {cx + s * 0.95f, cy - s * 0.65f}};
        g.DrawLines(&sym, pts, 3);
    } else if (st == tags::State::Partial) {
        g.DrawLine(&sym, cx - s * 0.8f, cy, cx + s * 0.8f, cy);
    }
}

}  // namespace draw
