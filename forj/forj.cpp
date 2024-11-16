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
    T val; // literal value of the Maybe
    int i = -1; // index of the error in the file
    int len = 0;  // length of the problem token
    bool success = false;
    Maybe(T val): val(val), success(true) {}
    Maybe(string s, int i, int l): string(s), i(i), len(l) {}
    T operator->() {return val;}
    operator bool() {return success;}
    operator T() {return val;}
    bool operator==(T v) {return val == v;}
    // append to the error message.
    Maybe<T> addmsg(string s) {return Maybe<T>((*this)+(s), i, len);}
    string str() {return "<" + to_string(i) + ">: " + (*this);}
    string str(string s);
};
// Shorthand to build a failed Maybe
template <typename T> Maybe<T> Fail(string msg, int i = 0, int l = 0) {return Maybe<T>(msg, i, l);}

// Quintessential stack datastructure
template <typename T> class Stack {
private:
    vector<T> s;
public:
    int sp = -1; // stack pointer
    Stack() {}
    Stack(Stack& stk): s(stk.s) {}
    T pull(int i = 1) {return s[(sp -= i)+1];}
    T push(T t) {
        sp++;
        if (sp >= s.size()) {s.push_back(t);}
        else {s[sp] = t;}
        return t;
    }
    T peek(int i = 0) {return s[sp-i];}
    void swapi(int i = 0) {
        T b = peek(i);
        s[sp-i] = peek();
        s[sp] = b;
    }
    bool isempty() {return sp<=-1;}
    string str(string (tostr)(T));
};

class Wrap;
class Node;
// function type for execution of nodes.
typedef int(*func)(Wrap*);
// Prim types.
// Want to replace these in-lang eventually
Node* ttype;
Node* tnothing;
Node* tstring;
Node* tliteral;
Node* tarray;
Node* tcontext;
Node* tbang;
Node* texec;
// Tree datastructure.
// Nodes track variable names, and have their own optional stacks.
vector<Node*> allnodes;
class Node : public map<string, Node*>, public Stack<Node*> {
public:
    Node* parent = 0;
    string name;
    word val = 0;
    func f = 0; // executable function
    // Create a new node, adding it to the vect so it can be deleted later
    Node(string s, Node* t): name(s) {
        settype(t);
        allnodes.push_back(this);
    }

    Node(Node& n): map<string, Node*>(n), Stack<Node*>(n) {
        name = n.name;
        settype(n.gettype());
        parent = n.parent;
        val = n.val;
        f = n.f;
        allnodes.push_back(this);
    }
    Node* gettype() {return (*this)["#"];}
    Node* settype(Node* t) {return (*this)["#"] = t;}
    // Delete all nodes created with `New`
    static void deleteall() {for (Node* n : allnodes) {delete n;}}
    // If the variable `s` exists
    bool in(string s) {return find(s) != end();}
    Node* addvar(string s, Node* t) {return (*this)[s] = new Node(s, t);}
    Maybe<Node*> getvar(string s) {
        if (in(s)) {return (*this)[s];}
        return Fail<Node*>("Couldn't find string '" + s + "' in scope '" + name + "'");
    }
    int exec(Wrap* W);
    // Search all parents for `s`
    Maybe<Node*> search(string s) {
        if (in(s)) {return (*this)[s];}
        if (parent) {
            Maybe<Node*> m = parent->getvar(s);
            if (m) {return m;}
        }
        if (gettype()) {
            Maybe<Node*> m = gettype()->getvar(s);
            if (m) {return m;}
        }
        return Fail<Node*>("Couldn't find string '" + s + "' in scope '" + name + "'");
    }
    string tostr(Node* n);
    string str();
};
// "Line" of execution
// tracks the path of the current scope
class Wrap {
public:
    Node* t; // Current node
    bool debugging = false;
    // Are we currently searching, ie `a:b:c`
    bool searching = false;
    Wrap* prev;
    Wrap(Node* t, Wrap* p, Wrap* n = 0): t(t), prev(p) {}
    // Move to a scope, ie: hello:[]
    Wrap* pushscope(Node* node) {return new Wrap(node, this);}
    Wrap* pullscope() {Wrap* w = prev; delete this; return w;}

