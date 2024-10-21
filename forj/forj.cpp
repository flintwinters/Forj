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
    int i = -1;
    bool fail;
    Maybe(): fail(false) {}
    Maybe(T val): val(val), fail(false) {}
    Maybe(string msg, int i): msg(msg), i(i), fail(true) {}
    Maybe<T> operator+(string s) {return Maybe(msg+s, i);}
    string str() {return "<" + to_string(i) + ">: " + msg;}
    string str(string s) {
        s.insert(i-1, "\033[31;1;4m!>>>>>");
        return "<" + to_string(i) + ">: " + msg + s + "\033[0m";
    }
};

template <typename T> Maybe<T> Fail(string msg, int i = 0) {return Maybe<T>(msg, i);}
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

typedef enum type {
    nothing, bang, scopetype
} type;

bool contains(string s, char c) {
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == c) {return true;}
    }
    return false;
}

class Scope {
public:
    string str;
    word val;
    type t = nothing;
    Scope* parent;
    map<string, Scope*> M;
    Scope(string str): str(str) {}
    Scope(string str, Scope* parent): str(str), parent(parent) {}
    Scope* append(string s) {
        return M[s] = new Scope(s, this);
    }
    Maybe<Scope*> get(string s) {
        if (M.find(s) == M.end()) {return Fail<Scope*>("Couldn't find '"+s+"' in scope");}
        return Success(M[s]);
    }
    Maybe<Scope*> searchall(string s) {
        Maybe<Scope*> m = get(s);
        if (!m.fail) {return m;}
        if (parent) {return parent->searchall(s);}
        return Fail<Scope*>("Couldn't find '"+s+"' in any parent scope");
    }
};

typedef struct obj {word w; Scope* t;} obj;
class TypedStack {
public:
    Stack<obj> S;
    TypedStack() {}
    obj pull() {return S.pull();}
    void push(word w, Scope* t) {S.push({w, t});}
    obj operator[](int i) {return S[i];}
    string str() {
        string top = "w: ";
        string bot = "t: ";
        for (int i = S.sp; i >= 0; i--) {
            top += ((Scope*) S[i].w)->str + "\t";
            bot += S[i].t->str + "\t";
        }
        return top + "\n" + bot;
    }
};

class Token {
public:
    string s;
    Token() {}
    Token(string s): s(s) {}
    Token* split(int i, int n) {
        Token* t = new Token(s.substr(0, i));
        s = s.substr(i+n);
        return t;
    }
    Maybe<int> find(string match, string escape = "") {
        int i = 0;
        if (!s.size()) {return Fail<int>("Couldn't find in empty string");}
        while (!contains(match, s[i])) {
            if (i >= s.size()-1) {
                return Fail<int>("Couldn't find any of '" + match + "' in: '" + s + "'");
            }
            i++;
            if (contains(escape, s[i])) {i += 2;}
        }
        return Success(i);
    }
};

bool isnum(char c) {return contains("0123456789", c);}
Maybe<int> tonum(Token* t) {
    int n = 0;
    int i = 0;
    int base = 10;
    if (t->s[0] == '0') {
        if (t->s[1] == 'x') {base = 16; i++;}
        else if (t->s[1] == 'b') {base = 2; i++;}
        i++;
    }
    for (char c : t->s) {
        if (c == '_') {continue;}
        if (!isnum(c)) {
            if (base == 16 && contains("abcdef", t->s[i])) {
                n += c-'a';
            }
            else {
                return Fail<int>("Non numeric char encountered", i);
            }
        }
        if (isnum(c)) {n += c-'0';}
        i++;
        n *= base;
    }
    n /= base;
    return n;
}

