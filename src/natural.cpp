#include "natural.h"
#include <algorithm>

namespace nat {

Node::Node(Kind k) : kind(k) {
    switch (k) {
        case Kind::Text: nslots = 0; break;
        case Kind::Sqrt: case Kind::Pow: case Kind::Abs: nslots = 1; break;
        case Kind::Frac: case Kind::Root: case Kind::LogB: nslots = 2; break;
        case Kind::Mixed: nslots = 3; break;
    }
    for (int i = 0; i < nslots; ++i) { slots[i].owner = this; slots[i].slot = i; }
}

Node *Seq::insert(int pos, NodeP n) {
    Node *raw = n.get();
    raw->parent = this;
    v.insert(v.begin() + pos, std::move(n));
    return raw;
}
int Seq::indexOf(const Node *n) const {
    for (int i = 0; i < (int)v.size(); ++i) if (v[i].get() == n) return i;
    return (int)v.size();
}

NodeP makeText(const std::string &shown, const std::string &engine) {
    auto n = std::make_unique<Node>(Kind::Text);
    n->text = shown; n->engine = engine.empty() ? shown : engine;
    if (shown == "(") { n->text = ""; n->open = true; n->engine = "("; }
    if (shown == ")") { n->text = ""; n->close = true; n->engine = ")"; }
    return n;
}
NodeP makeFunc(const std::string &shown, const std::string &engine, const std::string &sup) {
    auto n = std::make_unique<Node>(Kind::Text);
    n->text = shown; n->engine = engine; n->sup = sup; n->open = true;
    return n;
}
NodeP makeContainer(Kind k) { return std::make_unique<Node>(k); }
Node *addText(Seq &s, const std::string &shown, const std::string &engine) { return s.push(makeText(shown, engine)); }
void addDigits(Seq &s, const std::string &d) { for (char c : d) addText(s, std::string(1, c)); }

void clone(const Seq &src, Seq &dst) {
    for (auto &n : src.v) {
        auto c = std::make_unique<Node>(n->kind);
        c->text = n->text; c->engine = n->engine; c->sup = n->sup; c->open = n->open; c->close = n->close;
        Node *raw = dst.push(std::move(c));
        for (int i = 0; i < raw->nslots; ++i) clone(n->slots[i], raw->slots[i]);
    }
}

std::string toEngine(const Seq &s) {
    std::string o;
    for (auto &np : s.v) {
        const Node &n = *np;
        switch (n.kind) {
            case Kind::Text: o += n.engine; break;
            case Kind::Frac:  o += "((" + toEngine(n.slots[0]) + ")/(" + toEngine(n.slots[1]) + "))"; break;
            case Kind::Mixed: o += "((" + toEngine(n.slots[0]) + ")+(" + toEngine(n.slots[1]) + ")/(" + toEngine(n.slots[2]) + "))"; break;
            case Kind::Sqrt:  o += "sqrt(" + toEngine(n.slots[0]) + ")"; break;
            case Kind::Root: {
                std::string idx = toEngine(n.slots[0]);
                if (idx == "3") o += "cbrt(" + toEngine(n.slots[1]) + ")";
                else o += "((" + toEngine(n.slots[1]) + ")^(1/(" + idx + ")))";
                break;
            }
            case Kind::Pow: o += "^(" + toEngine(n.slots[0]) + ")"; break;
            case Kind::Abs: o += "abs(" + toEngine(n.slots[0]) + ")"; break;
            case Kind::LogB: o += "(ln(" + toEngine(n.slots[1]) + ")/ln(" + toEngine(n.slots[0]) + "))"; break;
        }
    }
    return o;
}

bool isBinaryOp(const Node &n) {
    return n.kind == Kind::Text && (n.text == "+" || n.text == "-" || n.text == "\xC3\x97" || n.text == "\xC3\xB7" || n.text == "=" || n.text == "P" || n.text == "C");
}

// ------------------------------------------------------------------- Editor
void Editor::insertNode(NodeP n) {
    Node *raw = cur.seq->insert(cur.pos, std::move(n));
    cur.pos++;
    (void)raw;
}
void Editor::digits(const std::string &d) { for (char c : d) type(std::string(1, c)); }

static bool isNumTok(const Node &n) {
    return n.kind == Kind::Text && n.text.size() == 1 && (isdigit((unsigned char)n.text[0]) || n.text[0] == '.');
}

void Editor::frac() {
    Seq &s = *cur.seq; int end = cur.pos, st = end;
    while (st > 0 && isNumTok(*s.v[st - 1])) --st;
    auto f = makeContainer(Kind::Frac);
    Node *fr = f.get();
    for (int i = st; i < end; ++i) fr->slots[0].push(std::move(s.v[i]));
    s.v.erase(s.v.begin() + st, s.v.begin() + end);
    s.insert(st, std::move(f));
    enter(fr, end > st ? 1 : 0);
}
void Editor::mixed() {
    auto f = makeContainer(Kind::Mixed); Node *n = f.get();
    insertNode(std::move(f));
    enter(n, 0);
}
void Editor::sqrtT() { auto f = makeContainer(Kind::Sqrt); Node *n = f.get(); insertNode(std::move(f)); enter(n, 0); }
void Editor::rootT(const std::string &pre) {
    auto f = makeContainer(Kind::Root); Node *n = f.get(); insertNode(std::move(f));
    if (pre.empty()) enter(n, 0); else { addDigits(n->slots[0], pre); enter(n, 1); }
}
void Editor::powT(const std::string &preset) {
    auto f = makeContainer(Kind::Pow); Node *n = f.get(); insertNode(std::move(f));
    if (preset.empty()) enter(n, 0);
    else { for (char c : preset) addText(n->slots[0], std::string(1, c)); }   // cursor stays after the node
}
void Editor::absT() { auto f = makeContainer(Kind::Abs); Node *n = f.get(); insertNode(std::move(f)); enter(n, 0); }
void Editor::logB() { auto f = makeContainer(Kind::LogB); Node *n = f.get(); insertNode(std::move(f)); enter(n, 0); }
void Editor::exp10() {
    type("\xC3\x97", "*"); type("1"); type("0");
    powT("");
}

static bool allSlotsEmpty(const Node &n) {
    for (int i = 0; i < n.nslots; ++i) if (!n.slots[i].v.empty()) return false;
    return true;
}

void Editor::del() {
    Seq &s = *cur.seq;
    if (cur.pos > 0) {
        Node *p = s.v[cur.pos - 1].get();
        if (p->kind == Kind::Text) { s.v.erase(s.v.begin() + cur.pos - 1); cur.pos--; }
        else if (allSlotsEmpty(*p)) { s.v.erase(s.v.begin() + cur.pos - 1); cur.pos--; }
        else cur = {&p->slots[p->nslots - 1], p->slots[p->nslots - 1].size()};
        return;
    }
    if (s.owner) {
        Node *o = s.owner;
        if (allSlotsEmpty(*o)) {
            Seq *ps = o->parent; int i = ps->indexOf(o);
            ps->v.erase(ps->v.begin() + i);
            cur = {ps, i};
        } else left();
    }
}

void Editor::right() {
    Seq &s = *cur.seq;
    if (cur.pos < s.size()) {
        Node *n = s.v[cur.pos].get();
        if (n->kind != Kind::Text) enter(n, 0); else cur.pos++;
        return;
    }
    if (s.owner) {
        Node *o = s.owner;
        if (s.slot + 1 < o->nslots) enter(o, s.slot + 1);
        else afterNode(o);
    } else cur = {&root, 0};            // wraps to the beginning, like the real unit
}
void Editor::left() {
    Seq &s = *cur.seq;
    if (cur.pos > 0) {
        Node *n = s.v[cur.pos - 1].get();
        if (n->kind != Kind::Text) { Seq &l = n->slots[n->nslots - 1]; cur = {&l, l.size()}; }
        else cur.pos--;
        return;
    }
    if (s.owner) {
        Node *o = s.owner;
        if (s.slot > 0) { Seq &l = o->slots[s.slot - 1]; cur = {&l, l.size()}; }
        else cur = {o->parent, o->parent->indexOf(o)};
    } else cur = {&root, root.size()};
}
bool Editor::up() {
    for (Seq *s = cur.seq; s->owner; s = s->owner->parent) {
        Node *o = s->owner;
        if ((o->kind == Kind::Frac || o->kind == Kind::Root) && s->slot == 1) { cur = {&o->slots[0], std::min(cur.pos, o->slots[0].size())}; return true; }
        if (o->kind == Kind::Mixed && s->slot == 2) { cur = {&o->slots[1], 0}; return true; }
    }
    return false;
}
bool Editor::down() {
    for (Seq *s = cur.seq; s->owner; s = s->owner->parent) {
        Node *o = s->owner;
        if ((o->kind == Kind::Frac || o->kind == Kind::Root) && s->slot == 0) { cur = {&o->slots[1], std::min(cur.pos, o->slots[1].size())}; return true; }
        if (o->kind == Kind::Mixed && s->slot == 1) { cur = {&o->slots[2], 0}; return true; }
    }
    return false;
}

} // namespace nat
