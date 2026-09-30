#pragma once
// natural.h -- Natural-Display expression tree + editor (no display code here).
// A Seq is a row of Nodes; container nodes (fraction, root, power...) own child
// Seqs ("slots").  The cursor is (Seq*, position) so it can sit inside a
// numerator, a radicand, an exponent, etc. -- exactly like the fx-570ES PLUS.
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace nat {

enum class Kind : uint8_t {
    Text,   // one token: digit, operator, "sin(", "pi", ...
    Frac,   // slots: numerator, denominator
    Mixed,  // slots: integer, numerator, denominator
    Sqrt,   // slots: radicand
    Root,   // slots: index, radicand
    Pow,    // slots: exponent (attached to the item before it)
    Abs,    // slots: inner
    LogB    // slots: base (subscript), argument
};

struct Node;
using NodeP = std::unique_ptr<Node>;

struct Seq {
    std::vector<NodeP> v;
    Node *owner = nullptr;   // container this Seq is a slot of (nullptr = root)
    int slot = 0;
    Seq() = default;
    Seq(const Seq &) = delete;
    Seq &operator=(const Seq &) = delete;
    Node *insert(int pos, NodeP n);
    Node *push(NodeP n) { return insert((int)v.size(), std::move(n)); }
    int indexOf(const Node *n) const;
    void clear() { v.clear(); }
    int size() const { return (int)v.size(); }
};

struct Node {
    Kind kind = Kind::Text;
    std::string text;      // shown text ("sin", "7", "+", "\xC3\x97" ...)
    std::string engine;    // ASCII for the evaluator ("sin(", "7", "+", "*")
    std::string sup;       // small raised suffix, e.g. "-1" for sin^-1
    bool open = false;     // token ends with an (auto-stretching) "("
    bool close = false;    // token is ")"
    Seq slots[3];
    int nslots = 0;
    Seq *parent = nullptr;
    explicit Node(Kind k);
};

NodeP makeText(const std::string &shown, const std::string &engine = "");
NodeP makeFunc(const std::string &shown, const std::string &engine, const std::string &sup = "");
NodeP makeContainer(Kind k);
Node *addText(Seq &s, const std::string &shown, const std::string &engine = "");
void addDigits(Seq &s, const std::string &digits);          // one node per character
void clone(const Seq &src, Seq &dst);
std::string toEngine(const Seq &s);
bool isBinaryOp(const Node &n);

struct Cursor { Seq *seq; int pos; };

class Editor {
public:
    Editor() { cur = {&root, 0}; }
    Seq root;
    Cursor cur;

    void reset() { root.clear(); cur = {&root, 0}; }
    void loadFrom(const Seq &src) { root.clear(); clone(src, root); toEnd(); }
    void toEnd() { cur = {&root, root.size()}; }
    void toStart() { cur = {&root, 0}; }
    bool empty() const { return root.v.empty(); }

    void insertNode(NodeP n);
    void type(const std::string &shown, const std::string &engine = "") { insertNode(makeText(shown, engine)); }
    void typeFunc(const std::string &shown, const std::string &engine, const std::string &sup = "") {
        insertNode(makeFunc(shown, engine, sup));
    }
    void digits(const std::string &d);

    void frac();                       // captures the number typed just before
    void mixed();
    void sqrtT();
    void rootT(const std::string &preIndex);   // "" = cursor starts in the index
    void powT(const std::string &preset);      // "" = cursor goes into the exponent
    void absT();
    void logB();
    void exp10();                      // "x10" followed by an empty exponent

    void del();
    void left();
    void right();
    bool up();                         // false = nothing to move to (caller may replay history)
    bool down();

    std::string engine() const { return toEngine(root); }

private:
    void enter(Node *n, int slot) { cur = {&n->slots[slot], 0}; }
    void afterNode(Node *n) { cur = {n->parent, n->parent->indexOf(n) + 1}; }
};

} // namespace nat
