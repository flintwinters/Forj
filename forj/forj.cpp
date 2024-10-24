#include <cstdio>
#include <stdio.h>
#include <map>
#include <string>
#include <vector>

using namespace std;
typedef long long word;

// "optional" object implementation
// Stores either the T value, or an error message.
template <typename T> class Maybe : public string {
public:
    T val;
    int i = -1;
    int l = 0;
    bool success = false;
    Maybe(T val): val(val), success(true) {}
    Maybe(string s, int i, int l): string(s), i(i), l(l) {}
    T operator->() {return val;}
    operator bool() {return success;}
    operator T() {return val;}
    bool operator==(T v) {return val == v;}
    Maybe<T> addmsg(string s) {return Maybe<T>((*this)+(s), i, l);}
    string str() {return "<" + to_string(i) + ">: " + (*this);}
    string str(string s) {
        if (l) {s.insert(i+l, "\033[0m");}
        s.insert(i, "\033[31;1;4m!>>>>>");
        return "\033[33;1m<" + to_string(i) + ">: " + (*this) + "\033[0m\n" + s + "\033[0m";
    }
};
// Shorthand to build a failed Maybe
template <typename T> Maybe<T> Fail(string msg, int i = 0, int l = 0) {return Maybe<T>(msg, i, l);}

// Quintessential stack datastructure
template <typename T> class Stack {
private:
    vector<T> s;
protected:
    string str(string (tostr)(T)) {
        string s = " ";
        for (int i = 0; i <= sp; i++) {s += tostr(peek(i)) + " ";}
        return s;
    }
public:
    int sp = -1;
    Stack() {}
    T pull() {return s[sp--];}
    T push(T t) {
        sp++;
        if (sp >= s.size()) {s.push_back(t);}
        else {s[sp] = t;}
        return t;
    }
    T peek(int i = 0) {return s[sp-i];}
    bool isempty() {return sp<=-1;}
};

class Wrap;
class Node;
typedef Maybe<Node*>(*func)(Node*, Wrap*);
// Prim types.
// Want to replace these in-lang eventually
Node* ttype;
Node* tnothing;
Node* tstring;
Node* tliteral;
Node* tarray;
Node* tbang;
Node* texec;
// Tree-like datastructure.
// Nodes track variable names, and have their own stacks.
vector<Node*> allnodes;
class Node : public map<string, Node*>, public Stack<Node*> {
public:
    string name;
    Node* parent;
    Node* type;
    word val = 0;
    func f;
    Node(string s, Node* t, Node* p): name(s), type(t), parent(p) {}
    static Node* New(string s, Node* t, Node* p) {
        allnodes.push_back(new Node(s, t, p));
        return allnodes.back();
    }
    static void deleteall() {for (Node* n : allnodes) {delete n;}}
    bool in(string s) {return find(s) != end();}
    Node* addvar(string s, Node* t) {return (*this)[s] = Node::New(s, t, this);}
    Maybe<Node*> getvar(string s) {
        if (in(s)) {return (*this)[s];}
        return Fail<Node*>("Couldn't find string '" + s + "' in scope '" + name + "'");
    }
    Maybe<Node*> global(string s) {
        if (in(s)) {return (*this)[s];}
        if (parent) {
            Maybe<Node*> m = parent->global(s);
            if (m) {return m;}
            return m.addmsg(":" + name);
        }
        return Fail<Node*>("Couldn't find string '" + s + "' in scope '" + name + "'");
    }
    string str() {
        return Stack::str([](Node* n)->string{
            if (n->type == tliteral) {return "\033[90;1m" + to_string(n->val) + "\033[0m";}
            if (n->type == texec)    {return "\033[31m" + to_string(n->val) + "\033[0m";}
            if (n->type == tstring)  {return "\033[32m\"" + *(string*) n->val + "\"\033[0m";}
            if (n->isempty()) {return "<>";}
            if (n->type == tarray) {return " <"+n->str()+"> ";}
            return "<"+to_string(n->val)+">";
        });
    }
};
// "Thread" of execution
// tracks the path of the current scope
class Wrap {
public:
    Node* t;
    bool searching = false;
    Wrap* prev, *next;
    Wrap(Node* t, Wrap* p, Wrap* n = 0): t(t), prev(p), next(n) {}
    Wrap* pushscope(Node* node) {return next = new Wrap(node, this);}
    Wrap* pullscope() {Wrap* w = prev; w->next = next; delete this; return w;}
    Maybe<Node*> global(string s) {
        Maybe<Node*> m = t->global(s);
        if (m) {return m;}
        if (prev) {return prev->global(s);}
        return m;
    }
    Maybe<Node*> search(string s) {return (searching) ? t->getvar(s) : global(s);}
    int scopedepth() {
        if (prev) {return 1+prev->scopedepth();}
        return 1;
    }

