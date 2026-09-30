// casio_ui_sim -- fx-570ES PLUS-style LCD simulator (LCD only) on real U8g2.
//
//   casio_ui_sim                        interactive window (keyboard)
//   casio_ui_sim --keys "1 frac 2 = @out.bmp"   scripted run, no window needed
//   options: --scale N  --ascii  (dump the 128x64 glass as text after the script)
#include <SDL2/SDL.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "calculator.h"
#include "display.h"

using app::Key;

namespace {

// ---------------- LCD -> SDL surface (pixel grid look) ----------------
void paintLcd(SDL_Surface *s, int scale, int margin) {
    SDL_FillRect(s, nullptr, SDL_MapRGB(s->format, 58, 62, 54));                     // bezel
    SDL_Rect glass{margin - 6, margin - 6, Display::width() * scale + 12, Display::height() * scale + 12};
    SDL_FillRect(s, &glass, SDL_MapRGB(s->format, 196, 204, 172));                   // LCD background
    if (!Display::powered()) return;
    Uint32 on = SDL_MapRGB(s->format, 24, 28, 22);
    Uint32 ghost = SDL_MapRGB(s->format, 188, 196, 164);
    int gap = scale >= 4 ? 1 : 0;
    for (int y = 0; y < Display::height(); ++y)
        for (int x = 0; x < Display::width(); ++x) {
            SDL_Rect r{margin + x * scale, margin + y * scale, scale - gap, scale - gap};
            SDL_FillRect(s, &r, Display::pixelAt(x, y) ? on : ghost);
        }
}

void dumpAscii() {
    for (int y = 0; y < Display::height(); ++y) {
        for (int x = 0; x < Display::width(); ++x) putchar(Display::pixelAt(x, y) ? '#' : '.');
        putchar('\n');
    }
}

bool saveShot(const std::string &path, int scale) {
    int margin = 16;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, Display::width() * scale + 2 * margin,
                                                   Display::height() * scale + 2 * margin, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return false;
    paintLcd(s, scale, margin);
    bool ok = SDL_SaveBMP(s, path.c_str()) == 0;
    SDL_FreeSurface(s);
    return ok;
}

// ---------------- key names for scripts ----------------
struct Seq1 { std::vector<Key> keys; };
const std::map<std::string, std::vector<Key>> &keyTable() {
    static const std::map<std::string, std::vector<Key>> t = {
        {"0", {Key::N0}}, {"1", {Key::N1}}, {"2", {Key::N2}}, {"3", {Key::N3}}, {"4", {Key::N4}},
        {"5", {Key::N5}}, {"6", {Key::N6}}, {"7", {Key::N7}}, {"8", {Key::N8}}, {"9", {Key::N9}},
        {".", {Key::Dot}}, {"+", {Key::Plus}}, {"-", {Key::Minus}}, {"*", {Key::Mul}}, {"/", {Key::Div}},
        {"neg", {Key::Neg}}, {"(", {Key::LPar}}, {")", {Key::RPar}},
        {"frac", {Key::Frac}}, {"sqrt", {Key::Sqrt}}, {"sq", {Key::Sq}}, {"pow", {Key::Pow}},
        {"recip", {Key::Recip}}, {"log", {Key::Log}}, {"logb", {Key::LogB}}, {"ln", {Key::Ln}},
        {"sin", {Key::Sin}}, {"cos", {Key::Cos}}, {"tan", {Key::Tan}}, {"abs", {Key::Abs}},
        {"exp10", {Key::Exp10}}, {"ans", {Key::Ans}}, {"=", {Key::Eq}}, {"sd", {Key::SD}},
        {"del", {Key::Del}}, {"ac", {Key::Ac}}, {"left", {Key::Left}}, {"right", {Key::Right}},
        {"up", {Key::Up}}, {"down", {Key::Down}}, {"shift", {Key::Shift}}, {"alpha", {Key::Alpha}},
        {"hyp", {Key::Hyp}},
        {"mode", {Key::Mode}},
        {"comma", {Key::Comma}}, {"npr", {Key::NPr}}, {"ncr", {Key::NCr}},
        {"mod", {Key::Mod}}, {"asinh", {Key::Asinh}}, {"acosh", {Key::Acosh}}, {"atanh", {Key::Atanh}},
        {"pct", {Key::Pct}}, {"floor", {Key::Floor}}, {"ceil", {Key::Ceil}},
        {"imag", {Key::Imaginary}}, {"arg", {Key::Arg}},
        {"pct", {Key::Pct}}, {"floor", {Key::Floor}}, {"ceil", {Key::Ceil}},
            {"imag", {Key::Imaginary}}, {"arg", {Key::Arg}},
        {"gcd", {Key::Gcd}}, {"lcm", {Key::Lcm}},
        {"sto", {Key::Shift, Key::Ans}}, {"A", {Key::Alpha, Key::N1}}, {"B", {Key::Alpha, Key::N2}},
        {"C", {Key::Alpha, Key::N3}}, {"D", {Key::Alpha, Key::N4}}, {"E", {Key::Alpha, Key::N5}},
        {"F", {Key::Alpha, Key::N6}}, {"X", {Key::Alpha, Key::N7}}, {"Y", {Key::Alpha, Key::N8}},
        {"M", {Key::Alpha, Key::N9}},
        {"and", {Key::BitAnd}}, {"or", {Key::BitOr}}, {"xor", {Key::BitXor}},
        {"xnor", {Key::BitXnor}}, {"not", {Key::BitNot}},
        {"mplus", {Key::MPlus}}, {"mminus", {Key::Shift, Key::MPlus}},
        {"mr", {Key::MR}}, {"mc", {Key::Shift, Key::MR}},
        // shifted / alpha functions written as one word
        {"mixed", {Key::Shift, Key::Frac}}, {"cbrt", {Key::Shift, Key::Sqrt}}, {"cube", {Key::Shift, Key::Sq}},
        {"root", {Key::Shift, Key::Pow}}, {"fact", {Key::Shift, Key::Recip}}, {"10^", {Key::Shift, Key::Log}},
        {"e^", {Key::Shift, Key::Ln}}, {"asin", {Key::Shift, Key::Sin}}, {"acos", {Key::Shift, Key::Cos}},
        {"atan", {Key::Shift, Key::Tan}}, {"pi", {Key::Shift, Key::Exp10}}, {"e", {Key::Alpha, Key::Exp10}},
        {"sinh", {Key::Hyp, Key::Sin}}, {"cosh", {Key::Hyp, Key::Cos}}, {"tanh", {Key::Hyp, Key::Tan}},
        {"setup", {Key::Shift, Key::Mode}},
    };
    return t;
}

void runScript(app::Calculator &calc, Display &d, const std::string &script, int scale) {
    std::istringstream in(script);
    std::string tok;
    while (in >> tok) {
        if (tok[0] == '@') {
            calc.draw(d, true);
            if (!saveShot(tok.substr(1), scale)) std::cerr << "could not save " << tok.substr(1) << "\n";
            continue;
        }
        auto it = keyTable().find(tok);
        if (it == keyTable().end()) { std::cerr << "unknown key: " << tok << "\n"; continue; }
        for (Key k : it->second) calc.press(k);
    }
    calc.draw(d, true);
}

const char *HELP =
    "Keys: 0-9 . + - * /   ( )   Enter/= equals   Backspace DEL   Delete AC   arrows\n"
    "  SHIFT+f mixed   SHIFT+r cube-root   SHIFT+q cube   SHIFT+h nth-root   SHIFT+v factorial\n"
    "    SHIFT+l 10^x   SHIFT+n e^x   SHIFT+s/c/t inverse trig   SHIFT+e pi   ALPHA+e Euler e\n"
    "    HYP then s/c/t = sinh/cosh/tanh (desktop HYP key: y)\n"
    "    a STO   m SETUP\n"
    "  f fraction   r sqrt   q x^2   ^ power   v x^-1   l log   n ln   s/c/t sin/cos/tan\n"
    "  i imaginary   F10 floor   F11 ceil   F12 arg\n"
    "  j nPr   k nCr   % percent   , comma   g/i/o asinh/acosh/atanh\n"
    "  b abs   e x10^x   a Ans (Shift+a STO)   p pi   m MODE   u SETUP   Tab S<=>D   Shift   x ALPHA\n"
    "  BASE-N: F1-F5 logic   Memory: F6 M+ (Shift+F6 M-)   F8 MR (Shift+F8 MC)\n"
    "  GCD/LCM: [ and ] keys\n"
    "  Shift+Ans stores answer (then press variable number)\n";

} // namespace

