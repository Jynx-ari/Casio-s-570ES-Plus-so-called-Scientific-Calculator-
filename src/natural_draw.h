#pragma once
// natural_draw.h -- lays out and draws Natural-Display trees with U8g2 primitives:
// stacked fractions, radicals with overbar, raised exponents, stretching parentheses,
// |abs| bars, log with subscript base, mixed fractions, pi glyph, empty-slot boxes.
#include "display.h"
#include "natural.h"

namespace nat {

struct Box { int w = 0, asc = 0, desc = 0; int h() const { return asc + desc; } };

class Renderer {
public:
    explicit Renderer(Display &d);

    bool dry = false;                       // measure/locate cursor only, draw nothing
    const Cursor *cursor = nullptr;         // where to report the cursor position
    bool curFound = false;
    int curX = 0, curTop = 0, curBot = 0;   // last row inclusive

    Box measure(const Seq &s, int level = 0);
    void draw(const Seq &s, int x, int baseline, int level = 0);

    int fontAsc(int level) const { return asc_[level > 0]; }

    // shared with screen code
    void arrowLeft(int x, int yc);          // small scroll triangles
    void arrowRight(int x, int yc);

private:
    struct Item { Box b; int pAsc = 0, pDesc = 0; };
    struct Laid { std::vector<Item> it; Box box; };

    Display &d_;
    int asc_[2], desc_[2];
    Display::Font font(int lv) const { return lv > 0 ? Display::Font::Small : Display::Font::Main; }

    Laid layout(const Seq &s, int lv);
    Box nodeBox(const Node &n, int lv);
    void drawNode(const Node &n, const Item &it, int x, int by, int lv);
    void drawPlaceholder(int x, int by, int lv);
    void paren(int x, int top, int bot, bool leftSide);

    int axis(int lv) const { return asc_[lv > 0] / 2; }
    Box fracBox(const Box &n, const Box &dn, int lv) const;
    void drawFracParts(const Seq &num, const Seq &den, int x, int by, int lv);
};

} // namespace nat