class Build {
public:
    TypedStack S;
    Stack<Scope*> scope;
    Stack<Token*> T;
    Stack<Token*> U;
    Scope* search = 0;
    string s;
    int parseidx = 0;
    Build(string s): s(s) {
        T.push(new Token(s));
        U.push(new Token(""));
    }
    void splitback(int i, int n = 0) {
        if (i == T[0]->s.length()) {return;}
        parseidx += n;
        T.push(T[0]->split(i, n));
    }
    Scope* getscope() {
        if (search) {return search;}
        return scope[0];
    }
    Token* pull() {
        parseidx += T[0]->s.size();
        // printf("<%s|%s>\n", T[0]->s.c_str(), U[0]->s.c_str());
        return U.push(T.pull());
    }
    Maybe<Scope*> get(string s) {return getscope()->get(s);}
    Maybe<Scope*> searchall(string s) {return getscope()->searchall(s);}
    Maybe<Token*> final() {
        Maybe<Scope*> m = get(T[0]->s);
        Maybe<int> n = tonum(T[0]);
        if (m.fail == n.fail) {
            return Fail<Token*>("Failed to find scope '" + T[0]->s + "'", parseidx) + " For attempted final resolution";
        }
        if (!m.fail) {S.push((word) m.val, searchall("scopetype").val);}
        else {S.push(n.val, searchall("scopetype").val);}
        pull();
        return Success(T[0]);
    }
    Maybe<Token*> execute() {
        return Success<Token*>(0);
    }
    Maybe<Token*> parse() {
        int i = 0;
        while (contains(" \n\t\r\b", T[0]->s[i])) {i++;}
        if (i) {
            if (i == T[0]->s.size()) {return Success(pull());}
            splitback(i-1, 1);
            pull();
        }
        Maybe<int> t = T[0]->find(" \n\t\r\b");
        if (!t.fail) {
            splitback(t.val, 1);
            if (!t.val) {parse();}
        }
        t = T[0]->find(":[]!\"");
        if (t.fail) {return final();}
        splitback(t.val);
        Token* tok = pull();
        if (!contains("!", T[0]->s[0])) {splitback(1);}
        if (T[0]->s == ":") {
            if (tok->s == "") {search = getscope();}
            else {
                Maybe<Scope*> m = get(tok->s);
                if (m.fail) {return Fail<Token*>(m.msg, parseidx) + " For subscope resolution";}
                search = m.val;
            }
            pull();
            return parse();
        }
        if (T[0]->s == "[") {
            scope.push(getscope());
            if (search) {search = 0;}
            return Success(pull());
        }
        if (T[0]->s == "]") {
            if (search) {return Fail<Token*>("Can't close bracket now", parseidx);}
            scope.pull();
            return Success(pull());
        }
        if (T[0]->s == "\"") {
            pull();
            if (!search) {
                string s = "";
                while ((t = T[0]->find("\"")).fail) {s += pull()->s;}
                splitback(t.val, 1);
                s += pull()->s;
                S.push((word) new Scope(s), searchall("string").val);
                return Success(pull());
            }
            t = T[0]->find("\"");
            splitback(t.val);
            if (t.fail) {return Fail<Token*>(t.msg, parseidx);}
            scope[0]->append(pull()->s);
            search = 0;
            return Success(pull());
        }
        i = 1;
        while (T[0]->s[i] == '!') {i++;}
        splitback(i);
        if (i == 1) {pull(); return execute();}
        S.push(i, searchall("bang").val);
        search = 0;
        return Success(pull());
    }
};

int main() {
    FILE* FP = fopen("test.qfj", "r");
    string s;
    while (!feof(FP)) {s += fgetc(FP);}
    char c;
    while ((c = fgetc(FP)) != EOF) {s += c;}
    s = s.substr(0, s.size()-1);
    fclose(FP);

    Build b(s);
    printf("FILE:\"\"\"%s\"\"\"\n", b.s.c_str());
    b.scope.push(new Scope("global"));
    b.scope[0]->append("nothing");
    b.scope[0]->append("bang");
    b.scope[0]->append("scopetype");
    b.scope[0]->append("string");
    b.scope[0]->append("executable");
    b.scope[0]->append("ok")->append("beeb")->append("sob")->append("soob");
    Maybe<Token*> m;
    while (b.T.sp != -1) {
        m = b.parse();
        if (m.fail) {
            printf("Error:\n");
            printf("%s\n", (m+"\n"+b.S.str()).str(b.s).c_str());
            return 1;
        }
    }
    printf("%s\n", b.S.str().c_str());
    return 0;
}