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
        return "\033[33;1;1m<" + to_string(i) + ">: " + msg + "\033[0m\n" + s + "\033[0m";
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

typedef void(*func)(void);

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
    Scope* t = 0;
    Scope* parent;
    map<string, Scope*> M;
    Scope(string str, Scope* parent, Scope* t): str(str), t(t), parent(parent) {}
    Scope* append(string s, Scope* t) {
        return M[s] = new Scope(s, this, t);
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

int min(int a, int b) {return (a > b) ? b : a;}

string printstack(Stack<Scope*> S) {
    string top = "w:\t";
    string bot = "t:\t";
    for (int i = S.sp; i >= 0; i--) {
        if (S[i]->t->str != "literal") {
            string a = S[i]->str;
            top += a.substr(0, min(4, a.size())) + "\t";
        }
        else {top += to_string(S[i]->val) + "\t";}
        string b = S[i]->t->str;
        bot += b.substr(0, min(4, b.size())) + "\t";
    }
    return "\033[34;1;4m" + top + "\033[0m\n\033[35;1;1m" + bot + "\033[0m";
}

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
    Stack<Scope*> S;
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
        return U.push(T.pull());
    }
    Maybe<Scope*> get(string s) {return getscope()->get(s);}
    Maybe<Scope*> searchall(string s) {return getscope()->searchall(s);}
    Maybe<Token*> final() {
        Maybe<Scope*> m = (search) ? get(T[0]->s) : searchall(T[0]->s);
        Maybe<int> n = tonum(T[0]);
        if (m.fail == n.fail) {
            return Fail<Token*>("Failed to find scope '" + T[0]->s + "'", parseidx) + " For attempted final resolution in '" + getscope()->str + "'";
        }
        if (!m.fail) {S.push(m.val);}
        else {
            S.push(new Scope("", scope[0], searchall("literal").val));
            S[0]->val = n.val;
        }
        pull();
        return Success(T[0]);
    }
    Maybe<Scope*> execute(Stack<Scope*>& s) {
        if (s[0]->t->str == "executable") {((func) s[0]->val)();}
        else if (s[0]->t->str == "literal") {}
        else if (s[0]->t->str == "bang") {
            if (s[0]->val == 2) {
                pull(); s.pull(); return execute(s);
            }
            s[0]->val -= 1;
        }
        else if (s[0]->t->str == "stacktype") {
            Stack<Scope*> v = *(Stack<Scope*>*) s[0]->val;
            for (int i = v.sp; i >= 0; i--) {
                if (v[i]->t->str == "executable") {
                    ((func) v[i]->val)();
                }
                if (v[i]->t->str == "bang") {
                    if (v[i]->val == 2) {s.pull(); execute(s);}
                }
                s.push(v[i]);
            }
        }
        else {return Fail<Scope*>("Can't execute type "+s[0]->t->str);}
        pull();
        return Success<Scope*>(s[0]);
    }
    bool cleanwhitespace() {
        int i = 0;
        while (contains(" \n\t\r\b", T[0]->s[i])) {i++;}
        if (i) {
            if (i == T[0]->s.size()) {return true;}
            search = 0;
            splitback(i-1, 1);
            pull();
        }
        Maybe<int> t = T[0]->find(" \n\t\r\b");
        if (!t.fail) {
            splitback(t.val);
            if (!t.val) {parse();}
        }
        return false;
    }
    Maybe<Token*> parse() {
        if (cleanwhitespace()) {return Success(pull());}
        Maybe<int> t = T[0]->find(":[]!\"");
        if (t.fail) {return final();}
        splitback(t.val);
        Token* tok = pull();
        if (T[0]->s[0] == '!') {
            int i = 1;
            while (T[0]->s[i] == '!') {i++;}
            splitback(i);
            if (i == 1) {
                Maybe<Scope*> s = execute(S);
                if (s.fail) {return Fail<Token*>("")+s.msg;}
            }
            S.push(new Scope("", getscope(), searchall("bang").val));
            search = 0;
            return Success(pull());
        }
        splitback(1);
        if (T[0]->s == ":") {
            if (tok->s == "") {search = getscope();}
            else {
                Maybe<Scope*> m = get(tok->s);
                if (m.fail) {return Fail<Token*>(m.msg, parseidx) + " For subscope of '" + getscope()->str + "'";}
                search = m.val;
            }
            pull();
            return parse();
        }
        if (T[0]->s == "[") {
            if (search) {scope.push(search);}
            else {scope.push(new Scope("", getscope(), searchall("stacktype").val));}
            search = 0;
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
                S.push(new Scope(s, getscope(), searchall("string").val));
                return Success(pull());
            }
            t = T[0]->find("\"");
            splitback(t.val);
            if (t.fail) {return Fail<Token*>(t.msg, parseidx);}
            scope[0]->append(pull()->s, searchall("scopetype").val);
            search = 0;
            return Success(pull());
        }
        return Success(pull());
    }
};


int N = 0;
void printplus() {
    printf("++++%d\n", N++);
}
int main() {
    FILE* FP = fopen("test.qfj", "r");
    string s;
    while (!feof(FP)) {s += fgetc(FP);}
    char c;
    while ((c = fgetc(FP)) != EOF) {s += c;}
    s = s.substr(0, s.size()-1);
    fclose(FP);

    Build b(s);
    // printf("FILE:\"\"\"%s\"\"\"\n", b.s.c_str());
    b.scope.push(new Scope("global", 0, 0));
    b.scope[0]->append("nothing", 0);
    b.scope[0]->append("bang", 0);
    b.scope[0]->append("literal", 0);
    Scope* scopetype = b.scope[0]->append("scopetype", 0);
    b.scope[0]->append("string", 0);
    b.scope[0]->append("stacktype", 0);
    Scope* executable = b.scope[0]->append("executable", 0);
    b.scope[0]->append("sayhi", 0);
    b.scope[0]->append("ok",    scopetype)->
                append("beeb",  scopetype)->
                append("sob",   scopetype)->
                append("soob",  scopetype);
    b.scope[0]->append("+", executable)->val = (word) printplus;
    Maybe<Token*> m;
    while (b.T.sp != -1) {
        m = b.parse();
        if (m.fail) {
            printf("\033[31;1;4mError:\033[0m\n");
            printf("%s\n%s\n", m.str(b.s).c_str(), printstack(b.S).c_str());
            return 1;
        }
    }
    printf("%s\n", printstack(b.S).c_str());
    return 0;
}