int main(int argc, char **argv) {
    std::string keys, shotPath; int scale = 5; bool ascii = false, scripted = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--keys" && i + 1 < argc) { keys = argv[++i]; scripted = true; }
        else if (a == "--scale" && i + 1 < argc) scale = std::max(1, atoi(argv[++i]));
        else if (a == "--ascii") ascii = true;
        else { std::cerr << "usage: casio_ui_sim [--keys \"...@file.bmp\"] [--scale N] [--ascii]\n"; return 2; }
    }

    Display display;
    app::Calculator calc;

    if (scripted) {
        runScript(calc, display, keys, scale);
        if (ascii) dumpAscii();
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) { std::cerr << "SDL_Init: " << SDL_GetError() << "\n"; return 1; }
    const int margin = 16;
    SDL_Window *win = SDL_CreateWindow("fx-570ES PLUS display sim (U8g2 128x64)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       Display::width() * scale + 2 * margin, Display::height() * scale + 2 * margin, SDL_WINDOW_SHOWN);
    if (!win) { std::cerr << "window: " << SDL_GetError() << "\n"; return 1; }
    std::cout << HELP;

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type != SDL_KEYDOWN) continue;
            SDL_Keycode k = e.key.keysym.sym;
            bool shifted = (e.key.keysym.mod & KMOD_SHIFT) != 0;
            auto press = [&](const char *name) { for (Key kk : keyTable().at(name)) calc.press(kk); };
            switch (k) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_0: if (shifted) press(")"); else press("0"); break;
                case SDLK_KP_0: press("0"); break;               case SDLK_1: case SDLK_KP_1: press("1"); break;
                case SDLK_2: case SDLK_KP_2: press("2"); break;   case SDLK_3: case SDLK_KP_3: press("3"); break;
                case SDLK_4: case SDLK_KP_4: press("4"); break;
            case SDLK_5: if (shifted) press("pct"); else press("5"); break;
            case SDLK_KP_5: press("5"); break;
                case SDLK_6: if (shifted) press("pow"); else press("6"); break;
                case SDLK_KP_6: press("6"); break;               case SDLK_7: case SDLK_KP_7: press("7"); break;
                case SDLK_8: case SDLK_KP_8: if (shifted) press("*"); else press("8"); break;
                case SDLK_9: case SDLK_KP_9: if (shifted) press("("); else press("9"); break;
                default: break;
            }
            switch (k) {
                case SDLK_PERIOD: case SDLK_KP_PERIOD: press("."); break;
                case SDLK_PLUS: case SDLK_KP_PLUS: press("+"); break;
                case SDLK_EQUALS: if (shifted) press("+"); else press("="); break;
                case SDLK_MINUS: case SDLK_KP_MINUS: press(shifted ? "neg" : "-"); break;
                case SDLK_ASTERISK: case SDLK_KP_MULTIPLY: press("*"); break;
                case SDLK_SLASH: case SDLK_KP_DIVIDE: press("/"); break;
                case SDLK_LEFTPAREN: press("("); break;
                case SDLK_RIGHTPAREN: press(")"); break;
                case SDLK_CARET: press("pow"); break;
                case SDLK_RETURN: case SDLK_KP_ENTER: press("="); break;
                case SDLK_BACKSPACE: press("del"); break;
                case SDLK_DELETE: press("ac"); break;
                case SDLK_LEFT: press("left"); break;   case SDLK_RIGHT: press("right"); break;
                case SDLK_UP: press("up"); break;       case SDLK_DOWN: press("down"); break;
                case SDLK_TAB: press("sd"); break;
                case SDLK_F1: press("and"); break; case SDLK_F2: press("or"); break;
                case SDLK_F3: press("xor"); break; case SDLK_F4: press("xnor"); break;
                case SDLK_F5: press("not"); break;
                    case SDLK_F10: press("floor"); break; case SDLK_F11: press("ceil"); break;
                    case SDLK_F12: press("arg"); break;
                case SDLK_F6: press(shifted ? "mminus" : "mplus"); break;
                case SDLK_F8: press(shifted ? "mc" : "mr"); break;
                case SDLK_j: press("npr"); break;      case SDLK_k: press("ncr"); break;
                case SDLK_COMMA: press("comma"); break; case SDLK_PERCENT: press("pct"); break;
                case SDLK_LEFTBRACKET: press("gcd"); break; case SDLK_RIGHTBRACKET: press("lcm"); break;
                case SDLK_g: press("asinh"); break;
                case SDLK_i: press(shifted ? "acosh" : "imag"); break;
                case SDLK_o: press("atanh"); break;
                case SDLK_f: press(shifted ? "mixed" : "frac"); break;
                case SDLK_r: press("sqrt"); break;
                case SDLK_q: press("sq"); break;
                case SDLK_h: press("pow"); break;
                case SDLK_v: press("recip"); break;
                case SDLK_l: press("log"); break;
                case SDLK_n: press("ln"); break;
                case SDLK_b: press("abs"); break;
                case SDLK_s: press("sin"); break;
                case SDLK_c: press("cos"); break;
                case SDLK_t: press("tan"); break;
                case SDLK_e: press("exp10"); break;
                case SDLK_a: press("ans"); break;
                case SDLK_p: press("pi"); break;        case SDLK_m: press("mode"); break;
                case SDLK_u: press("setup"); break;
                case SDLK_x: press("alpha"); break;
                case SDLK_y: press("hyp"); break;
                case SDLK_LSHIFT: press("shift"); break;
                default: break;
            }
        }
        bool blink = (SDL_GetTicks() / 500) % 2 == 0;
        calc.draw(display, blink);
        SDL_Surface *ws = SDL_GetWindowSurface(win);
        paintLcd(ws, scale, margin);
        SDL_UpdateWindowSurface(win);
        SDL_Delay(30);
    }
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
