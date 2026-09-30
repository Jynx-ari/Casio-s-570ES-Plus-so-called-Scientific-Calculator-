#include "natural_draw.h"
#include "compact_font.h"
#include <algorithm>

namespace nat {

namespace {
constexpr int PARENW = 4;       // paren glyph 3px + 1px spacing
constexpr int PLW = 6;          // empty-slot box (5px) + spacing
constexpr int RADW = 7;         // radical sign width
const char *const PI_ROWS[5] = {"#####.", ".#.#..", ".#.#..", ".#.#..", ".#..##"};
const char *const TRI_L[5] = {"..#", ".##", "###", ".##", "..#"};
const char *const TRI_R[5] = {"#..", "##.", "###", "##.", "#.."};
const char *const DIGITS[10][7] = {
    {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."},
    {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."},
    {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"},
    {"####.", "....#", "....#", ".###.", "....#", "....#", "####."},
    {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."},
    {"#####", "#....", "#....", "####.", "....#", "#...#", ".###."},
    {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."},
    {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."},
    {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."},
    {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."}
};
const char *const PERMUTATION_ROWS[8] = {".####.", "##..##", "##..##", "#####.", "##....", "##....", "##....", "##...."};
const char *const COMBINATION_ROWS[8] = {"..####.", ".##....", "##.....", "##.....", "##.....", "##.....", ".##....", "..####."};
const char *const PERCENT_ROWS[7] = {
    "##...",
    "##..#",
    "...#.",
    "..#..",
    ".#...",
    "#..##",
    "...##"
};

bool hasDescender(const std::string &t) { return t.find_first_of("gjpqy") != std::string::npos; }
bool isDigitToken(const Node &n) {
    return n.kind == Kind::Text && n.text.size() == 1 && n.text[0] >= '0' && n.text[0] <= '9';
}
bool isCombinationToken(const Node &n) {
    return n.kind == Kind::Text && (n.text == "P" || n.text == "C");
}
bool isAlphabetToken(const Node &n) {
    return n.kind == Kind::Text && n.text.size() == 1 && lcd_font::glyph(n.text[0]) != nullptr;
}
} // namespace

Renderer::Renderer(Display &d) : d_(d) {
    asc_[0] = d.ascent(Display::Font::Main);  desc_[0] = d.descent(Display::Font::Main);
    asc_[1] = d.ascent(Display::Font::Small); desc_[1] = d.descent(Display::Font::Small);
}

Box Renderer::measure(const Seq &s, int lv) { return layout(s, lv).box; }

Box Renderer::fracBox(const Box &n, const Box &dn, int lv) const {
    Box b;
    b.w = std::max(n.w, dn.w) + 4;
    // rows used: [by-asc, by+desc-1];  bar at by-axis, 1 blank row above/below it
    b.asc = axis(lv) + 1 + n.h();
    b.desc = std::max(0, dn.h() + 2 - axis(lv));
    return b;
}

Box Renderer::nodeBox(const Node &n, int lv) {
    Box b;
    const int fa = asc_[lv > 0], fd = desc_[lv > 0];
    switch (n.kind) {
        case Kind::Text: {
            if (isDigitToken(n)) { b.w = 6; b.asc = 7; b.desc = 0; return b; }
            if (isCombinationToken(n)) { b.w = 8; b.asc = 8; b.desc = 0; return b; }
            if (isAlphabetToken(n)) { b.w = lcd_font::CellWidth; b.asc = lcd_font::GlyphHeight; b.desc = 0; return b; }
            if (n.text == "\xCF\x80") { b.w = 6; b.asc = 5; b.desc = 0; return b; }      // pi
            if (n.text == ".") { b.w = 3; b.asc = fa; b.desc = 0; return b; }               // narrow dot
            int pad = isBinaryOp(n) ? 1 : 0;
            b.w = d_.textWidth(n.text, font(lv)) + 2 * pad;
            if (!n.sup.empty()) b.w += d_.textWidth(n.sup, Display::Font::Small);
            if (n.open || n.close) b.w += PARENW;
            b.asc = fa; b.desc = hasDescender(n.text) ? fd : 0;
            return b;
        }
        case Kind::Frac: {
            Box a = measure(n.slots[0], lv), c = measure(n.slots[1], lv);
            return fracBox(a, c, lv);
        }
        case Kind::Mixed: {
            Box i = measure(n.slots[0], lv), a = measure(n.slots[1], lv), c = measure(n.slots[2], lv);
            Box f = fracBox(a, c, lv);
            b.w = i.w + 1 + f.w; b.asc = std::max(i.asc, f.asc); b.desc = std::max(i.desc, f.desc);
            return b;
        }
        case Kind::Sqrt: {
            Box c = measure(n.slots[0], lv);
            b.w = RADW + c.w + 2; b.asc = c.asc + 2; b.desc = std::max(c.desc, 1) + 1;
            return b;
        }
        case Kind::Root: {
            Box i = measure(n.slots[0], 1), c = measure(n.slots[1], lv);
            int lead = std::max(0, i.w - 3);
            b.w = lead + RADW + c.w + 2; b.asc = c.asc + 2; b.desc = std::max(c.desc, 1) + 1;
            int rise = (c.asc + 2) * 2 / 3;       // index sits above the radical tick
            b.asc = std::max(b.asc, rise + i.h() - 1);
            return b;
        }
        case Kind::Pow: {
            Box e = measure(n.slots[0], 1);
            int shift = std::max(3, e.desc + 3);
            b.w = e.w + 1; b.asc = shift + e.asc; b.desc = 0;
            return b;
        }
        case Kind::Abs: {
            Box c = measure(n.slots[0], lv);
            b.w = c.w + 6; b.asc = c.asc + 1; b.desc = c.desc;
            return b;
        }
        case Kind::LogB: {
            Box base = measure(n.slots[0], 1), arg = measure(n.slots[1], lv);
            int logW = d_.textWidth("log", font(lv));
            b.w = logW + base.w + PARENW + arg.w + PARENW;
            b.asc = std::max(fa + 1, arg.asc + 1);
            b.desc = std::max({arg.desc, 3 + base.desc, fd});
            return b;
        }
    }
    return b;
}

Renderer::Laid Renderer::layout(const Seq &s, int lv) {
    Laid L;
    const int fa = asc_[lv > 0];
    if (s.v.empty()) {
        L.box = s.owner ? Box{PLW, fa, 0} : Box{0, fa, 0};
        return L;
    }
    for (auto &n : s.v) {
        Item it; it.b = nodeBox(*n, lv);
        it.pAsc = fa + 1; it.pDesc = 1;
        if (n->open || n->close) { it.b.asc = std::max(it.b.asc, it.pAsc); it.b.desc = std::max(it.b.desc, it.pDesc); }
        L.it.push_back(it);
    }
    for (int i = 0; i + 1 < (int)s.v.size(); ++i)
        if (isDigitToken(*s.v[i]) && isDigitToken(*s.v[i + 1])) ++L.it[i].b.w;
    // stretch matched parentheses over their contents
    std::vector<int> st;
    auto stretch = [&](int o, int c) {
        int ma = 0, md = 0;
        for (int k = o + 1; k < c; ++k) { ma = std::max(ma, L.it[k].b.asc); md = std::max(md, L.it[k].b.desc); }
        int pa = std::max(L.it[o].pAsc, ma + 1), pd = std::max(L.it[o].pDesc, md);
        for (int idx : {o, c}) {
            if (idx >= (int)L.it.size()) continue;
            L.it[idx].pAsc = pa; L.it[idx].pDesc = pd;
            L.it[idx].b.asc = std::max(L.it[idx].b.asc, pa);
            L.it[idx].b.desc = std::max(L.it[idx].b.desc, pd);
        }
    };
    for (int i = 0; i < (int)s.v.size(); ++i) {
        if (s.v[i]->open) st.push_back(i);
        else if (s.v[i]->close && !st.empty()) { int o = st.back(); st.pop_back(); stretch(o, i); }
    }
    for (int o : st) stretch(o, (int)s.v.size());
    for (auto &it : L.it) { L.box.w += it.b.w; L.box.asc = std::max(L.box.asc, it.b.asc); L.box.desc = std::max(L.box.desc, it.b.desc); }
    return L;
}

void Renderer::paren(int x, int top, int bot, bool left) {
    if (dry) return;
    int h = bot - top + 1;
    if (h < 5) { d_.vLine(x + 1, top, h); return; }
    if (left) {
        d_.pixel(x + 2, top); d_.pixel(x + 1, top + 1); d_.vLine(x, top + 2, h - 4);
        d_.pixel(x + 1, bot - 1); d_.pixel(x + 2, bot);
    } else {
        d_.pixel(x, top); d_.pixel(x + 1, top + 1); d_.vLine(x + 2, top + 2, h - 4);
        d_.pixel(x + 1, bot - 1); d_.pixel(x, bot);
    }
}
void Renderer::arrowLeft(int x, int yc) { if (!dry) d_.glyph(x, yc - 2, TRI_L, 5); }
void Renderer::arrowRight(int x, int yc) { if (!dry) d_.glyph(x, yc - 2, TRI_R, 5); }

void Renderer::drawPlaceholder(int x, int by, int lv) {
    if (dry) return;
    int h = asc_[lv > 0];
    d_.frame(x, by - h, 5, h);
}

void Renderer::draw(const Seq &s, int x, int by, int lv) {
    Laid L = layout(s, lv);
    const bool here = cursor && cursor->seq == &s;
    auto mark = [&](int cx) {
        if (!here) return;
        curFound = true; curX = cx;
        curTop = by - std::max(L.box.asc, asc_[lv > 0]);
        curBot = by + std::max(L.box.desc, 1) - 1;
    };
    if (s.v.empty()) {
        if (s.owner) drawPlaceholder(x, by, lv);
        if (here) mark(x);
        return;
    }
    int cx = x;
    for (int i = 0; i < (int)s.v.size(); ++i) {
        if (here && cursor->pos == i) mark(cx);
        drawNode(*s.v[i], L.it[i], cx, by, lv);
        cx += L.it[i].b.w;
    }
    if (here && cursor->pos == (int)s.v.size()) mark(cx);
}

void Renderer::drawFracParts(const Seq &num, const Seq &den, int x, int by, int lv) {
    Box n = measure(num, lv), c = measure(den, lv);
    Box f = fracBox(n, c, lv);
    int yb = by - axis(lv);
    if (!dry) d_.hLine(x, yb, f.w);
    draw(num, x + (f.w - n.w) / 2, yb - 1 - n.desc, lv);
    draw(den, x + (f.w - c.w) / 2, yb + 2 + c.asc, lv);
}

void Renderer::drawNode(const Node &n, const Item &it, int x, int by, int lv) {
    const Display::Font f = font(lv);
    switch (n.kind) {
        case Kind::Text: {
            if (n.text == "P") { if (!dry) d_.glyph(x, by - 8, PERMUTATION_ROWS, 8); return; }
            if (n.text == "C") { if (!dry) d_.glyph(x, by - 8, COMBINATION_ROWS, 8); return; }
            if (isAlphabetToken(n)) {
                const lcd_font::Glyph *glyph = lcd_font::glyph(n.text[0]);
                if (!dry) {
                    for (int row = 0; row < lcd_font::GlyphHeight; ++row)
                        for (int col = 0; col < lcd_font::GlyphWidth; ++col)
                            if (glyph->rows[row] & (1u << (lcd_font::GlyphWidth - col - 1)))
                                d_.pixel(x + col, by - lcd_font::GlyphHeight + row);
                }
                return;
            }
            if (isDigitToken(n)) {
                if (!dry) d_.glyph(x, by - 7, DIGITS[n.text[0] - '0'], 7);
                return;
            }
            if (n.text == "\xCF\x80") { if (!dry) d_.glyph(x, by - 5, PI_ROWS, 5); return; }
            if (n.text == "%") { if (!dry) d_.glyph(x, by - 7, PERCENT_ROWS, 7); return; }
            if (n.text == ".") { if (!dry) d_.box(x + 1, by - 2, 2, 2); return; }
            int pad = isBinaryOp(n) ? 1 : 0;
            int tx = x + pad;
            if (!n.text.empty() && !dry) d_.text(tx, by, n.text, f);
            tx += d_.textWidth(n.text, f);
            if (!n.sup.empty()) {
                if (!dry) d_.text(tx, by - 3, n.sup, Display::Font::Small);
                tx += d_.textWidth(n.sup, Display::Font::Small);
            }
            if (n.open) paren(tx, by - it.pAsc, by + it.pDesc - 1, true);
            if (n.close) paren(x, by - it.pAsc, by + it.pDesc - 1, false);
            return;
        }
        case Kind::Frac: drawFracParts(n.slots[0], n.slots[1], x, by, lv); return;
        case Kind::Mixed: {
            Box i = measure(n.slots[0], lv);
            draw(n.slots[0], x, by, lv);
            drawFracParts(n.slots[1], n.slots[2], x + i.w + 1, by, lv);
            return;
        }
        case Kind::Sqrt: case Kind::Root: {
            const bool root = n.kind == Kind::Root;
            const Seq &rad = n.slots[root ? 1 : 0];
            Box c = measure(rad, lv);
            int lead = 0;
            int yt = by - c.asc - 2;                       // overbar row (1 blank row above the content)
            int yb = by + std::max(c.desc, 1);             // bottom of the radical sign
            int ym = yt + (yb - yt) * 2 / 3;
            if (root) {
                Box i = measure(n.slots[0], 1);
                lead = std::max(0, i.w - 3);
                draw(n.slots[0], x, ym - i.desc, 1);
            }
            int x0 = x + lead + 1;
            if (!dry) {
                d_.line(x0, ym, x0 + 2, yb);
                d_.line(x0 + 2, yb, x0 + 5, yt);
                d_.hLine(x0 + 5, yt, c.w + 3);
            }
            draw(rad, x + lead + RADW, by, lv);
            return;
        }
        case Kind::Pow: {
            Box e = measure(n.slots[0], 1);
            int shift = std::max(3, e.desc + 3);
            draw(n.slots[0], x, by - shift, 1);
            return;
        }
        case Kind::Abs: {
            Box c = measure(n.slots[0], lv);
            int top = by - it.b.asc, bot = by + it.b.desc - 1;
            if (!dry) { d_.vLine(x + 1, top, bot - top + 1); d_.vLine(x + it.b.w - 2, top, bot - top + 1); }
            draw(n.slots[0], x + 3, by, lv);
            (void)c;
            return;
        }
        case Kind::LogB: {
            int tx = x;
            if (!dry) d_.text(tx, by, "log", f);
            tx += d_.textWidth("log", f);
            Box base = measure(n.slots[0], 1), arg = measure(n.slots[1], lv);
            draw(n.slots[0], tx, by + 3, 1);
            tx += base.w;
            int pa = std::max(asc_[lv > 0] + 1, arg.asc + 1), pd = std::max(arg.desc, 1);
            paren(tx, by - pa, by + pd - 1, true);
            tx += PARENW;
            draw(n.slots[1], tx, by, lv);
            tx += arg.w;
            paren(tx, by - pa, by + pd - 1, false);
            return;
        }
    }
}

} // namespace nat
