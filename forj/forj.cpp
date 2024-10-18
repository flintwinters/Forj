#include <cstdint>
#include <stdio.h>
#include <map>
#include <vector>
#include <string>

using namespace std;
typedef long long word;

template <typename T> class Maybe {
public:
    T val;
    string msg;
    int l, a, b;
    bool fail;
    Maybe(): fail(false) {}
    Maybe(T val): val(val), fail(false) {}
    Maybe(string msg, int l, int a, int b): msg(msg), l(l), a(a), b(b), fail(true) {}
};

template <typename T> Maybe<T> Fail(string msg, int l = 0, int a = 0, int b = 0) {return Maybe<T>(msg, l, a, b);}
template <typename T> Maybe<T> Success(T val) {return Maybe<T>(val);}

template <typename T> class Stack {
private:
    vector<T> s;
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
    T operator[](int i) {return s[sp-i];}
};

typedef struct obj {word w; word t;} obj;

bool contains(string s, char c) {
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == c) {return true;}
    }
    return false;
}

class TypedStack {
public:
    Stack<obj> S;
    TypedStack() {}
    obj pull() {return S.pull();}
    void push(word w, word t) {S.push({w, t});}
    obj operator[](int i) {return S[i];}
};

class Scope {
public:
    string name;
    map<string, Scope*> M;
    Scope(string name): name(name) {}
    Scope* append(string s) {return M[s] = new Scope(s);}
    Maybe<Scope*> get(string s) {
        if (M.find(s) == M.end()) {return Fail<Scope*>("Couldn't find '"+s+"' in scope");}
        return Success(M[s]);
    }
};

class Token {
public:
    string s;
    Token() {}
    Token(string s): s(s) {}
    Token* split(int i) {
        Token* t = new Token(s.substr(0, i));
        s = s.substr(i+1);
        return t;
    }
    Maybe<int> find(string match) {
        int i = 0;
        while (!contains(match, s[i])) {
            if (i >= s.size()-1) {
                return Fail<int>("Couldn't find any of '" + match + "' in: '" + s + "'");
            }
            i++;
        }
        return Success(i);
    }
};

class Build {
public:
    TypedStack S;
    Stack<Scope*> scope;
    Stack<Token*> T;
    Build(string s) {T.push(new Token(s));}
    Token* splitback(int i) {return T.push(T[0]->split(i));}
    Maybe<Token*> parse() {
        Maybe<int> t = T[0]->find(" ");
        if (t.fail) {return Fail<Token*>(t.msg);}
        splitback(t.val);
        Scope* search = scope[0];
        Maybe<Scope*> m;
        string s;
        while (!(t = T[0]->find(":")).fail) {
            splitback(t.val);
            m = search->get(T[0]->s);
            if (m.fail) {break;}
            search = m.val;
            s += T.pull()->s + ':';
        }
        if (T[0]->s == "[") {
            scope.push(search);
            return Success(T.pull());
        }
        m = search->get(T[0]->s);
        if (m.fail) {return Fail<Token*>("Failed to find scope '" + s+T[0]->s + "'");}
        S.push((word) m.val, 1);
        return Success(T.pull());
    }
};

int main() {
    FILE* FP = fopen("test.qfj", "r");
    string s;
    while (!feof(FP)) {s += fgetc(FP);}
    fclose(FP);

    Build b(s);
    b.scope.push(new Scope("global"));
    b.scope[0]->append("ok")->append("beeb")->append("sob")->append("soob");
    Maybe<Token*> m = b.parse();
    if (m.fail) {printf("%s\n", m.msg.c_str());}
    else {printf("%s\n", m.val->s.c_str());}
    m = b.parse();
    if (m.fail) {printf("%s\n", m.msg.c_str());}
    else {printf("%s\n", m.val->s.c_str());}
    return 0;
}