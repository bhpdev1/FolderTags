// Shared helpers for the mockups.
// - "#bare" in the URL switches to the transparent variant used inside the hero.
// - drawDot() reproduces draw::PaintDot (src/draw.cpp) as SVG.
if (location.hash === "#bare") document.body.classList.add("bare");

const TAGS = [
  { key: "red",    rgb: [240, 82, 79] },
  { key: "orange", rgb: [246, 155, 48] },
  { key: "yellow", rgb: [247, 203, 63] },
  { key: "green",  rgb: [98, 194, 85] },
  { key: "blue",   rgb: [56, 138, 240] },
  { key: "purple", rgb: [176, 99, 214] },
  { key: "gray",   rgb: [152, 152, 157] },
];

// state: "none" | "all" | "partial"; hover adds the ring and the +/x symbol.
function drawDot(parent, cx, cy, r, tag, state = "none", hover = false, dark = true) {
  const [R, G, B] = TAGS[tag].rgb;
  const pad = r * 1.8, size = pad * 2;
  const ns = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(ns, "svg");
  svg.setAttribute("class", "dot");
  svg.setAttribute("width", size); svg.setAttribute("height", size);
  svg.setAttribute("viewBox", `${-pad} ${-pad} ${size} ${size}`);
  svg.style.left = `${cx - pad}px`; svg.style.top = `${cy - pad}px`;
  const el = (name, attrs) => {
    const e = document.createElementNS(ns, name);
    for (const k in attrs) e.setAttribute(k, attrs[k]);
    svg.appendChild(e); return e;
  };
  if (hover) {
    el("circle", { r: r * 1.45, fill: "none", "stroke-width": Math.max(1, r * 0.2),
      stroke: dark ? "rgba(255,255,255,.667)" : "rgba(0,0,0,.47)" });
  }
  el("circle", { r, fill: `rgb(${R},${G},${B})` });
  const bw = Math.max(1, r * 0.12);
  el("circle", { r: r - bw / 2, fill: "none", "stroke-width": bw,
    stroke: `rgb(${Math.floor(R * .82)},${Math.floor(G * .82)},${Math.floor(B * .82)})` });

  const light = 0.299 * R + 0.587 * G + 0.114 * B > 186;
  const sym = { fill: "none", "stroke-width": Math.max(1.2, r * 0.26), "stroke-linecap": "round",
    "stroke-linejoin": "round", stroke: light ? "rgba(70,55,0,.82)" : "#fff" };
  const s = r * 0.5;
  if (hover) {
    if (state === "all") {
      const d = s * 0.8;
      el("path", { ...sym, d: `M${-d} ${-d}L${d} ${d}M${-d} ${d}L${d} ${-d}` });
    } else {
      el("path", { ...sym, d: `M${-s} 0H${s}M0 ${-s}V${s}` });
    }
  } else if (state === "all") {
    el("path", { ...sym, d: `M${-s * .95} ${s * .05}L${-s * .25} ${s * .7}L${s * .95} ${-s * .65}` });
  } else if (state === "partial") {
    el("path", { ...sym, d: `M${-s * .8} 0H${s * .8}` });
  }
  parent.appendChild(svg);
  return svg;
}