    Maybe<Node*> global(string s) {
        Maybe<Node*> m = t->search(s);
        if (m) {return m;}
        if (!t->isempty()) {
            m = peek()->search(s);
            if (m) {return m;}
        }
        if (prev) {return prev->global(s);}
        return m;
    }
    Maybe<Node*> search(string s) {
        return (searching) ? t->search(s) : global(s);
    }
    int scopedepth() {
        if (prev) {return 1+prev->scopedepth();}
        return 1;
    }

    // Separated into a function for breakpoints
    void debug() {
        printf("\033[0m%s\n", t->str().c_str());
    }
    // Pass functions to the current node
    Node* peek(int i = 0) {return t->peek(i);}
    Node* push(Node* n) {
        t->push(n);
        if (debugging) {debug();}
        return t->peek();
    }
    Node* pull(int i = 1) {return t->pull(i);}
};

int Node::exec(Wrap* W) {
    W->push(this);
    return gettype()->f(W);
}

// Parser class
class Text : public Stack<string> {
public:
    int idx = 0;
    Wrap* W = 0;
    Stack<string> U;
    Text(string s, Wrap* w): W(w) {push(s);}
    // pull a token
    string pull() {
        idx += peek().size();
        return U.push(Stack::pull());
    }
    // split the top token at i
    string split(int i, int n = 0) {
        string s = Stack::pull();
        push(s.substr(i+n));
        push(s.substr(0, i));
        idx += n;
        return peek();
    }
    // maybe find the first index of a char in match
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
    // maybe find and split at the first match
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
    while (!i) {
        str += pull();
        i = find(close);
    }
    split(i, 1);
    str += pull();
    return str;
}
Maybe<string> Text::final(string s) {
    if (s == "") {return peek();}
    if (isnum(s[0]) || (s[0] == '-' && isnum(s[1]))) {
        W->push(new Node(s, tliteral))->val = tonum(s);
        return peek();
    }
    Maybe<Node*> m = W->search(s);
    if (!m) {return Fail<string>(m, idx, s.size());}
    if (W->searching) {
        W = W->pullscope();
        W->push(m);
    }
    else {W->push(m);}
    return peek();
}
int arrayfunc(Node* n, Wrap* W);
Maybe<string> Text::parse() {
    if (cleanwhitespace()) {return string(" ");}
    Maybe<char> t = splitfind(" \n\t\r\b");
    t = splitfind(":[](!\"`");
    if (!t) {
        Maybe<string> m = final(peek());
        if (m) {return pull();}
        return m;
    }
    string s = pull();
    split(1);
    if (t == ':') {
        if (s == "") {W = W->pushscope(W->t);}
        else {
            Maybe<Node*> m = W->search(s);
            if (!m) {return Fail<string>(m, idx) + " For subscope of '" + s + "'";}
            if (!W->searching) {W = W->pushscope(m);}
            W->t = m;
        }
        W->searching = true;
    }
    if (s != "") {final(s);}
    if (t == '!') {
        pull(); 
        int i = -1;
        while (peek()[++i] == '!');
        split(i);
        pull();
        if (i) {W->push(new Node(peek(), tbang))->val = i;}
        else if (!W->peek()->exec(W)) {
            return Fail<string>("function errored");
        }
        W->searching = false;
        return s;
    }
    else if (t == '[') {
        Node* n = W->t;
        if (W->searching) {W = W->pullscope()->pushscope(n);}
        else {W = W->pushscope(new Node("", tarray));}
    }
    else if (t == ']') {
        Node* n = W->t;
        (W = W->pullscope())->push(n);
    }
    else if (t == '\"' || t == '`') {
        string str;
        if (t == '\"') {str = capture("\"");}
        else {str = capture("`");}
        if (W->searching) {
            Node* v = W->t->addvar(str, tnothing);
            W = W->pullscope();
            W->push(v);
            return peek();
        }
        W->push(new Node("", tstring))->val = (word) new string(str);
        return str;
    }
    else if (t == '(') {return capture(")");}
    return pull();
}