    Node* peek(int i = 0) {return t->peek(i);}
    Node* push(Node* n) {return t->push(n);}
    Node* pull() {return t->pull();}
};

// Parser class
class Text : public Stack<string> {
public:
    int idx = 0;
    Wrap* W = 0;
    Stack<string> U;
    Text(string s, Wrap* w): W(w) {push(s);}
    string pull() {
        idx += peek().size();
        return U.push(Stack::pull());
    }
    string split(int i, int n = 0) {
        string s = Stack::pull();
        push(s.substr(i+n));
        push(s.substr(0, i));
        idx += n;
        return peek();
    }
    Maybe<int> find(string match) {
        if (!peek().size()) {return Fail<int>("Couldn't find in empty string", idx, peek().size());}
        int i = 0;
        for (; match.find(peek()[i]) == string::npos; i++) {
            if (i >= peek().size()) {
                return Fail<int>("Couldn't find any of '" + match + "' in: '" + peek() + "'", idx, peek().size());
            }
        }
        return i;
    }
    Maybe<char> splitfind(string match, int n = 0) {
        Maybe<int> i = find(match);
        if (i) {char c = peek()[i.val]; split(i, n); return c;}
        return Fail<char>(i, idx, peek().size());
    }
    bool isnum(char c) {return in("0123456789", c);}
    int tonum(string s) {
        int n = 0;
        int i = 0;
        int base = 10;
        if (s[0] == '0') {
            if (s[1] == 'x') {base = 16; i+=2;}
            else if (s[1] == 'b') {base = 2; i+=2;}
        }
        return stoi(s.substr(i), nullptr, base);
    }
    void print() {printf("<%s>\n", str([](string s)->string{return s;}).c_str());}

    bool in(string match, char n);
    bool cleanwhitespace();
    string capture(string close);
    Maybe<string> final(string s);
    Maybe<string> parse();
};

bool Text::in(string match, char n) {for (char c : match) {if (c == n) {return true;}} return false;}
bool Text::cleanwhitespace() {
    int i = 0;
    while (in(" \n\t\r\b", peek()[i])) {i++;}
    if (i) {
        if (i == peek().size()) {pull(); return true;}
        split(i-1, 1);
        pull();
    }
    return false;
}

string Text::capture(string close) {
    pull();
    while (peek() == "") {pull();}
    string str = "";
    Maybe<int> i = find(close);
    while (!i) {str += pull(); i = find(close);}
    split(i, 1);
    str += pull();
    return str;
}
Maybe<string> Text::final(string s) {
    if (s == "") {return peek();}
    if (isnum(s[0])) {
        W->push(Node::New(s, tliteral, W->t))->val = tonum(s);
        return peek();
    }
    Maybe<Node*> m = W->search(s);
    if (!m) {return Fail<string>(m, idx, s.size());}
    if (W->searching) {W = W->pullscope(); W->push(m);}
    else {W->push(m);}
    return peek();
}
Maybe<string> Text::parse() {
    if (cleanwhitespace()) {return string(" ");}
    Maybe<char> t = splitfind(" \n\t\r\b");
    t = splitfind(":[](!\"");
    if (!t) {
        Maybe<string> m = final(peek());
        if (m) {return pull();}
        return m;
    }
    string s = pull();
    if (t == ':') {
        split(1);
        if (s == "") {W = W->pushscope(W->t);}
        else {
            Maybe<Node*> m = W->search(s);
            if (!m) {return Fail<string>(m, idx) + " For subscope of '" + m->name + "'";}
            if (!W->searching) {W = W->pushscope(m);}
            W->t = m;
        }
        W->searching = true;
    }
    split(1);
    if (s != "") {final(s);}
    if (t == '!') {
        pull(); pull();
        int i = 0;
        while (peek()[i] == '!') {i++;}
        split(i);
        pull();
        if (i) {W->push(Node::New(peek(), tbang, W->t))->val = i;}
        else {W->peek()->type->f(W->peek(), W);}
        W->searching = false;
        return s;
    }
    else if (t == '[') {
        Node* n = W->t;
        if (W->searching) {W = W->pullscope()->pushscope(n);}
        else {W = W->pushscope(Node::New("", tarray, n));}
    }
    else if (t == ']') {
        Node* n = W->t;
        (W = W->pullscope())->push(n);
    }
    else if (t == '\"') {
        string str = capture("\"");
        if (W->searching) {
            W->t->addvar(str, tnothing);
            W = W->pullscope();
            return pull();
        }
        W->push(Node::New("", tstring, 0))->val = (word) new string(str);
        return str;
    }
    else if (t == '(') {return capture(")");}
    return pull();
}
