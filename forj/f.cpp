#include <stdio.h>
#include <map>
#include <string>
#include <vector>

using namespace std;
typedef long long word;

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
        return "\033[33;1;1m<" + to_string(i) + ">: " + (*this) + "\033[0m\n" + s + "\033[0m";
    }
};
template <typename T> Maybe<T> Fail(string msg, int i = 0, int l = 0) {return Maybe<T>(msg, i, l);}
template <typename T> Maybe<T> Success(T val) {return Maybe<T>(val);}


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

class Node : public map<string, Node*>, public Stack<Node*> {
public:
    string name;
    Node* parent;
    Node* type;
    word val;
    Node(string s, Node* t, Node* p = 0): name(s), type(t), parent(p) {}
    bool in(string s) {return find(s) != end();}
    Node* addvar(string s, Node* t) {return (*this)[s] = new Node(s, t);}
    Maybe<Node*> get(string s) {
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
            if (n->isempty()) {return to_string(n->val);}
            return "<"+n->str()+">";
        });
    }
};
class Wrap {
public:
    Node* t;
    bool searching = false;
    Wrap* prev, *next;
    Wrap(Node* t, Wrap* p, Wrap* n = 0): t(t), prev(p), next(n) {}
    Wrap* pushscope(Node* node) {return next = new Wrap(node, this);}
    Wrap* pullscope() {Wrap* w = prev; delete this; return w;}
    Maybe<Node*> global(string s) {
        Maybe<Node*> m = t->global(s);
        if (m) {return m;}
        if (prev) {return prev->global(s);}
        return m;
    }
    Maybe<Node*> search(string s) {return (searching) ? t->get(s) : global(s);}

    Node* peek(int i = 0) {return t->peek(i);}
    Node* push(Node* n) {return t->push(n);}
    Node* push() {return push(new Node("", 0, t));}
    Node* pull() {return t->pull();}
};

class Text : public Stack<string>, public string {
public:
    int idx = 0;
    Wrap* W = 0;
    Stack<string> U;
    Text(string s, Wrap* w): string(s), W(w) {push(s);}
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
        return Success(i);
    }
    Maybe<char> splitfind(string match, int n = 0) {
        Maybe<int> i = find(match);
        if (i) {char c = peek()[i.val]; split(i, n); return Success(c);}
        return Fail<char>(i, idx, peek().size());
    }
    bool isnum(char c) {return in("0123456789", c);}
    int tonum(string s) {
        int n = 0;
        int i = 0;
        int base = 10;
        if (s[0] == '0') {
            if (s[1] == 'x') {base = 16; i++;}
            else if (s[1] == 'b') {base = 2; i++;}
            i++;
        }
        return stoi(s.substr(i), nullptr, base);
    }
    void print() {printf("<%s>\n", str([](string s)->string{return s;}).c_str());}

    bool in(string match, char n);
    bool cleanwhitespace();
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
Maybe<string> Text::parse() {
    if (cleanwhitespace()) {return Success(" ");}
    Maybe<char> t = splitfind(" \n\t\r\b");
    t = splitfind(":[]!\"");
    if (!t) {
        string s = peek();
        if (s == "") {return Success(pull());}
        if (isnum(s[0])) {W->push()->val = tonum(s); return Success(pull());}
        Maybe<Node*> m = W->search(s);
        if (!m) {return Fail<string>(m, idx, s.size());}
        if (W->searching) {W = W->pullscope(); W->push(m);}
        else {W->push(m);}
        return Success(pull());
    }
    string s = pull();
    if (t == '!') {
        int i = 1;
        while (peek()[i] == '!') {i++;}
        split(i);
        if (i == 1) {/* return execute */}
        /* push bang predecessor */
        W->searching = false;
        return Success(pull());
    }
    split(1);
    if (t == ':') {
        if (s == "") {(W = W->pushscope(W->t))->searching = true;}
        else {
            Maybe<Node*> m = W->search(s);
            if (!m) {return Fail<string>(m, idx) + " For subscope of '" + m->name + "'";}
            if (!W->searching) {W = W->pushscope(m);}
            W->searching = true;
            W->t = m;
        }
        // pull();
    }
    else if (t == '[') {
        if (W->searching) {W->searching = false;}
        else {W = W->pushscope(new Node("", 0));}
        return Success(pull());
    }
    else if (t == ']') {
        Node* n = W->t;
        (W = W->pullscope())->push(n);
    }
    else if (t == '\"') {
        pull();
        string str = "";
        Maybe<int> i = find("\"");
        while (!i) {s += pull(); i = find("\"");}
        split(i, 1);
        s += pull();
        if (W->searching) {
            W->t->addvar(s, new Node(s, 0, W->t));
            W->searching = false;
            return Success(pull());
        }
        W->push(new Node("", 0));
        W->peek()->val = (word) new string(s);
        return Success(pull());
    }
    return Success(pull());
}

int main() {
    Node* Global = new Node("Global", 0);
    Wrap* W = new Wrap(Global, 0);
    W->pushscope(W->t->addvar("hi", 0));
    (*W->t)["hi"]->val = 3;
    (*W->t)["hi"]->addvar("hello", 0);
    (*(*W->t)["hi"])["hello"]->val = 2;
    // W->next->t->addvar("hello");
    Text* t = new Text(" 123  hi:hello [ hi ] :\"hell\" hell hell \"hi\" 543  ", W);
    t->print();
    Maybe<string> m = t->parse();
    while (m && !t->isempty()) {
        m = t->parse();
        if (!m) {printf("%s\n", m.str(*t).c_str()); return 1;}
    }
    printf("<%s>\n", W->t->str().c_str());
}